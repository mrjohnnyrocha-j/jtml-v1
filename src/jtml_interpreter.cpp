// jtml_interpreter.cpp
#include "../include/jtml_interpreter.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>

Interpreter::~Interpreter() {
    if (wsThread.joinable()) {
        wsThread.join();
         // Ensure the thread is joined before destruction
    }
}
Interpreter::Interpreter(JtmlTranspiler& transpilerRef)
    : transpiler(transpilerRef) // Initialize transpiler reference
{
    // Create the Renderer
    renderer = std::make_unique<JTML::Renderer>();
    // Create the WebSocket server
    wsServer = std::make_shared<JTML::WebSocketServer>();
    
    // Initialize the global and current environments
    globalEnv = std::make_shared<JTML::Environment>(nullptr, 0, renderer.get());
    currentEnv = globalEnv;

    globalEnv->setRenderer(renderer.get());
    currentEnv->setRenderer(renderer.get());

    // Initialize nodeID
    nodeID = 0;
    uniqueVarID  = 0;
    uniqueArrayVarID = 1;
    uniqueDictVarID = 1;

    std::thread wsThread([this]() {
        wsServer->run(8080);
    });
    wsThread.detach();

    wsServer->setOpenCallback(
        [this](websocketpp::connection_hdl hdl) {
            std::cout << "[DEBUG] New WebSocket connection established.\n";
            populateBindings(hdl);
    });

    // Set Renderer callback to send messages via WebSocket
    renderer->setFrontendCallback([this](const std::string& msg) {
        wsServer->broadcastMessage(msg);
    });

    // Set WebSocket message handler
    wsServer->setMessageCallback(
        [this](const std::string& msg, websocketpp::connection_hdl hdl) {
            handleFrontendMessage(msg, hdl);
        });

    // Assign Renderer to the global environment
 
}

// In jtml_interpreter.cpp

void Interpreter::populateBindings(websocketpp::connection_hdl hdl) {
    try {
        nlohmann::json bindingsJson;

        // Debug log: Starting the populateBindings process
        std::cout << "[DEBUG] Starting populateBindings for WebSocket connection.\n";

        // Use the global environment to gather bindings
        std::shared_ptr<JTML::Environment> env = globalEnv;
        if (!env) {
            std::cerr << "[ERROR] Global environment is not initialized.\n";
            throw std::runtime_error("Global environment is not initialized.");
        }

        // Debug log: Iterate through bindings
        std::cout << "[DEBUG] Gathering bindings from the global environment.\n";

        // Iterate through all bindings in the environment
        for (const auto& [varName, bindingInfos] : env->getBindings()) {
            std::cout << "[DEBUG] Processing variable: " << varName << " with " << bindingInfos.size() << " bindings.\n";

            for (const auto& binding : bindingInfos) {
                // Retrieve the variable's current value
                if (binding.bindingType == "attribute_event") {
                    continue;
                }
                std::shared_ptr<JTML::VarValue> varVal = env->getVariable(binding.varName);
                std::string valueStr = varVal ? varVal->toString() : "undefined";

                // Debug log: Binding details
                std::cout << "[DEBUG] Binding - ElementID: " << binding.elementId
                          << ", Attribute: " << binding.attribute
                          << ", BindingType: " << binding.bindingType
                          << ", Value: " << valueStr << "\n";

                // Populate the JSON based on the binding type
                if (binding.bindingType == "content") {
                    bindingsJson["content"][binding.elementId] = valueStr;
                } else if (binding.bindingType == "attribute") {
                    bindingsJson["attributes"][binding.elementId][binding.attribute] = valueStr;
                } else {
                    std::cerr << "[WARN] Unknown binding type: " << binding.bindingType << "\n";
                }
            }
        }

        // Debug log: Constructing the populateBindings message
        std::cout << "[DEBUG] Constructing the populateBindings message.\n";

        // Construct the populateBindings message
        nlohmann::json message;
        message["type"] = "populateBindings";
        message["bindings"] = bindingsJson;

        // Serialize the message to a string
        std::string messageStr = message.dump();

        // Debug log: Message serialization
        std::cout << "[DEBUG] Serialized message: " << messageStr << "\n";

        // Send the message to the client through the WebSocket
        wsServer->sendMessage(hdl, messageStr);

        // Debug log: Message sent
        std::cout << "[DEBUG] Sent populateBindings to frontend. Message size: " << messageStr.size() << " bytes\n";
    } catch (const std::exception& e) {
        // Log the error and optionally send an error message to the frontend
        std::cerr << "[ERROR] Failed to populate bindings: " << e.what() << "\n";
        wsServer->sendMessage(hdl, R"({"type": "error", "message": "Failed to populate bindings"})");
    }
}


struct ReturnException : public std::exception {
    std::shared_ptr<JTML::VarValue> value;

    ReturnException(std::shared_ptr<JTML::VarValue> val) : value(val) {}

    const char* what() const noexcept override {
        return "ReturnException";
    }
};

struct BreakException : public std::exception {
    std::shared_ptr<JTML::VarValue> value;

    BreakException(std::shared_ptr<JTML::VarValue> val) : value(val) {}

    const char* what() const noexcept override {
        return "Unexpected break outside loop";
    }
};

struct ContinueException : public std::exception {
    std::shared_ptr<JTML::VarValue> value;

    ContinueException(std::shared_ptr<JTML::VarValue> val) : value(val) {}

    const char* what() const noexcept override {
        return "Unexpected break outside loop";
    }
};

void Interpreter::handleFrontendMessage(const std::string& msg, websocketpp::connection_hdl hdl) {
    try {
        // Parse the incoming message as JSON
        auto parsedMessage = nlohmann::json::parse(msg);

        // Validate message structure
        if (!parsedMessage.contains("type")) {
            throw std::runtime_error("Message missing 'type' field.");
        }

        std::string type = parsedMessage["type"].get<std::string>();

        if (type == "event") {
            // Extract event details
            std::string elementIdStr = parsedMessage["elementId"].get<std::string>();
            std::string eventType = parsedMessage["eventType"].get<std::string>();
            std::vector<nlohmann::json> args = parsedMessage.value("args", std::vector<nlohmann::json>());

            std::cout << "[DEBUG] Event received: ElementID=" << elementIdStr 
                      << ", EventType=" << eventType << "\n";

      

            // Find the binding for the given elementId and attribute (eventType)
            JTML::CompositeKey elementVarKey = {globalEnv->instanceID, elementIdStr};
            auto bindingsIt = globalEnv->bindings.find(elementIdStr);
            if (bindingsIt != globalEnv->bindings.end()) {
                bool bindingFound = false;

                for (const auto& binding : bindingsIt->second) {
                    
                    if (binding.elementId == elementIdStr && binding.bindingType == "attribute_event") {
                        bindingFound = true;

                        std::cout << "[DEBUG] Found binding: ElementID=" << elementIdStr  
                                  << ", Attribute=" << eventType << "\n";

                        
                    if (eventType == "onInput") {
                        // Special handling for onInput: pass inputValue as the sole argument
                        std::vector<std::shared_ptr<JTML::VarValue>> args;
                        std::string inputValue = parsedMessage["args"][2].get<std::string>();
                        args.push_back(std::make_shared<JTML::VarValue>(inputValue));

                        // Ensure the expression is a function call
                        if (binding.expression->getExprType() != ExpressionStatementNodeType::FunctionCall) {
                            std::cerr << "[ERROR] onInput binding expression is not a function call.\n";
                            renderer->sendError("onInput binding expression is not a function call.");
                            continue;
                        }

                        // Cast to FunctionCallExpressionStatementNode to access functionName
                        auto funcCallExpr = std::static_pointer_cast<FunctionCallExpressionStatementNode>(binding.expression);
                        std::string functionName = funcCallExpr->functionName;

                        // Retrieve the function from the environment
                        JTML::CompositeKey funcKey{ globalEnv->instanceID, functionName };
                        auto func = globalEnv->getFunction(funcKey);
                        if (!func) {
                            std::cerr << "[ERROR] Function '" << functionName << "' not found.\n";
                            renderer->sendError("Function '" + functionName + "' not found.");
                            continue;
                        }

                        // Execute the function with args
                        auto result = executeFunction(func, args, nullptr);
                        

                        std::cout << "[DEBUG] Event handled: ElementID=" << elementIdStr 
                                    << ", EventType=" << eventType 
                                    << ", Result=" << (result ? result->toString() : "(null)") << "\n";


                        // Break after handling the binding
                        recalcDirty(globalEnv);
                        break;
                    }
                                   
                    // Evaluate the derived expression (assumes it's a function call or similar)
                    auto result = evaluateExpression(binding.expression.get(), globalEnv);

                    std::cout << "[DEBUG] Event handled: ElementID=" << elementIdStr    
                                << ", EventType=" << eventType 
                                << ", Result=" << (result ? result->toString() : "(null)") << "\n";

                    // Break after handling the binding
                    recalcDirty(globalEnv);
                    break;
                }
                
            }

            if (!bindingFound) {
                std::cerr << "[WARNING] No binding found for ElementID=" << elementIdStr
                            << ", EventType=" << eventType << "\n";
                renderer->sendError("No binding found for the triggered event.");
            }
        } else {
            std::cerr << "[WARNING] No bindings registered for event type: " << eventType << "\n";
            renderer->sendError("No bindings registered for event type: " + eventType + " and element name: " + elementIdStr);
        }
    } else {
            std::cerr << "[WARNING] Unrecognized message type: " << type << "\n";
            renderer->sendError("Unrecognized message type: " + type);
        }
    }
    catch (const nlohmann::json::exception& e) {
        std::cerr << "[ERROR] JSON parsing failed: " << e.what() << "\n";
        renderer->sendError("Invalid JSON message.");
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] handleFrontendMessage exception: " << e.what() << "\n";
        renderer->sendError(e.what());
    }
}





std::shared_ptr<JTML::Environment> Interpreter::getCurrentEnvironment() const {
    return currentEnv;
}


void Interpreter::interpret(const JtmlElementNode& root) {
    // Recursively process the JtmlElementNode
    std::cout << "Interpreting element: " << root.tagName << "\n";

    // Process attributes
    for (const auto& attr : root.attributes) {
        std::cout << "  Attribute: " << attr.key << " = " << attr.value->toString() << "\n";
    }

    // Process child nodes
    for (const auto& child : root.content) {
        interpretNode(*child);
    }
}

// Interpret a single AST node (e.g., JtmlElementNode)
void Interpreter::interpret(const ASTNode& node) {
    interpretNode(node);
}

// Interpret a vector of AST nodes (e.g., the entire program)
void Interpreter::interpret(const std::vector<std::unique_ptr<ASTNode>>& program) {
    for (const auto& node : program) {
            std::cout << " " << node->toString() << "\n";
            interpretNode(*node);
        
    }
    // After all statements in a block => recalc
    recalcDirty(globalEnv);
}

// Interpret directly from code (string)
void Interpreter::interpret(const std::string& code) {
    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();

        // Check for lexer errors
        const auto& lexErrors = lexer.getErrors();
        if (!lexErrors.empty()) {
            for (const auto& err : lexErrors) {
                handleError(err);
            }
            return;
        }

        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();

        // Optionally, check for parser errors if your Parser provides such functionality

        interpret(program);
    }
    catch (const std::exception& e) {
        handleError(std::string("Interpretation error: ") + e.what());
    }
}

// Interpret a single AST node by delegating to specific methods
void Interpreter::interpretNode(const ASTNode& node) {
    std::cout << "Interpreting node " << node.toString() << "\n";
    try {
        switch (node.getType()) {
            case ASTNodeType::JtmlElement:
                interpretElement(static_cast<const JtmlElementNode&>(node));
                break;
            case ASTNodeType::BlockStatement:
                interpretBlockStatement(static_cast<const BlockStatementNode&>(node));
                break;
            case ASTNodeType::ShowStatement:
                interpretShow(static_cast<const ShowStatementNode&>(node));
                break;
            case ASTNodeType::DefineStatement:
                interpretDefine(static_cast<const DefineStatementNode&>(node));
                break;
            case ASTNodeType::AssignmentStatement:
                interpretAssignment(static_cast<const AssignmentStatementNode&>(node));
                break;
            case ASTNodeType::ExpressionStatement:
                interpretExpression(static_cast<const ExpressionNode&>(node));
                break;
            case ASTNodeType::DeriveStatement:
                interpretDerive(static_cast<const DeriveStatementNode&>(node));
                break;
            case ASTNodeType::UnbindStatement:
                interpretUnbind(static_cast<const UnbindStatementNode&>(node));
                break;
            case ASTNodeType::StoreStatement:
                interpretStore(static_cast<const StoreStatementNode&>(node));
                break;
            case ASTNodeType::IfStatement:
                interpretIf(static_cast<const IfStatementNode&>(node));
                break;
            case ASTNodeType::WhileStatement:
                interpretWhile(static_cast<const WhileStatementNode&>(node));
                break;
            case ASTNodeType::BreakStatement:
                interpretBreak(static_cast<const BreakStatementNode&>(node));
                break;
            case ASTNodeType::ContinueStatement:
                interpretContinue(static_cast<const ContinueStatementNode&>(node));
                break;
            case ASTNodeType::ForStatement:
                interpretFor(static_cast<const ForStatementNode&>(node));
                break;
            case ASTNodeType::TryExceptThen:
                interpretTryExceptThen(static_cast<const TryExceptThenNode&>(node));
                break;
            case ASTNodeType::ThrowStatement:
                interpretThrow(static_cast<const ThrowStatementNode&>(node));
                break;
            case ASTNodeType::SubscribeStatement:
                interpretSubscribe(static_cast<const SubscribeStatementNode&>(node));
                break;
            case ASTNodeType::UnsubscribeStatement:
                interpretUnsubscribe(static_cast<const UnsubscribeStatementNode&>(node));
                break;    
            case ASTNodeType::FunctionDeclaration:
                interpretFunctionDeclaration(static_cast<const FunctionDeclarationNode&>(node));
                break;
            case ASTNodeType::ClassDeclaration:
                interpretClassDeclaration(static_cast<const ClassDeclarationNode&>(node));
                break;
            case ASTNodeType::ReturnStatement:
                std::cout << "ReturnStatement node" << node.toString();
                interpretReturn(static_cast<const ReturnStatementNode&>(node));
                break;
            case ASTNodeType::NoOp:
                break;
            default:
                handleError("Unknown ASTNodeType encountered during interpretation.");
        }
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const BreakException&) {
        throw;
    } catch (const ContinueException&) {
        throw;
    } catch (const std::exception& e) {
        handleError("Node Interpretation Error: " + std::string(e.what()));
    }
}

// ------------------- Interpretation Methods -------------------

void Interpreter::interpretElement(const JtmlElementNode& elem) {
    std::cout << "[DEBUG] Interpreting Element: <" << elem.tagName << ">\n";
    nodeID++;
    // Create a new environment for this element
  

    // Evaluate attributes or do any binding registrations (transpile-time binding approach)

    interpretElementAttributes(elem);

    // Now interpret child statements: only show, if, for, while, or nested element
    for (auto& child : elem.content) {
        switch(child->getType()) {
            case ASTNodeType::ShowStatement:
                interpretShowElement(static_cast<const ShowStatementNode&>(*child));
                break;
            case ASTNodeType::IfStatement:
                interpretIfElement(static_cast<const IfStatementNode&>(*child));
                break;
            case ASTNodeType::ForStatement:
                interpretForElement(static_cast<const ForStatementNode&>(*child));
                break;
            case ASTNodeType::WhileStatement:
                interpretWhileElement(static_cast<const WhileStatementNode&>(*child));
                break;
            case ASTNodeType::JtmlElement:
                interpretElement(static_cast<const JtmlElementNode&>(*child));
                break;
            default:
                // not allowed inside an element
                std::cerr << "[ERROR] Disallowed statement '"
                          << child->toString()
                          << "' inside <" << elem.tagName << ">\n";
                break;
        }
    }

   
    std::cout << "[DEBUG] Exiting Element <" << elem.tagName << ">\n";
}
// In jtml_interpreter.cpp

void Interpreter::interpretElementAttributes(const JtmlElementNode& elem) {
    // Iterate through all attributes of the element
    for (const auto& attr : elem.attributes) {
        
        const std::string& attrName = attr.key;
        if (attrName  == "onClick" || attrName == "onInput" || attrName == "onMouseOver") {
            uniqueVarID++;
            const std::unique_ptr<ExpressionStatementNode>& attrValue = attr.value;
            std::string derivedVarName = "attr_" + std::to_string(uniqueVarID);
        
            JTML::CompositeKey attrKey{ globalEnv->instanceID, derivedVarName };
                    // Create a BindingInfo for the attribute
            JTML::BindingInfo binding;
            binding.varName = attrKey;
            binding.elementId = "attr_" + std::to_string(uniqueVarID);
            binding.bindingType = "attribute_event";
            binding.attribute = attrName;
            binding.expression = attrValue->clone();

            // Register the binding
            globalEnv->registerBinding(binding); 

        } else {
            uniqueVarID++;
            const std::unique_ptr<ExpressionStatementNode>& attrValue = attr.value;
            // Register attribute bindings similar to content bindings
            std::string derivedVarName = "attr_" + std::to_string(uniqueVarID);
            
            JTML::CompositeKey attrKey{ globalEnv->instanceID, derivedVarName };

            // Gather dependencies
            std::vector<JTML::CompositeKey> deps;
            gatherDeps(attrValue.get(), deps, globalEnv);

            // Clone the expression
            std::unique_ptr<ExpressionStatementNode> clonedExpr = attrValue->clone();

            // Define an evaluator lambda
            auto evaluator = [this](const ExpressionStatementNode* expr) -> std::shared_ptr<JTML::VarValue> {
                return evaluateExpression(expr, this->globalEnv);
            };

            // Derive the variable
            globalEnv->deriveVariable(attrKey, std::move(clonedExpr), deps, evaluator);

            // Create a BindingInfo for the attribute
            JTML::BindingInfo binding;
            binding.varName = attrKey;
            binding.elementId = "attr_" + std::to_string(uniqueVarID);
            binding.bindingType = "attribute";
            binding.attribute = attrName;
            binding.expression = attrValue->clone();

            // Register the binding
            globalEnv->registerBinding(binding);  
        }
    }
}

// Helper function to determine if an attribute is an event handler
bool Interpreter::isEventAttribute(const std::string& attrName) const {
    return attrName.rfind("on", 0) == 0 && attrName.size() > 2; // Starts with "on"
}

// Helper function to extract event type from attribute name
std::string Interpreter::extractEventType(const std::string& attrName) const {
    if (attrName.size() > 2) {
        return "on" + attrName.substr(2); // Keep the "on" prefix
    }
    return attrName;
}

// Helper function to check if an expression contains dynamic bindings
bool Interpreter::containsExpression(const ExpressionStatementNode* exprNode) const {
    // Implement logic to determine if the expression contains variables or operations
    // that require dynamic binding
    // For simplicity, assume all non-literal expressions require binding
    if (!exprNode) return false;
    switch (exprNode->getExprType()) {
        case ExpressionStatementNodeType::Variable:
        case ExpressionStatementNodeType::Binary:
        case ExpressionStatementNodeType::Unary:
        case ExpressionStatementNodeType::FunctionCall:
        case ExpressionStatementNodeType::Subscript:
            return true;
        default:
            return false;
    }
}


void Interpreter::interpretShowElement(const ShowStatementNode& node) {
    nodeID++;
    uniqueVarID++;
    auto itNode = transpiler.nodeDerivedMap.find(nodeID);
    if (itNode == transpiler.nodeDerivedMap.end()) {
        std::cerr << "[WARN] No derived var for Show nodeID=" 
                  << nodeID << "\n";
        return;
    }
    auto itVar = itNode->second.find("show");
    if (itVar == itNode->second.end()) {
        // fallback ephemeral
        std::cerr << "[WARN] Misplaced var for Show nodeID=" 
                  << nodeID << "\n";
        return;
    }
    std::string derivedName = itVar->second;
    std::vector<JTML::CompositeKey> deps;
    gatherDeps(node.expr.get(), deps, globalEnv);

    auto cloned = node.expr->clone();
    auto eval   = [this](const ExpressionStatementNode* expr){
        return evaluateExpression(expr, globalEnv);
    };

    JTML::CompositeKey showKey{ globalEnv->instanceID, derivedName };
    globalEnv->deriveVariable(showKey, std::move(cloned), deps, eval);

    // binding
    JTML::BindingInfo b;
    b.varName     = showKey;
    b.elementId = "expr_" + std::to_string(uniqueVarID);
    b.bindingType = "content"; // e.g. content or "show"
    b.expression = node.expr->clone();
    globalEnv->registerBinding(b);

    // interpret once => log or similar
    auto val = globalEnv->getVariable(showKey);
    std::cout << "[SHOW] " << derivedName << " => "
              << (val ? val->toString() : "(null)") << "\n";
}


void Interpreter::interpretWhileElement(const WhileStatementNode& node) {
    nodeID++;
    uniqueVarID++;
    auto itNode = transpiler.nodeDerivedMap.find(nodeID);
    if (itNode == transpiler.nodeDerivedMap.end()) {
        std::cerr << "[WARN] No entry in transpiler.nodeDerivedMap for While nodeID=" 
                  << nodeID << "\n";
        return;
    }
    auto itVar = itNode->second.find("while");
    if (itVar == itNode->second.end()) {
        // fallback ephemeral
        std::cerr << "[WARN] Misplaced var for While nodeID=" 
                << nodeID << "\n";
        return;
    }
    std::string derivedName = itVar->second;
    // gather deps, define variable
    std::vector<JTML::CompositeKey> deps;
    gatherDeps(node.condition.get(), deps, globalEnv);

    auto cloned = node.condition->clone();
    auto eval = [this](const ExpressionStatementNode* expr){
        return evaluateExpression(expr, globalEnv);
    };

    JTML::CompositeKey condKey{ globalEnv->instanceID, derivedName };
    globalEnv->deriveVariable(condKey, std::move(cloned), deps, eval);

    // binding
    JTML::BindingInfo b;
    b.varName     = condKey;
    b.elementId = "cond_" + std::to_string(uniqueVarID);
    b.bindingType = "while";
    b.expression = node.condition->clone();
    globalEnv->registerBinding(b);

    // server side while loop if you want
    while(true) {
        bool condVal = isTruthy(globalEnv->getVariable(condKey)->toString());
        if(!condVal) break;

        try {
            for (auto& stmt : node.body) {
                interpretNode(*stmt);
            }
        } catch (const BreakException&) {
            break;
        } catch (const ContinueException&) {
            continue;
        }
        recalcDirty(globalEnv);
    }
}

void Interpreter::interpretIfElement(const IfStatementNode& node) {
    nodeID++;
    uniqueVarID++;
    auto itNode = transpiler.nodeDerivedMap.find(nodeID);
    if (itNode == transpiler.nodeDerivedMap.end()) {
        std::cerr << "[WARN] No entry in transpiler.nodeDerivedMap for If nodeID=" 
                  << nodeID << "\n";
        return;
    }
    auto itVar = itNode->second.find("if");
    if (itVar == itNode->second.end()) {
        // fallback ephemeral
        std::cerr << "[WARN] Misplaced var for If nodeID=" 
                << nodeID << "\n";
        return;
    }
    std::string derivedName = itVar->second;
    std::vector<JTML::CompositeKey> deps;
    gatherDeps(node.condition.get(), deps, globalEnv);

    // define the derived variable
    auto clonedExpr = node.condition->clone();
    auto evaluator  = [this](const ExpressionStatementNode* expr){
        return evaluateExpression(expr, globalEnv);
    };

    JTML::CompositeKey condKey{ globalEnv->instanceID, derivedName };
    globalEnv->deriveVariable(condKey, std::move(clonedExpr), deps, evaluator);

    // register an "if" binding
    JTML::BindingInfo bind;
    bind.varName = condKey;
    bind.elementId = "cond_" + std::to_string(uniqueVarID);
    bind.bindingType = "if";
    bind.expression = node.condition->clone();
    globalEnv->registerBinding(bind);

    // interpret THEN/ELSE once if you want
    bool condResult = isTruthy(globalEnv->getVariable(condKey)->toString());
    if (condResult) {
        for (auto& stmt : node.thenStatements) {
            interpretNode(*stmt);
        }
    } else {
        for (auto& stmt : node.elseStatements) {
            interpretNode(*stmt);
        }
    }
}


void Interpreter::interpretForElement(const ForStatementNode& node) {
    nodeID++;
    uniqueVarID++;
    auto forNode = transpiler.nodeDerivedMap.find(nodeID);
    if (forNode == transpiler.nodeDerivedMap.end()) {
        std::cerr << "[WARN] No entry in transpiler.nodeDerivedMap for For nodeID=" 
                  << nodeID << "\n";
        return;
    }
    auto forVar = forNode->second.find("for");
    if (forVar == forNode->second.end()) {
        // fallback ephemeral
        std::cerr << "[WARN] Misplaced var for For nodeID=" 
                << nodeID << "\n";
        return;
    }
    std::string derivedName = forVar->second;
    std::vector<JTML::CompositeKey> deps;
    gatherDeps(node.iterableExpression.get(), deps, globalEnv);

    // define the derived variable
    auto clonedExpr = node.iterableExpression->clone();
    auto evaluator  = [this](const ExpressionStatementNode* expr){
        return evaluateExpression(expr, globalEnv);
    };

    JTML::CompositeKey condKey{ globalEnv->instanceID, derivedName };
    globalEnv->deriveVariable(condKey, std::move(clonedExpr), deps, evaluator);

    // register an "if" binding
    JTML::BindingInfo bind;
    bind.varName = condKey;
    bind.bindingType = "for";
    bind.elementId = "range_" + std::to_string(uniqueVarID);
    bind.expression = node.iterableExpression->clone();
    globalEnv->registerBinding(bind);

        // 7) (Optional) Actually interpret the loop on the server side
    //    For example:
    try {
        // Evaluate the iterable expression => array, string, or numeric range
        auto iterableVal = evaluateExpression(node.iterableExpression.get(), globalEnv);

        if (!node.rangeEndExpr) {
            // “for (i in array_or_string)”
            if (iterableVal->isArray()) {
                const auto& arr = iterableVal->getArray();
                for (size_t idx = 0; idx < arr->size(); ++idx) {
                    // Bind the iterator variable
                    JTML::CompositeKey iterKey = { globalEnv->instanceID, node.iteratorName };
                    globalEnv->setVariable(iterKey, (*arr)[idx]);

                    try {
                        // interpret each statement in the loop body
                        for (const auto& stmt : node.body) {
                            interpretNode(*stmt);
                        }
                    }
                    catch (const BreakException&) {
                        break;
                    }
                    catch (const ContinueException&) {
                        continue;
                    }
                    recalcDirty(globalEnv);
                }
            }
            else if (iterableVal->isString()) {
                const std::string& s = iterableVal->getString();
                for (char c : s) {
                    auto cVal = std::make_shared<JTML::VarValue>(std::string(1, c));
                    JTML::CompositeKey iterKey = { globalEnv->instanceID, node.iteratorName };
                    globalEnv->setVariable(iterKey, cVal);

                    try {
                        for (const auto& stmt : node.body) {
                            interpretNode(*stmt);
                        }
                    }
                    catch (const BreakException&) {
                        break;
                    }
                    catch (const ContinueException&) {
                        continue;
                    }
                    recalcDirty(globalEnv);
                }
            }
            else {
                throw std::runtime_error("For loop: not array, not string, no rangeEndExpr => invalid iterable.");
            }
        }
        else {
            // “for (i in X..Y)”
            std::shared_ptr<JTML::VarValue> endVal =
                evaluateExpression(node.rangeEndExpr.get(), globalEnv);

            double startNum = getNumericValue(iterableVal);
            double endNum   = getNumericValue(endVal);

            int startI = static_cast<int>(startNum);
            int endI   = static_cast<int>(endNum);

            for (int i = startI; i <= endI; i++) {
                auto iVal = std::make_shared<JTML::VarValue>(static_cast<double>(i));
                JTML::CompositeKey iterKey = { globalEnv->instanceID, node.iteratorName };
                globalEnv->setVariable(iterKey, iVal);

                try {
                    for (const auto& stmt : node.body) {
                        interpretNode(*stmt);
                    }
                }
                catch (const BreakException&) {
                    break;
                }
                catch (const ContinueException&) {
                    continue;
                }
                recalcDirty(globalEnv);
            }
        }
    }
    catch (const ReturnException&) {
        // Let ReturnException bubble up for function returns
        throw;
    }
    catch (const std::exception& e) {
        handleError("For Loop Interpretation Error (server side iteration): " + std::string(e.what()));
    }

}



void Interpreter::interpretBlockStatement(const BlockStatementNode& block) {
    std::cout << "[DEBUG] Entering BlockStatement with " 
              << block.statements.size() << " statements.\n";
    
    auto previousEnv = currentEnv;
    currentEnv = std::make_shared<JTML::Environment>(previousEnv);

    try {
        for (const auto& stmt : block.statements) {
            std::cout << "Interpreting node " << stmt->toString() << "\n";
            interpretNode(*stmt);
        }
    } catch (...) {
        // Ensure environment restoration even on exception
        currentEnv = previousEnv;
        throw;
    }

    currentEnv = previousEnv;

    std::cout << "[DEBUG] Exiting BlockStatement.\n";
}

void Interpreter::interpretShow(const ShowStatementNode& stmt) {
    if (!stmt.expr) {
        std::cout << "[SHOW] (empty?)\n";
        return;
    }
    // Evaluate the expression => store it in a new server var like expr_X

    // Evaluate
    auto val = evaluateExpression(stmt.expr.get(), currentEnv);

    std::cout << "[SHOW] " <<  val->toString() << "\n";
    // Whenever val changes, the environment can push updateBinding messages
}

void Interpreter::interpretExpression(const ExpressionNode& node) {
    // Evaluate the expression contained in the node
    auto result = evaluateExpression(node.expression.get(), currentEnv);

    // Optionally, handle side effects or log the result
    std::cout << "[DEBUG] Evaluated expression: " << result->toString() << "\n";
}

void Interpreter::interpretDefine(const DefineStatementNode& stmt) {
    try {
        std::shared_ptr<JTML::VarValue> valPtr = evaluateExpression(stmt.expression.get(), currentEnv);
        JTML::CompositeKey varKey = { currentEnv->instanceID, stmt.identifier };
        
        // 3. Set the variable in the global environment using JTML::CompositeKey
        currentEnv->setVariable(varKey, valPtr);

        // Only mark dirty and recalculate if in the global or instance scope
        if (currentEnv->isGlobalEnvironment()) {
            currentEnv->recalcDirty([this](JTML::VarID varID) {
                updateVariable(varID, currentEnv);
            });
        }

        // Enhanced Logging: Separate value and type information
        std::cout << "[DEFINE] " << currentEnv->getCompositeName(varKey) << " = " << valPtr->toString();
        if (valPtr->isNumber()) {
            std::cout << " (Number)";
        }
        else if (valPtr->isString()) {
            std::cout << " (String)";
        }
        else if (valPtr->isBool()) {
            std::cout << " (Boolean)";
        }
        else if (valPtr->isArray()) {
            std::cout << " (Array)";
        }
        else if (valPtr->isDict()) {
            std::cout << " (Dictionary)";
        }
        std::cout << "\n";
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        handleError(std::string("Define Statement Error: ") + e.what());
    } 
}


void Interpreter::interpretAssignment(const AssignmentStatementNode& stmt) {
    // 1) Evaluate the RHS
    auto newVal = evaluateExpression(stmt.rhs.get(), currentEnv);
    std::cout << " (Assignment RHS: " << newVal->toString() << "\n";

    // 2) Evaluate the LHS (determine its type and handle accordingly)
    switch (stmt.lhs->getExprType()) {
        case ExpressionStatementNodeType::Variable: {
            // Handle simple variable assignment
            const auto& varNode = static_cast<const VariableExpressionStatementNode&>(*stmt.lhs);
            JTML::CompositeKey varKey = { currentEnv->instanceID, varNode.name };
            currentEnv->setVariable(varKey, newVal);
            break;
        }

        case ExpressionStatementNodeType::ObjectPropertyAccess: {
            // Handle property access assignment (e.g., obj.prop)
            const auto& propNode = static_cast<const ObjectPropertyAccessExpressionNode&>(*stmt.lhs);
            auto baseVal = evaluateExpression(propNode.base.get(), currentEnv);

            if (!baseVal->isObject()) {
                throw std::runtime_error("Cannot assign to non-object property: " + propNode.propertyName);
            }

            auto& objHandle = baseVal->getObjectHandle();
            JTML::CompositeKey propKey = { objHandle.instanceEnv->instanceID, propNode.propertyName };
            
            // Set the property using JTML::CompositeKey
            objHandle.instanceEnv->setVariable(propKey, newVal);
            break;
        }

        case ExpressionStatementNodeType::Subscript: {
            // Handle subscript assignment (e.g., arr[idx] or dict[key])
            const auto& subNode = static_cast<const SubscriptExpressionStatementNode&>(*stmt.lhs);
            auto baseVal = evaluateExpression(subNode.base.get(), currentEnv);
            auto indexVal = evaluateExpression(subNode.index.get(), currentEnv);

            if (baseVal->isArray()) {
                auto reactiveArray = baseVal->getArray();
                double idxNum = getNumericValue(indexVal);
                int idx = static_cast<int>(idxNum);

                if (idx < 0 || static_cast<size_t>(idx) >= reactiveArray->size()) {
                    throw std::runtime_error("Array index out of range: " + std::to_string(idx));
                }

                reactiveArray->set(idx, newVal);
            }
            else if (baseVal->isDict()) {
                auto reactiveDict = baseVal->getDict();
                std::string key = getStringValue(indexVal);
                reactiveDict->set(key, newVal);
            }
            else {
                throw std::runtime_error("Cannot subscript-assign to non-array/non-dict type");
            }
            break;
        }

        default:
            throw std::runtime_error("Invalid left-hand side expression in assignment");
    }

    // 3) Recalculate dirty variables, log the assignment, etc.
    std::cout << "[ASSIGN] LHS=";
    switch (stmt.lhs->getExprType()) {
        case ExpressionStatementNodeType::Variable: {
            const auto& varNode = static_cast<const VariableExpressionStatementNode&>(*stmt.lhs);
            JTML::CompositeKey varKey = { currentEnv->instanceID, varNode.name };
            std::cout << varKey.varName << " (InstanceID: " << varKey.instanceID << ")";
            break;
        }
        case ExpressionStatementNodeType::ObjectPropertyAccess: {
            const auto& propNode = static_cast<const ObjectPropertyAccessExpressionNode&>(*stmt.lhs);
            JTML::CompositeKey propKey = { currentEnv->instanceID, propNode.propertyName };
            std::cout << propKey.varName << " (InstanceID: " << propKey.instanceID << ")";
            break;
        }
        case ExpressionStatementNodeType::Subscript: {
            const auto& subNode = static_cast<const SubscriptExpressionStatementNode&>(*stmt.lhs);
            std::cout << subNode.toString();
            break;
        }
        default:
            std::cout << "Unknown LHS";
            break;
    }
    std::cout << " => RHS=" << newVal->toString() << "\n";
    currentEnv->recalcDirty([this](JTML::VarID varID) { 
                        updateVariable(varID, currentEnv); 
                    });

}


void Interpreter::interpretClassDeclaration(const ClassDeclarationNode& node) {
    if (classDeclarations.find(node.name) != classDeclarations.end()) {
        throw std::runtime_error("Class already defined: " + node.name);
    }

    std::cout << "[DEBUG] Interpreting ClassDeclarationNode: " << node.toString() << "\n";

    // Move the members to the new class declaration
    auto membersCopy = std::vector<std::unique_ptr<ASTNode>>{};
    for (const auto& member : node.members) {
        membersCopy.push_back(member->clone()); // Assuming `ASTNode` has a `clone` method
    }

    classDeclarations[node.name] = std::make_shared<ClassDeclarationNode>(
        node.name, node.parentName, std::move(membersCopy)
    );

    std::cout << "Class '" << node.name << "' defined.\n";
}
 void Interpreter::interpretDerive(const DeriveStatementNode& stmt) { 
        try {
            // Ensure currentEnv is valid
            if (!currentEnv) {
                throw std::runtime_error("No active environment for derive statement.");
            }

            // Retrieve the current InstanceID from the environment
            JTML::InstanceID currentID = currentEnv->instanceID;

            // Gather dependencies from the expression
            std::vector<JTML::CompositeKey> deps;
            gatherDeps(stmt.expression.get(), deps, currentEnv);

            // Clone the expression to store in the derived variable
            std::unique_ptr<ExpressionStatementNode> newExpr = stmt.expression->clone();

            // Define a lambda to evaluate expressions using the Interpreter's evaluateExpression
            auto evaluator = [this](const ExpressionStatementNode* exprNode) -> std::shared_ptr<JTML::VarValue> {
                return this->evaluateExpression(exprNode, this->currentEnv);
            };

            // Create JTML::CompositeKey for the variable
            JTML::CompositeKey key = { currentID, stmt.identifier };

            std::cout << "[DEBUG] About to rum derive on " << currentEnv->getCompositeName(key) << " derived from dependencies: ";
            for (const auto& dep : deps) {
                std::cout << currentEnv->getCompositeName(dep) << " ";
            }
            std::cout << "\n";

            // Define or update the derived variable by calling Environment's method
            currentEnv->deriveVariable(key, std::move(newExpr), std::move(deps), evaluator);

            std::cout << "[DERIVE] " << currentEnv->getCompositeName(key) << " derived from dependencies: ";
            for (const auto& dep : deps) {
                std::cout << dep.varName << " ";
            }
            std::cout << "\n";

        } catch (const std::exception& e) {
            handleError("Derive Statement Error: " + std::string(e.what()));
        } 
    }

    // Adapted interpretUnbind
void Interpreter::interpretUnbind(const UnbindStatementNode& stmt) {
    try {
        // Ensure currentEnv is valid
        if (!currentEnv) {
            throw std::runtime_error("No active environment for unbind statement.");
        }

        // Retrieve the current InstanceID from the environment
        JTML::InstanceID currentID = currentEnv->instanceID;

        // Create JTML::CompositeKey for the variable
        JTML::CompositeKey key = { currentID, stmt.identifier };

        // Unbind the variable by calling Environment's method
        currentEnv->unbindVariable(key);

        // Logging
        std::cout << "[UNBIND] " << key.varName << " has been unbound from environment.\n";
    } catch (const std::exception& e) {
        handleError("Unbind Statement Error: " + std::string(e.what()));
    }
}

void Interpreter::interpretStore(const StoreStatementNode& stmt) {
    try {
        storeVariable(stmt.targetScope, stmt.variableName);
        std::cout << "[STORE] " << stmt.variableName << " => Scope: " << stmt.targetScope << "\n";
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        handleError(std::string("Store Statement Error: ") + e.what());
    } 
}

void Interpreter::interpretIf(const IfStatementNode& node) {
    try {
        bool conditionResult = evaluateCondition(node.condition.get(), currentEnv);
        std::cout << "[IF] Condition evaluated to: " << (conditionResult ? "true" : "false") << "\n";
        if (conditionResult) {
            // "Truthy" condition
            for (const auto& stmt : node.thenStatements) {
                interpretNode(*stmt);
            }
        } else if (!node.elseStatements.empty()) {
            // "Falsey" condition with an else block
            for (const auto& stmt : node.elseStatements) {
                interpretNode(*stmt);
            }
        }
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const ContinueException&) {
        throw;
    } catch (const BreakException&) {
        throw;
    } catch (const std::exception& e) {
        handleError("If Statement Error: " + std::string(e.what()));
    } 
}

void Interpreter::interpretWhile(const WhileStatementNode& node) {
    try {
        while (true) {
            if (!evaluateCondition(node.condition.get(), currentEnv)) {
                // Condition is "falsey" => break
                break;
            }

            // Interpret the loop body
            try {
                for (const auto& stmt : node.body) {
                    interpretNode(*stmt);
                }
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            }

            // Recalculate derived variables if required
            recalcDirty(currentEnv);
        }
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        handleError("While Interpretation Error: " + std::string(e.what()));
    }
}


void Interpreter::interpretFor(const ForStatementNode& node) {
    // Determine the current environment
    std::shared_ptr<JTML::Environment> env = currentEnv; // Assuming currentEnv is a member variable

    try {
        // Evaluate the iterable expression => returns a VarValue
        auto iterableVal = evaluateExpression(node.iterableExpression.get(), env);

        if (!node.rangeEndExpr) {
            // "for (i in someCollection)" logic
            if (iterableVal->isArray()) {
                const auto& arr = iterableVal->getArray();
                for (size_t idx = 0; idx < arr->size(); ++idx) {
                    // Construct JTML::CompositeKey for the iterator variable
                    JTML::CompositeKey varKey = { env->instanceID, node.iteratorName };

                    // Set the iterator variable in the current environment
                    env->setVariable(varKey, (*arr)[idx]);

                    try {
                        // Interpret each statement in the loop body
                        for (const auto& stmt : node.body) {
                            interpretNode(*stmt);
                        }
                    } catch (const BreakException&) {
                        break; // Exit the loop
                    } catch (const ContinueException&) {
                        continue; // Proceed to the next iteration
                    }

                    // Recalculate dirty variables if necessary
                    env->recalcDirty([this](JTML::VarID varID) { 
                        updateVariable(varID, currentEnv); 
                    });
                }
            }
            else if (iterableVal->isString()) {
                // For each character in the string
                const auto& s = iterableVal->getString();
                for (char c : s) {
                    // Create a VarValue for the current character
                    auto cVal = std::make_shared<JTML::VarValue>(std::string(1, c));

                    // Construct JTML::CompositeKey for the iterator variable
                    JTML::CompositeKey varKey = { env->instanceID, node.iteratorName };

                    // Set the iterator variable in the current environment
                    env->setVariable(varKey, cVal);

                    try {
                        // Interpret each statement in the loop body
                        for (const auto& stmt : node.body) {
                            interpretNode(*stmt);
                        }
                    } catch (const BreakException&) {
                        break; // Exit the loop
                    } catch (const ContinueException&) {
                        continue; // Proceed to the next iteration
                    }

                    // Recalculate dirty variables if necessary
                    env->recalcDirty([this](JTML::VarID varID) { 
                        updateVariable(varID, currentEnv); 
                    });
                }
            }
            else {
                throw std::runtime_error("Iterable in 'for' loop is neither an array nor a string.");
            }
        }
        else {
            // "for (i in X..Y)" logic
            auto endVal = evaluateExpression(node.rangeEndExpr.get(), env);
            double startNum = getNumericValue(iterableVal);
            double endNum   = getNumericValue(endVal);
            int startI = static_cast<int>(startNum);
            int endI   = static_cast<int>(endNum);

            for (int i = startI; i <= endI; i++) {
                // Create a VarValue for the current index
                auto iVal = std::make_shared<JTML::VarValue>(static_cast<double>(i));

                // Construct JTML::CompositeKey for the iterator variable
                JTML::CompositeKey varKey = { env->instanceID, node.iteratorName };

                // Set the iterator variable in the current environment
                env->setVariable(varKey, iVal);

                try {
                    // Interpret each statement in the loop body
                    for (const auto& stmt : node.body) {
                        interpretNode(*stmt);
                    }
                } catch (const BreakException&) {
                    break; // Exit the loop
                } catch (const ContinueException&) {
                    continue; // Proceed to the next iteration
                }

                // Recalculate dirty variables if necessary
                env->recalcDirty([this](JTML::VarID varID) { 
                    updateVariable(varID, currentEnv); 
                });
            }
        }
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        handleError("For Loop Interpretation Error: " + std::string(e.what()));
    }
}

void Interpreter::interpretBreak(const BreakStatementNode& node) {
    throw BreakException();
}

void Interpreter::interpretContinue(const ContinueStatementNode& node) {
    throw ContinueException();
}
void Interpreter::interpretTryExceptThen(const TryExceptThenNode& node) {
    std::shared_ptr<JTML::Environment> env = currentEnv; // Assuming currentEnv is a member variable
    bool errorOccurred = false;
    std::string errorMessage;

    // 1) Try block
    try {
        interpret(node.tryBlock);
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        errorOccurred = true;
        errorMessage = e.what();
    } 

    // 2) Except block
    if (errorOccurred && node.hasCatch) {
        // If there's a catch variable like 'except err'
        if (!node.catchIdentifier.empty()) {
            // Store the error message in environment using JTML::CompositeKey
            JTML::CompositeKey errKey = { env->instanceID, node.catchIdentifier };
            auto errVal = std::make_shared<JTML::VarValue>(errorMessage);
            env->setVariable(errKey, errVal);
        }
        try {
            interpret(node.catchBlock);
        } catch (const ReturnException&) {
            // Allow ReturnException to propagate
            throw;
        } catch (const std::exception& e) {
            handleError("Try-Except Block Interpretation Error: " + std::string(e.what()));
        }
    }

    // 3) Then (finally) block
    if (node.hasFinally) {
        try {
            interpret(node.finallyBlock);
        } catch (const ReturnException&) {
            // Allow ReturnException to propagate
            throw;
        } catch (const std::exception& e) {
            handleError("Finally Block Interpretation Error: " + std::string(e.what()));
        }
    } else if (errorOccurred && !node.hasCatch) {
        // Rethrow if we had an error but no catch
        throw std::runtime_error(errorMessage);
    }
}
void Interpreter::interpretThrow(const ThrowStatementNode& node) {
    try {
        std::string msg;
        if (node.expression) {
            msg = getStringValue(evaluateExpression(node.expression.get(), currentEnv));
        } else {
            msg = "Unspecified error";
        }
        // Throw it as a runtime_error
        throw std::runtime_error(msg);
    } catch (const ReturnException&) {
        // Allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        handleError(std::string("Throw Statement Error: ") + e.what());
    } 
}

void Interpreter::interpretReturn(const ReturnStatementNode& node) {
    std::cout << "[DEBUG] Interpreting ReturnStatementNode: " << node.toString() << "\n";

    if (!inFunctionContext) {
        handleError("Return statement outside function context");
        return;
    }

    try {
        // Initialize the return value
        std::shared_ptr<JTML::VarValue> returnValue;

        // Evaluate the return expression if it exists
        if (node.expr) {
            std::cout << "[DEBUG] node.expr: " << node.expr->toString() << "\n";
            returnValue = evaluateExpression(node.expr.get(), currentEnv);
        } else {
            // Default return value: an empty string
            static const std::shared_ptr<JTML::VarValue> defaultReturnValue = 
                std::make_shared<JTML::VarValue>(JTML::ValueVariant{""});
            returnValue = defaultReturnValue;
        }

        // Log the return value
        std::cout << "[RETURN] " << (returnValue ? returnValue->toString() : "void") << "\n";
        
        // Propagate the return value through a ReturnException
        throw ReturnException(returnValue);

    } catch (const ReturnException&) {
        // Explicitly allow ReturnException to propagate
        throw;
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Function is not following ReturnException path! "<< "\n";
        handleError("Return Statement Error: " + std::string(e.what()));
    }
}

// jtml_interpreter.cpp

void Interpreter::interpretSubscribe(const SubscribeStatementNode& node) {
    try {
        // Get the function to be subscribed
        JTML::CompositeKey funcKey = { currentEnv->instanceID, node.functionName };
        std::shared_ptr<JTML::Function> func = currentEnv->getFunction(funcKey);
        if (!func) {
            throw std::runtime_error("Function '" + node.functionName + "' not defined.");
        }

        // Get the VarID of the variable to subscribe to
        JTML::CompositeKey key = { currentEnv->instanceID, node.variableName };

        // Get the VarID of the variable to subscribe to using JTML::CompositeKey
        JTML::VarID varID = currentEnv->getVarID(key);

        // Check if the variable is an array or dict
        auto varValue = currentEnv->getVariable(key);
        if (!varValue) {
            throw std::runtime_error("Variable '" + key.varName + "' not found for subscription.");
        }

        std::function<void()> callback;

        if (varValue->isArray()) {
            auto reactiveArray = varValue->getArray();
            callback = [this, reactiveArray, func, key]() {
                try {
                    // Iterate over the array and pass elements as arguments
                    std::vector<std::shared_ptr<JTML::VarValue>> args;
                    for (const auto& elem : reactiveArray->getArrayData()) {
                        args.push_back(elem);
                    }
                    executeFunction(func, args, nullptr);
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Callback execution failed for function '"
                              << func->name << "': " << e.what() << "\n";
                }
            };
        }
        else if (varValue->isDict()) {
            auto reactiveDict = varValue->getDict();
            callback = [this, reactiveDict, func, key]() {
                try {
                    // Iterate over the dict and pass key-value pairs as arguments
                    std::vector<std::shared_ptr<JTML::VarValue>> args;
                    for (const auto& [k, v] : reactiveDict->getDictData()) {
                        // Optionally, create a struct or tuple to hold key-value
                        // For simplicity, we'll pass the value
                        args.push_back(v);
                    }
                    executeFunction(func, args, nullptr);
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Callback execution failed for function '"
                              << func->name << "': " << e.what() << "\n";
                }
            };
        }
        else {
            // Handle other types if necessary
            throw std::runtime_error("Variable '" + key.varName + "' is not an array or dict for subscription.");
        }

        // Subscribe the callback to the variable
        JTML::SubscriptionID subID = currentEnv->subscribeFunctionToVariable(key, node.functionName, callback);

        std::cout << "[SUBSCRIBE] Function '" << node.functionName
                  << "' subscribed to variable '" << key.varName << "'\n";
    } catch (const std::exception& e) {
        handleError("Subscribe Statement Error: " + std::string(e.what()));
    }
}


void Interpreter::interpretUnsubscribe(const UnsubscribeStatementNode& node) {
    try {
        // Get the VarID of the variable to unsubscribe from
         JTML::CompositeKey key = { currentEnv->instanceID, node.variableName };

        // Function name to unsubscribe
        std::string funcName = node.functionName;

        // Unsubscribe the function from the variable
        currentEnv->unsubscribeFunctionFromVariable(key, funcName);

        std::cout << "[UNSUBSCRIBE] Function '" << funcName
                  << "' unsubscribed from variable '" << node.variableName << "'\n";
    } catch (const std::exception& e) {
        handleError("Unsubscribe Statement Error: " + std::string(e.what()));
    }
}

void Interpreter::interpretFunctionDeclaration(const FunctionDeclarationNode& decl)
{   
    std::cout << "[DEBUG] Interpreting FunctionDeclarationNode: " << decl.toString() << "\n";
    // 1) Clone body for safe storage in the new function
    std::vector<std::unique_ptr<ASTNode>> clonedBody;
    clonedBody.reserve(decl.body.size());
    for (const auto& stmt : decl.body) {
        clonedBody.push_back(stmt->clone());
    }

    // 2) Build a Function object
    auto newFunc = std::make_shared<JTML::Function>(
        decl.name,
        decl.parameters,
        decl.returnType,
        std::move(clonedBody),
        currentEnv  // This environment is the 'closure'
    );

    // 3) Define the function by name in the current environment
    JTML::CompositeKey funcKey = { currentEnv->instanceID, decl.name };
    currentEnv->defineFunction(funcKey, newFunc);

    std::cout << "[DEBUG] Defined function '" << decl.name << "' with ";
    std::cout << decl.parameters.size() << " parameters:\n"; 
    for (const auto& stmt :newFunc->body) {
        std::cout << stmt->toString() << ", ";
    }
    std::cout << "returning " << (decl.returnType) << "\n";
              

    // 4) For *nested* function declarations in the body, define them too
    //    but skip normal statements (like 'if', 'return', etc.).
    //    We'll interpret normal statements only at call time.
    collectAllNestedFunctions(decl.body, newFunc->closure);
}

void Interpreter::collectAllNestedFunctions(
    const std::vector<std::unique_ptr<ASTNode>>& stmts,
    const std::shared_ptr<JTML::Environment>& closureEnv
)
{
    for (const auto& stmt : stmts) {
        if (!stmt) continue;

        switch (stmt->getType()) {
        case ASTNodeType::FunctionDeclaration: {
            // We found a nested function
            auto& nestedFuncDecl = static_cast<const FunctionDeclarationNode&>(*stmt);
            
            // Temporarily switch environment to closureEnv
            auto oldEnv = currentEnv;
            currentEnv = closureEnv;

            // interpretNode(...) will call interpretFunctionDeclaration(...),
            // which creates & defines the nested function in closureEnv
            std::vector<std::unique_ptr<ASTNode>> clonedBody;
            clonedBody.reserve(nestedFuncDecl.body.size());
            for (const auto& stmt : nestedFuncDecl.body) {
                clonedBody.push_back(stmt->clone());
            }

            // 2) Build a Function object
            auto newFunc = std::make_shared<JTML::Function>(
                nestedFuncDecl.name,
                nestedFuncDecl.parameters,
                nestedFuncDecl.returnType,
                std::move(clonedBody),
                currentEnv  // This environment is the 'closure'
            );

            JTML::CompositeKey funcKey = { currentEnv->instanceID, nestedFuncDecl.name };

            // 3) Define the function by name in the current environment
            currentEnv->defineFunction(funcKey, newFunc);
            currentEnv = oldEnv;
            break;
        }
        case ASTNodeType::IfStatement: {
            // We do NOT interpret the condition or run the blocks,
            // but we do gather function declarations inside then/else blocks
            auto& ifNode = static_cast<const IfStatementNode&>(*stmt);
            collectAllNestedFunctions(ifNode.thenStatements, closureEnv);
            collectAllNestedFunctions(ifNode.elseStatements, closureEnv);
            break;
        }
        case ASTNodeType::WhileStatement: {
            auto& whileNode = static_cast<const WhileStatementNode&>(*stmt);
            collectAllNestedFunctions(whileNode.body, closureEnv);
            break;
        }
        case ASTNodeType::ForStatement: {
            auto& forNode = static_cast<const ForStatementNode&>(*stmt);
            collectAllNestedFunctions(forNode.body, closureEnv);
            break;
        }
        case ASTNodeType::BlockStatement: {
            auto& blockNode = static_cast<const BlockStatementNode&>(*stmt);
            collectAllNestedFunctions(blockNode.statements, closureEnv);
            break;
        }
        // ... similarly handle TryExceptThen or others if they can contain child statements
        default:
            // Normal statements (Return, Show, etc.) => do nothing here
            break;
        }
    }
}

std::shared_ptr<JTML::VarValue> Interpreter::executeFunction(
    const std::shared_ptr<JTML::Function>& func,
    const std::vector<std::shared_ptr<JTML::VarValue>>& args,
    const std::shared_ptr<JTML::VarValue>& thisPtr 
) {
    // Check argument count
    if (args.size() != func->parameters.size()) {
        throw std::runtime_error("Function '" + func->name + "' expects " +
            std::to_string(func->parameters.size()) + " arguments, got " +
            std::to_string(args.size()));
    }

    // Manage function context
    bool previousContext = inFunctionContext;
    inFunctionContext = true;

    // Limit Recursion Depth
    static std::mutex recursionMutex;
    std::lock_guard<std::mutex> lock(recursionMutex);
    static int recursionDepth = 0;
    const int MAX_RECURSION_DEPTH = 1000;
    if (++recursionDepth > MAX_RECURSION_DEPTH) {
        throw std::runtime_error("Maximum recursion depth exceeded in function '" + func->name + "'");
    }

    // Create a new environment for the function execution
    auto funcEnv = std::make_shared<JTML::Environment>(
        func->closure, // Parent environment (closure)
        JTML::InstanceIDGenerator::getNextID(),
        currentEnv->renderer
    ); // Closure for accessing outer variables
    std::shared_ptr<JTML::Environment> previousEnv = currentEnv;
    currentEnv = funcEnv; // Update current environment

    // Bind parameters locally using JTML::CompositeKey
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        const std::string& paramName = func->parameters[i].name;
        JTML::CompositeKey paramKey = { funcEnv->instanceID, paramName };
        funcEnv->setVariable(paramKey, args[i]);
    }    
    
    // Bind 'this' if applicable
    if (thisPtr) {
        JTML::CompositeKey thisKey = { funcEnv->instanceID, "this" };
        funcEnv->setVariable(thisKey, thisPtr);
    }
    
    // Debug: Print function environment variables
    std::cout << "[DEBUG] Function '" << func->name << "' environment (InstanceID: " 
              << funcEnv->instanceID << ") variables:\n";
    for (const auto& [key, varInfo] : funcEnv->variables) {
        std::cout << "  " << funcEnv->getCompositeName(key) << " = " 
                  << (varInfo->currentValue ? varInfo->currentValue->toString() : "undefined") 
                  << "\n";
    }

    // Initialize return value
    std::shared_ptr<JTML::VarValue> returnValue = nullptr;

    try {
        // Interpret each statement in the function body
        for (const auto& stmt : func->body) {
            std::cout << "[DEBUG] Executing statement in function '" << func->name << "': " 
                      << stmt->toString() << "\n";
            interpretNode(*stmt);
        }
    } catch (const ReturnException& re) {
        std::cout << "[DEBUG] Function '" << func->name << "' returned with value: " 
                  << re.value->toString() << "\n";
        returnValue = re.value;
    } catch (const std::exception& e) {
        // Restore previous environment and context before handling the error
        currentEnv = previousEnv; // Restore previous environment
        inFunctionContext = previousContext;
        handleError("Function Execution Error: " + std::string(e.what()));
    }

    // Restore previous environment and context
    currentEnv = previousEnv; // Restore previous environment
    inFunctionContext = previousContext;

    // Debug: Print parent environment variables
    std::cout << "[DEBUG] Parent environment variables after function execution:\n";
    for (const auto& [key, varInfo] : currentEnv->variables) {
        std::cout << "  " << currentEnv->getCompositeName(key) << " = " 
                  << (varInfo->currentValue ? varInfo->currentValue->toString() : "undefined") 
                  << "\n";
    }

    --recursionDepth;

    // If no return statement, return a default value
    if (!returnValue) {
        return std::make_shared<JTML::VarValue>(JTML::ValueVariant{""});
    }

    return returnValue;
}

// Store a variable to a different scope
void Interpreter::storeVariable(const std::string& scope, const std::string& varName) {
    // Placeholder implementation
    // Depending on your scope management, implement storing logic here
    std::cout << "[STORE] Variable '" << varName << "' stored to scope '" << scope << "'.\n";
}

std::shared_ptr<JTML::VarValue> Interpreter::instantiateClass(
    const ClassDeclarationNode& classNode,
    const std::vector<std::unique_ptr<ExpressionStatementNode>>& arguments,
    std::shared_ptr<JTML::Environment> parentEnv
) {
    // Create an environment for the object with a unique InstanceID
    auto objEnv = std::make_shared<JTML::Environment>(
        parentEnv,
        JTML::InstanceIDGenerator::getNextID(),
        currentEnv->renderer
    );

    // Add class properties (from DefineStatements) to the object's environment
    for (const auto& member : classNode.members) {
        if (member->getType() == ASTNodeType::DefineStatement) {
            const auto& defNode = static_cast<const DefineStatementNode&>(*member);

            // Construct JTML::CompositeKey for the property
            JTML::CompositeKey propKey = { objEnv->instanceID, defNode.identifier };

            // Initialize property with a default value
            objEnv->setVariable(propKey, std::make_shared<JTML::VarValue>());
        }
    }

    std::shared_ptr<JTML::Function> constructorFunc = nullptr;

    // Look for a constructor function and prepare to execute it if found
    for (const auto& member : classNode.members) {
        if (member->getType() == ASTNodeType::FunctionDeclaration) {
            const auto& funcNode = static_cast<const FunctionDeclarationNode&>(*member);

            // Clone the constructor's body for safe storage
            std::vector<std::unique_ptr<ASTNode>> clonedBody;
            clonedBody.reserve(funcNode.body.size());
            for (const auto& stmt : funcNode.body) {
                clonedBody.push_back(stmt->clone()); // Ensure clone() is properly implemented
            }

            // Construct JTML::CompositeKey for the constructor
            JTML::CompositeKey constructorKey = { objEnv->instanceID, funcNode.name };

            // Create a Function instance for the constructor with cloned body
            auto newFunc = std::make_shared<JTML::Function>(
                funcNode.name,                         // Function name
                funcNode.parameters,                   // Function parameters
                funcNode.returnType,                   // Return type
                std::move(clonedBody),                 // Move the cloned function body
                objEnv                                 // Closure (object environment)
            );

            // Define the constructor function in the object's environment using JTML::CompositeKey
            objEnv->defineFunction(constructorKey, newFunc);

            if (funcNode.name == "constructor") {
                constructorFunc = newFunc;
            }
        }
    }

    if (constructorFunc) {
        // Evaluate arguments
        std::vector<std::shared_ptr<JTML::VarValue>> argValues;
        for (const auto& argExpr : arguments) {
            argValues.push_back(evaluateExpression(argExpr.get(), objEnv));
        }

        // Construct JTML::CompositeKey for the constructor
        JTML::CompositeKey constructorKey = { objEnv->instanceID, "constructor" };

        // Retrieve the constructor function using JTML::CompositeKey
        auto retrievedConstructor = objEnv->getFunction(constructorKey);
        if (!retrievedConstructor) {
            throw std::runtime_error("Constructor function not found in class '" + classNode.name + "'.");
        }

        // "this" = the current object wrapped in VarValue
        auto thisVarValue = std::make_shared<JTML::VarValue>(JTML::ValueVariant{ JTML::ObjectHandle{ objEnv } });

        // Execute the constructor function with arguments and 'this' bound
        executeFunction(retrievedConstructor, argValues, thisVarValue);
    }

    // Return the object wrapped in a VarValue
    return std::make_shared<JTML::VarValue>(JTML::ValueVariant{ JTML::ObjectHandle{ objEnv } });
}

// ------------------- Expression Evaluation -------------------

bool Interpreter::evaluateCondition(const ExpressionStatementNode* condition, std::shared_ptr<JTML::Environment> env) {
    if (!condition) {
        throw std::runtime_error("Null condition encountered.");
    }

    std::shared_ptr<JTML::VarValue> condValPtr = evaluateExpression(condition, env);
    std::string condVal = getStringValue(condValPtr);

    // Use isTruthy for truthiness evaluation
    try {
        return isTruthy(condVal);
    } catch (const std::exception& e) {
        throw std::runtime_error("Error evaluating condition: " + std::string(e.what()));
    }
}

// (F) Evaluate an expression and return its string value
std::shared_ptr<JTML::VarValue> Interpreter::evaluateExpression(const ExpressionStatementNode* exprNode, std::shared_ptr<JTML::Environment> env) {

    if (!exprNode) {
        throw std::runtime_error("Null expression node encountered.");
    }

    switch (exprNode->getExprType()) {
        case ExpressionStatementNodeType::Binary: {
            const auto* binExpr = static_cast<const BinaryExpressionStatementNode*>(exprNode);
            std::shared_ptr<JTML::VarValue> leftVal  = evaluateExpression(binExpr->left.get(), env);
            std::shared_ptr<JTML::VarValue> rightVal = evaluateExpression(binExpr->right.get(), env);
            const std::string& op = binExpr->op;

            // For logical ops "&&" and "||", etc...
            if (op == "&&" || op == "||") {
                bool leftBool  = isTruthy(leftVal->toString());
                bool rightBool = isTruthy(rightVal->toString());
                if (op == "&&") {
                    return std::make_shared<JTML::VarValue>(leftBool && rightBool ? "true" : "false");
                }
                else {
                    // op == "||"
                    bool resultBool = (leftBool || rightBool);
                    return std::make_shared<JTML::VarValue>(resultBool ? "true" : "false");
                }
            }
             if (binExpr->op == "==" || binExpr->op == "!=" ||
                binExpr->op == "<"  || binExpr->op == "<=" ||
                binExpr->op == ">"  || binExpr->op == ">=" ) 
            {
                // Decide if it's numeric or string compare
                // 1) If both are numeric => numeric compare
                // 2) Else if both are string => string compare
                // 3) Else fallback or convert if you want

                if (leftVal->isNumber() && rightVal->isNumber()) {
                    double ln = leftVal->getNumber();
                    double rn = rightVal->getNumber();
                    return std::make_shared<JTML::VarValue>(JTML::ValueVariant{performNumericCompare(op, ln, rn) });
                }
                else if (leftVal->isString() && rightVal->isString()) {
                    const std::string& ls = leftVal->toString();
                    const std::string& rs = rightVal->toString();
                    return std::make_shared<JTML::VarValue>(JTML::ValueVariant{performStringCompare(op, ls, rs) });
                }
                else {
                    // If they differ in type, you can define a rule
                    // e.g., coerce everything to string or numeric?
                    // For demonstration, let's coerce to string:
                    std::string ls = leftVal->toString();
                    std::string rs = rightVal->toString();
                    std::cout << "Comparing variables of different types!"<< ls << " "<< rs << "\n";
                    return std::make_shared<JTML::VarValue>(JTML::ValueVariant{ performStringCompare(op, ls, rs)} );
                }
            }

            if (op == "+") {
                if (leftVal->isString() || rightVal->isString()) {
                    return std::make_shared<JTML::VarValue>(leftVal->toString() + rightVal->toString());
                }
                else if (leftVal->isNumber() && rightVal->isNumber()) {
                    double result = leftVal->getNumber() + rightVal->getNumber();
                    return std::make_shared<JTML::VarValue>(result);
                }
                else {
                    throw std::runtime_error("Invalid types for '+' operation");
                }
            }

            // (B) The operators -, *, /, %
           else if (op == "-" || op == "*" || op == "/" || op == "%") {
                if (!leftVal->isNumber() || !rightVal->isNumber()) {
                    throw std::runtime_error("Arithmetic operators require numeric types");
                }
                double ln = leftVal->getNumber();
                double rn = rightVal->getNumber();
                double result;

                if (op == "-") {
                    result = ln - rn;
                }
                else if (op == "*") {
                    result = ln * rn;
                }
                else if (op == "/") {
                    if (rn == 0.0) throw std::runtime_error("Division by zero");
                    result = ln / rn;
                }
                else { // op == "%"
                    int li = static_cast<int>(ln);
                    int ri = static_cast<int>(rn);
                    if (ri == 0) throw std::runtime_error("Modulo by zero");
                    result = static_cast<double>(li % ri);
                }

                return std::make_shared<JTML::VarValue>(result);
            }
            // If we get here => unrecognized op
            throw std::runtime_error("Unsupported binary operator: " + op);
        }

        case ExpressionStatementNodeType::Unary: {
            const auto* unaryExpr = static_cast<const UnaryExpressionStatementNode*>(exprNode);
            std::shared_ptr<JTML::VarValue> operandVal = evaluateExpression(unaryExpr->right.get(), env);
            const std::string& op = unaryExpr->op;

            if (op == "!") {
                // logical NOT => just interpret truthiness
                bool val = isTruthy(operandVal->toString());
                return std::make_shared<JTML::VarValue>(val ? "false" : "true");
            }
            else if (op == "-") {
                // numeric negation => warn or try convert
                if (!operandVal->isNumber()) {
                    std::cerr << "[Warning] Using unary '-' on a non-numeric value.\n";
                }
                double num = 0.0;
                try {
                    num = std::stod(operandVal->toString());
                } catch (...) {
                    throw std::runtime_error("Invalid numeric format in unary '-'");
                }
                double result = -num;
                return std::make_shared<JTML::VarValue>(std::to_string(result));
            }
            else {
                throw std::runtime_error("Unsupported unary operator: " + op);
            }
        }

      case ExpressionStatementNodeType::Variable: {
            const auto* varExpr = static_cast<const VariableExpressionStatementNode*>(exprNode);
            // Construct JTML::CompositeKey for the variable
            JTML::CompositeKey varKey = { env->instanceID, varExpr->name };
            auto varVal = env->getVariable(varKey);
       
            std::cout << "[EVAL] Variable " << env->getCompositeName(varKey) << " (InstanceID: " << varKey.instanceID 
                      << ") = " << varVal->toString() << "\n";

            // Check if the variable is dirty and needs to be updated
            JTML::VarID varID = env->getVarID(varKey);

            if (env->dirtyVars.count(varID)) {
                updateVariable(varID, env);
            }

            return varVal;
        }  

        case ExpressionStatementNodeType::StringLiteral: {
            const auto* strExpr = static_cast<const StringLiteralExpressionStatementNode*>(exprNode);
            return std::make_shared<JTML::VarValue>(JTML::ValueVariant{strExpr->value});
        }

        case ExpressionStatementNodeType::CompositeString: {
            const auto* composite = static_cast<const CompositeStringExpressionStatementNode*>(exprNode);
            std::string result;

            for (const auto& part : composite->parts) {
                result += evaluateExpression(part.get(), env)->toString();
            }

            return std::make_shared<JTML::VarValue>(JTML::ValueVariant(result));
        }

        case ExpressionStatementNodeType::EmbeddedVariable: {
            const auto* embExpr = static_cast<const EmbeddedVariableExpressionStatementNode*>(exprNode);
            return evaluateExpression(embExpr->embeddedExpression.get(), env);
        }

        case ExpressionStatementNodeType::NumberLiteral: {
            const auto* numExpr = static_cast<const NumberLiteralExpressionStatementNode*>(exprNode);
            return std::make_shared<JTML::VarValue>(JTML::ValueVariant{numExpr->value});
        }

        case ExpressionStatementNodeType::BooleanLiteral: {
            const auto* boolExpr = static_cast<const BooleanLiteralExpressionStatementNode*>(exprNode);
            return std::make_shared<JTML::VarValue>(JTML::ValueVariant{boolExpr->value ? "true" : "false"});
        }

        case ExpressionStatementNodeType::ArrayLiteral: {
            const auto* arrNode = static_cast<const ArrayLiteralExpressionStatementNode*>(exprNode);

            JTML::CompositeKey tempKey = {env->instanceID, "__temp__array_" + std::to_string(uniqueArrayVarID++)};

            auto array = std::make_shared<JTML::ReactiveArray>(env, tempKey);
    
            // Evaluate each element in the array literal and add to the reactive array
       
            for (const auto& elemExpr : arrNode->elements) {
                auto elemValue = evaluateExpression(elemExpr.get(), env);
                array->push(elemValue);
            }

            
            return std::make_shared<JTML::VarValue>(array);
        }

        case ExpressionStatementNodeType::DictionaryLiteral: {
            const auto* dictNode = static_cast<const DictionaryLiteralExpressionStatementNode*>(exprNode);

            // Use the variable name if already defined or create a new one
            JTML::CompositeKey tempKey = {env->instanceID, "__temp__dict_" + std::to_string(uniqueDictVarID++)};


            // Create the reactive dict using the defined or derived key
            auto dict = std::make_shared<JTML::ReactiveDict>(env, tempKey);
         
            
            // Evaluate each element in the dict literal and add to the reactive dict
            for (auto& entry : dictNode->entries) {
                std::string key = entry.key.text;
                auto value = evaluateExpression(entry.value.get(), env);
                if (!value) {
                    handleError("Dictionary value evaluation failed");
                    value = std::make_shared<JTML::VarValue>("<error>");
                }
                dict->set(key, value);
            }
           

            return std::make_shared<JTML::VarValue>(dict);
        }

        case ExpressionStatementNodeType::Subscript: {
            auto* subExpr = static_cast<const SubscriptExpressionStatementNode*>(exprNode);
            auto baseVal  = evaluateExpression(subExpr->base.get(), env);
            auto indexVal = evaluateExpression(subExpr->index.get(), env);

            if (baseVal->isArray()) {
                auto reactiveArray = baseVal->getArray();
                double idxNum = getNumericValue(indexVal);
                int idx = static_cast<int>(idxNum);

                if (idx < 0 || static_cast<size_t>(idx) >= reactiveArray->size()) {
                    throw std::runtime_error("Array index out of bounds: " + std::to_string(idx));
                }

                return reactiveArray->get(idx);
            }
            else if (baseVal->isDict()) {
                auto reactiveDict = baseVal->getDict();
                std::string key = getStringValue(indexVal);
                return reactiveDict->get(key);
            }
            else {
                throw std::runtime_error("Subscript on non-array/non-dict");
            }
        }

        case ExpressionStatementNodeType::FunctionCall: {
            const auto* callExpr = static_cast<const FunctionCallExpressionStatementNode*>(exprNode);

            // Check if the function name corresponds to a class for instantiation
            auto classIt = classDeclarations.find(callExpr->functionName);
            if (classIt != classDeclarations.end()) {
                // Instantiate the class
                return instantiateClass(*classIt->second, callExpr->arguments, env);
            }

            std::cout << "Function call: " << callExpr->toString() << "\n";

            if (!callExpr) {
                throw std::runtime_error("FunctionCallExpressionStatementNode is null.");
            }

            // Construct JTML::CompositeKey for the function
            JTML::CompositeKey funcKey = { env->instanceID, callExpr->functionName };
            auto func = env->getFunction(funcKey);
            if (!func) {
                throw std::runtime_error("Function '" + callExpr->functionName + "' not found in the current environment.");
            }

            // Evaluate arguments
            std::vector<std::shared_ptr<JTML::VarValue>> args;
            for (size_t i = 0; i < callExpr->arguments.size(); ++i) {
                if (!callExpr->arguments[i]) {
                    throw std::runtime_error("Null argument at index " + std::to_string(i) +
                                            " in function call to '" + callExpr->functionName + "'.");
                }
                args.push_back(evaluateExpression(callExpr->arguments[i].get(), env));
            }

            // Debug logging
            std::cout << "[DEBUG] Calling function: " << func->name << "\n";
            for (const auto& arg : args) {
                std::cout << "[DEBUG] Argument value: " << (arg ? arg->toString() : "null") << "\n";
            }

            // Display function body for debugging
            for (const auto& stmt : func->body) {
                std::cout << "[DEBUG] Body of function " << func->name << " value: " << stmt->toString() << "\n";
            }

            // Display current environment variables
            std::cout << "[DEBUG] Current environment before function call:\n";
            for (const auto& [key, varInfo] : env->variables) {
                std::cout << "  " << env->getCompositeName(key) << " = " 
                          << (varInfo->currentValue ? varInfo->currentValue->toString() : "undefined") 
                          << "\n";
            }

            // Display closure environment variables
            std::cout << "[DEBUG] Closure for function " << func->name << ":\n";
            for (const auto& [key, varInfo] : func->closure->variables) {
                std::cout << "  " << env->getCompositeName(key) << " = " 
                          << (varInfo->currentValue ? varInfo->currentValue->toString() : "undefined") 
                          << "\n";
            }

            // Execute the function
            try {
                return executeFunction(func, args, nullptr); // 'this' is nullptr for regular functions
            } catch (const std::exception& e) {
                throw std::runtime_error("Error during execution of function '" + callExpr->functionName + "': " + e.what());
            }
        }

        case ExpressionStatementNodeType::ObjectPropertyAccess: {
            const auto* propAccess = static_cast<const ObjectPropertyAccessExpressionNode*>(exprNode);

            // Evaluate the base object
            std::shared_ptr<JTML::VarValue> baseVal = evaluateExpression(propAccess->base.get(), env);

            if (!baseVal->isObject()) {
                throw std::runtime_error("Attempted to access a property on a non-object.");
            }

            // Retrieve the property from the object's environment
            auto objHandle = baseVal->getObjectHandle();
            JTML::CompositeKey propKey = { objHandle.instanceEnv->instanceID, propAccess->propertyName };
            auto propertyVal = objHandle.instanceEnv->getVariable(propKey);

            if (!propertyVal) {
                throw std::runtime_error("Property '" + propAccess->propertyName + "' not found in object.");
            }

            std::cout << "[EVAL] Accessing property '" << propKey.varName << "' (InstanceID: " << propKey.instanceID 
                      << ") = " << propertyVal->toString() << "\n";

            return propertyVal;
        }

        case ExpressionStatementNodeType::ObjectMethodCall: {
            const auto* methodCall = static_cast<const ObjectMethodCallExpressionNode*>(exprNode);

            // Evaluate the base object
            std::shared_ptr<JTML::VarValue> baseVal = evaluateExpression(methodCall->base.get(), env);

            if (!baseVal->isObject()) {
                throw std::runtime_error("Attempted to call a method on a non-object.");
            }

            // Retrieve the method from the object's environment
            auto objHandle = baseVal->getObjectHandle();
            JTML::CompositeKey methodKey = { objHandle.instanceEnv->instanceID, methodCall->methodName };
            auto methodFunc = objHandle.instanceEnv->getFunction(methodKey);

            if (!methodFunc) {
                throw std::runtime_error("Method '" + methodCall->methodName + "' not found in object.");
            }

            // Evaluate the arguments
            std::vector<std::shared_ptr<JTML::VarValue>> args;
            for (const auto& argExpr : methodCall->arguments) {
                args.push_back(evaluateExpression(argExpr.get(), env));
            }

            // Execute the method with 'this' bound to the object
            std::shared_ptr<JTML::VarValue> returnValue = executeFunction(methodFunc, args, baseVal);

            std::cout << "[EVAL] Executed method '" << methodCall->methodName << "' on object (InstanceID: " 
                      << objHandle.instanceEnv->instanceID << ")\n";

            return returnValue;
        }
        
        default:
            throw std::runtime_error("Unknown ExpressionStatementNodeType encountered during evaluation.");
    }
}

bool Interpreter::isTruthy(const std::string& value) {
    // Boolean literals
    if (value == "true") return true;
    if (value == "false") return false;

    // Numeric values: Non-zero is truthy
    try {
        return std::stod(value) != 0.0;
    } catch (const std::invalid_argument&) {
        // Non-numeric, proceed to check as string
    }

    // Strings: Non-empty is truthy
    return !value.empty();
}

bool Interpreter::performNumericCompare(const std::string& op, double ln, double rn) {
    if (op == "==") return ln == rn;
    if (op == "!=") return ln != rn;
    if (op == "<")  return ln <  rn;
    if (op == "<=") return ln <= rn;
    if (op == ">")  return ln >  rn;
    if (op == ">=") return ln >= rn;
    throw std::runtime_error("Invalid numeric comparison operator: " + op);
}

bool Interpreter::performStringCompare(const std::string& op, const std::string& ls, const std::string& rs) {
    if (op == "==") return (ls == rs);
    if (op == "!=") return (ls != rs);
    if (op == "<")  return (ls <  rs);  // lexicographic
    if (op == "<=") return (ls <= rs);
    if (op == ">")  return (ls >  rs);
    if (op == ">=") return (ls >= rs);
    throw std::runtime_error("Invalid string comparison operator: " + op);
}

// (G) Gather dependencies from an expression
// Interpreter.cpp

void Interpreter::gatherDeps(
    const ExpressionStatementNode* exprNode, 
    std::vector<JTML::CompositeKey>& out, 
    std::shared_ptr<JTML::Environment> env
) {
    if (!exprNode) return;

    std::cout << "[DEBUG 0] gatherDeps Element: <" << exprNode->toString() << ">\n";

    switch (exprNode->getExprType()) {
        case ExpressionStatementNodeType::Variable: {
            const auto* varExpr = static_cast<const VariableExpressionStatementNode*>(exprNode);
            JTML::CompositeKey varKey = { env->instanceID, varExpr->name };

            try {
                auto varValue = env->getVariable(varKey);
                if (!varValue) {
                    throw std::runtime_error("Variable '" + varExpr->name + "' not found.");
                }

                // Add the variable itself as a dependency
                out.push_back(varKey);
                std::cout << "[GATHER_DEPS] Dependency found: " 
                          << env->getCompositeName(varKey) << "\n";

                // Determine the VarKind and gather additional dependencies accordingly
                

            } catch (const std::exception& e) {
                handleError("Dependency Gathering Error: " + std::string(e.what()));
            }

            break;  
        }

        case ExpressionStatementNodeType::Binary: {
            const auto* binExpr = static_cast<const BinaryExpressionStatementNode*>(exprNode);
            gatherDeps(binExpr->left.get(), out, env);
            gatherDeps(binExpr->right.get(), out, env);
            break;
        }

        case ExpressionStatementNodeType::Unary: {
            const auto* unaryExpr = static_cast<const UnaryExpressionStatementNode*>(exprNode);
            gatherDeps(unaryExpr->right.get(), out, env);
            break;
        }

        case ExpressionStatementNodeType::StringLiteral: {
            break;
        }

        case ExpressionStatementNodeType::CompositeString: {
            const auto* compositeExpr = static_cast<const CompositeStringExpressionStatementNode*>(exprNode);
            for (const auto& part : compositeExpr->parts) {
                gatherDeps(part.get(), out, env);
            }
            break;
        }

        case ExpressionStatementNodeType::EmbeddedVariable: {
            const auto* embeddedExpr = static_cast<const EmbeddedVariableExpressionStatementNode*>(exprNode);
            gatherDeps(embeddedExpr->embeddedExpression.get(), out, env);
            break;
        }

        case ExpressionStatementNodeType::NumberLiteral:
            // Literals have no dependencies
            break;

        case ExpressionStatementNodeType::ArrayLiteral: {
            const auto* arrNode = static_cast<const ArrayLiteralExpressionStatementNode*>(exprNode);

            // Iterate through each element in the array literal
            for (const auto& elemExpr : arrNode->elements) {
                // Recursively gather dependencies from each element
                gatherDeps(elemExpr.get(), out, env);
            }
            break;
        }

        case ExpressionStatementNodeType::DictionaryLiteral: {
            const auto* dictNode = static_cast<const DictionaryLiteralExpressionStatementNode*>(exprNode);

            // Iterate through each entry in the dictionary
            for (const auto& entry : dictNode->entries) {
                // Gather dependencies from the value expressions
                gatherDeps(entry.value.get(), out, env);
            }

            // Keys are usually static (strings/identifiers) and don't add to dependencies
            break;
        }

        case ExpressionStatementNodeType::Subscript: {
            auto* sub = static_cast<const SubscriptExpressionStatementNode*>(exprNode);

            // First, gather dependencies from the base
            gatherDeps(sub->base.get(), out, env);

            // Then gather the index or key
            gatherDeps(sub->index.get(), out, env);

            // Add dependency for the full array or dictionary and specific element/key
            if (sub->base->getExprType() == ExpressionStatementNodeType::Variable) {
                const auto* varNode = static_cast<const VariableExpressionStatementNode*>(sub->base.get());
                JTML::CompositeKey fullArrayKey = { env->instanceID, varNode->name };
                out.emplace_back(fullArrayKey);
                std::cout << "[GATHER_DEPS] Dependency found: " 
                          << env->getCompositeName(fullArrayKey) << "\n";

                // Specific element dependency
                // Construct a unique key for the subscripted element, e.g., "a.array_1[0]"
                std::string specificName = varNode->name + "["; // Start of subscript
                if (sub->index->getExprType() == ExpressionStatementNodeType::NumberLiteral) {
                    const auto* indexLiteral = static_cast<const NumberLiteralExpressionStatementNode*>(sub->index.get());
                    specificName += std::to_string(static_cast<int>(indexLiteral->value)) + "]";
                } else if (sub->index->getExprType() == ExpressionStatementNodeType::StringLiteral) {
                    const auto* indexLiteral = static_cast<const StringLiteralExpressionStatementNode*>(sub->index.get());
                    specificName += indexLiteral->value + "]";
                } else {
                    // Handle other index types if necessary
                    specificName += "unknown]";
                }
                JTML::CompositeKey specificKey = { env->instanceID, specificName };
                out.emplace_back(specificKey);
                std::cout << "[GATHER_DEPS] Specific Subscript Dependency added: " 
                          << env->getCompositeName(specificKey) << "\n";
            }
            break;
        }

        default:
            // Handle other expression types as needed
            break;
    }
}


// ------------------- Batch Updates -------------------


// ------------------- Cycle Detection -------------------

// (E) Detect cycle using DFS

void Interpreter::recalcDirty(std::shared_ptr<JTML::Environment> env) {
    env->recalcDirty([this, &env](JTML::VarID varID) {
        this->updateVariable(varID, env);
    });
}

void Interpreter::updateVariable(JTML::VarID varID, std::shared_ptr<JTML::Environment> env) {
   
    JTML::CompositeKey key = env->idToKey[varID];
    auto it = env->variables.find(key);
    if (it == env->variables.end()) {
        handleError("Attempted to update undefined variable '" + env->getCompositeName(key) + 
            "' in InstanceID " + std::to_string(env->instanceID));
        return;
    }

    if (it->second->kind == JTML::VarKind::Derived && it->second->expression) {
        try {
            std::shared_ptr<JTML::VarValue> newValue = evaluateExpression(it->second->expression.get(), env);
            std::cout << "[UPDATE] Evaluated " << key.varName << " = " << newValue->toString() << "\n";
            if (getStringValue(newValue) != getStringValue(it->second->currentValue)) {
                it->second->currentValue = newValue;
                std::cout << "[UPDATE] " << key.varName << " updated to " << newValue->toString() << "\n";
                // Emit events
                env->emitEvents(varID);
                // Notify dependents by marking them dirty
                for (const auto& dependentVarID : env->adjacency[varID]) {
                    std::cout << "[UPDATE] marking dirty variable dependent on" << key.varName << " name " << dependentVarID << "\n";
                    JTML::CompositeKey dependentKey = env->idToKey[dependentVarID];
                    env->markDirty(dependentKey);
                }
            }
        }
        catch (const std::exception& e) {
            handleError("Error updating derived variable '" + key.varName + "': " + e.what());
        }
    }
    else {
        std::cout << "[SKIP] Normal variable '" << key.varName << "' does not require updates.\n";
    }
    // Normal variables do not require updates
}


double Interpreter::getNumericValue(const std::shared_ptr<JTML::VarValue>& valPtr) {
    if (!valPtr) throw std::runtime_error("Null VarValue");
    if (valPtr->isNumber()) {
        return valPtr->getNumber();
    }
    else if (valPtr->isBool()) {
        return valPtr->getBool() ? 1.0 : 0.0;
    }
    else if (valPtr->isString()) {
        // attempt to parse the string as double
        const auto& s = valPtr->getString();
        try {
            return std::stod(s);
        } catch (...) {
            throw std::runtime_error("Cannot convert string to number: " + s);
        }
    }
    // If it's array or dict, throw
    throw std::runtime_error("Cannot convert array/dict to number");
}

std::string Interpreter::getStringValue(const std::shared_ptr<JTML::VarValue>& valPtr) {
    if (!valPtr) throw std::runtime_error("Null VarValue");
    if (valPtr->isString()) {
        return valPtr->getString();
    }
    else if (valPtr->isNumber()) {
        double d = valPtr->getNumber();
        std::ostringstream oss; oss << d;
        return oss.str();
    }
    else if (valPtr->isBool()) {
        return valPtr->getBool() ? "true" : "false";
    }
    else if (valPtr->isArray()) {
        // convert array to string e.g. "[...]"
        // or you can do a custom print
        auto& arr = valPtr->getArray()->getArrayData();
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << getStringValue(arr[i]);
        }
        oss << "]";
        return oss.str();
    }
    else if (valPtr->isDict()) {
        auto& dict = valPtr->getDict()->getDictData();
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (auto& kv : dict) {
            if (!first) oss << ", ";
            first = false;
            oss << "\"" << kv.first << "\": " << getStringValue(kv.second);
        }
        oss << "}";
        return oss.str();
    }
    throw std::runtime_error("Unknown type in getStringValue");
}


// ------------------- Error Handling -------------------

// Handle and report errors
void Interpreter::handleError(const std::string& message) {
    std::cerr << "Interpreter Error: " << message << "\n";
    // Depending on requirements, you might throw exceptions or handle errors differently
}


std::string toString(ASTNodeType nodeType) {
    switch (nodeType) {
        case ASTNodeType::DefineStatement: return "DefineStatement";
        case ASTNodeType::DeriveStatement: return "DeriveStatement";
        case ASTNodeType::UnbindStatement: return "UnbindStatement";
        case ASTNodeType::StoreStatement: return "StoreStatement";
        case ASTNodeType::ThrowStatement: return "ThrowStatement";
        case ASTNodeType::ShowStatement: return "ShowStatement";
        case ASTNodeType::AssignmentStatement: return "AssignmentStatement";
        case ASTNodeType::ExpressionStatement: return "ExpressionStatement";
        case ASTNodeType::FunctionDeclaration: return "FunctionDeclaration";
        case ASTNodeType::ClassDeclaration: return "ClassDeclaration";
        case ASTNodeType::ReturnStatement: return "ReturnStatement";
        case ASTNodeType::BreakStatement: return "BreakStatement";
        case ASTNodeType::ContinueStatement: return "ContinueStatement";
        case ASTNodeType::SubscribeStatement: return "SubscribeStatement";
        case ASTNodeType::IfStatement: return "IfStatement";
        case ASTNodeType::WhileStatement: return "WhileStatement";
        case ASTNodeType::ForStatement: return "ForStatement";
        case ASTNodeType::BlockStatement: return "BlockStatement";
        case ASTNodeType::TryExceptThen: return "TryExceptThen";
        case ASTNodeType::JtmlElement: return "JtmlElement";
        // Add other ASTNodeTypes here as needed
        default: return "Unknown";
    }
}