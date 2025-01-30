// jtml_parser.cpp
#include "../include/jtml_parser.h"

// ------------------- Parser Class Implementations -------------------
std::vector<std::string> loopContextStack; 
Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)), m_pos(0), m_line(1), m_column(1) {}

// Parses the entire program and returns a vector of AST nodes
std::vector<std::unique_ptr<ASTNode>> Parser::parseProgram() {
    std::vector<std::unique_ptr<ASTNode>> nodes;
    std::cout << "=== Starting Program Parsing ===\n";

    while (!isAtEnd()) {
        try {
            std::cout << "[DEBUG] Parsing statement at token position: " << m_pos << "\n";
            auto stmt = parseStatement();
            if (stmt) { // Statement successfully parsed
                std::cout << "[DEBUG] Parsed Node Type: " << static_cast<int>(stmt->getType()) << "\n";
                nodes.push_back(std::move(stmt));
            } else {
                std::cout << "[DEBUG] Null statement encountered.\n";
            }
        } catch (const std::runtime_error& e) {
            std::cout << "[ERROR] " << e.what() << " at token position: " << m_pos << "\n";
            recordError(e.what());
            synchronize();
        }
    }

    std::cout << "=== Finished Program Parsing ===\n";
    return nodes;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    std::cout << "[DEBUG] Attempting to parse a statement at token position: " << m_pos << "\n";

    if (check(TokenType::SHOW)) {
        std::cout << "[DEBUG] Found 'show' statement.\n";
        return parseShowStatement();
    }
    if (check(TokenType::DEFINE)) {
        std::cout << "[DEBUG] Found 'define' statement.\n";
        return parseDefineStatement();
    }
    if (check(TokenType::DERIVE)) {
        std::cout << "[DEBUG] Found 'derive' statement.\n";
        return parseDeriveStatement();
    }
    if (check(TokenType::UNBIND)) {
        std::cout << "[DEBUG] Found 'unbind' statement.\n";
        return parseUnbindStatement();
    }
    if (check(TokenType::STORE)) {
        std::cout << "[DEBUG] Found 'store' statement.\n";
        return parseStoreStatement();
    }
    if (check(TokenType::IF)) {
        std::cout << "[DEBUG] Found 'if' statement.\n";
        return parseIfElseStatement();
    }
    if (check(TokenType::WHILE)) {
        std::cout << "[DEBUG] Found 'while' statement.\n";
        return parseWhileStatement();
    }
    if (check(TokenType::BREAK)) {
        std::cout << "[DEBUG] Found 'break' statement.\n";
        return parseBreakStatement();
    }
    if (check(TokenType::CONTINUE)) {
        std::cout << "[DEBUG] Found 'continue' statement.\n";
        return parseContinueStatement();
    }
    if (check(TokenType::FUNCTION)) {
        std::cout << "[DEBUG] Found 'function' declaration.\n";
        return parseFunctionDeclaration();
    }
    if (check(TokenType::OBJECT)) {
        std::cout << "[DEBUG] Found 'object' declaration.\n";
        return parseClassDeclaration();
    }
    if (check(TokenType::SUBSCRIBE)) {
        std::cout << "[DEBUG] Found 'subscribe' declaration.\n";
        return parseSubscribeStatement();
    }
    if (check(TokenType::FOR)) {
        std::cout << "[DEBUG] Found 'for' statement.\n";
        return parseForStatement();
    }
    if (check(TokenType::TRY)) {
        std::cout << "[DEBUG] Found 'try' statement.\n";
        return parseTryExceptThenStatement();
    }
    if (check(TokenType::RETURN)) {
        std::cout << "[DEBUG] Found 'return' statement.\n";
        return parseReturnStatement();
    }
    if (check(TokenType::THROW)) {
        std::cout << "[DEBUG] Found 'throw' statement.\n";
        return parseThrowStatement();
    }
    if (check(TokenType::ELEMENT)) {
        std::cout << "[DEBUG] Found JtmlElement (element).\n";
        return parseJtmlElement();
    }

    // Handle assignment statements
    if (canBeReferenceExpression()) {
        bool validLHS = false;
        auto potentialLHS = parseReferenceExpression(validLHS);
        std::cout << "[DEBUG] Found potential LHS expression: " << potentialLHS->toString() << "\n";
        if (check(TokenType::ASSIGN)) {
            // If '=' follows, it's an assignment statement
            return parseAssignmentStatement(std::move(potentialLHS));
        } else {
            // Otherwise, it's a standalone expression statement
            return parseExpressionStatement(std::move(potentialLHS));
        }
    }

    std::cout << "[DEBUG] Found expression statement.\n";
    return parseExpressionStatement();

    // Fallback for unexpected tokens
    Token t = peek();
    std::string errorMessage = "Unexpected token '" + t.text +
        "' at line " + std::to_string(t.line) +
        ", column " + std::to_string(t.column);
    std::cout << "[ERROR] " << errorMessage << "\n";
    throw std::runtime_error(errorMessage);
}
bool Parser::canBeReferenceExpression() {
    return check(TokenType::IDENTIFIER) && !checkNext(TokenType::LPAREN); // Currently, only identifiers are valid LHS starters
}

std::unique_ptr<ASTNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression(); // Parse the expression
    consume(TokenType::STMT_TERMINATOR, "Expected '\\' after expression statement");
    
    auto node = std::make_unique<ExpressionNode>();
    node->expression = std::move(expr);
    return node;
}

std::unique_ptr<ASTNode> Parser::parseExpressionStatement(std::unique_ptr<ExpressionStatementNode> lhs = nullptr) {
    std::unique_ptr<ExpressionStatementNode> expr;

    if (lhs) {
        // Use the pre-parsed LHS if available
        expr = std::move(lhs);
    } else {
        // Otherwise, parse the entire expression
        expr = parseExpression();
    }

    consume(TokenType::STMT_TERMINATOR, "Expected '\\' after expression statement");

    auto node = std::make_unique<ExpressionNode>();
    node->expression = std::move(expr);
    return node;
}

// Parses an expression and returns an ExpressionStatementNode
std::unique_ptr<ExpressionStatementNode> Parser::parseExpression() {
    ExpressionParser ep(m_tokens, m_pos, *this); // Pass the parent Parser instance
    return ep.parseExpression();
}

// Parses an assignment statement (e.g., 'a = 10\\')
std::unique_ptr<ASTNode> Parser::parseAssignmentStatement(
    std::unique_ptr<ExpressionStatementNode> lhs) {
    if (!lhs) {
        throw std::runtime_error("Expected a valid LHS for assignment statement");
    }
    consume(TokenType::ASSIGN, "Expected '=' after reference expression");
    auto rhs = parseExpression();          // Parse the RHS expression
    consume(TokenType::STMT_TERMINATOR, "Expected '\\' after assignment statement");

    auto node = std::make_unique<AssignmentStatementNode>();
    node->lhs = std::move(lhs);            // Use the parsed LHS
    node->rhs = std::move(rhs);            // Use the parsed RHS
    return node;
}

std::unique_ptr<ExpressionStatementNode> Parser::parseReferenceExpression(bool &validLHS) {
    // Consume the initial identifier token (e.g., variable name)
    Token idTok = consume(TokenType::IDENTIFIER, "Expected an identifier for reference expression.");

    // Start with the base variable expression
    std::unique_ptr<ExpressionStatementNode> expr = std::make_unique<VariableExpressionStatementNode>(idTok);

    std::cout << "[DEBUG] Parsed base variable: " << idTok.text << "\n";

    // Continuously parse property accesses, method calls, and subscript accesses
    while (true) {
        if (match(TokenType::DOT)) {
            // Handle property access or method call after '.'

            // Consume the property or method name identifier
            Token memberToken = consume(TokenType::IDENTIFIER, "Expected property or method name after '.'");

            if (match(TokenType::LPAREN)) {
                // Method call detected: obj.method(args)
                std::vector<std::unique_ptr<ExpressionStatementNode>> arguments;
                if (!check(TokenType::RPAREN)) {
                    do {
                        arguments.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                consume(TokenType::RPAREN, "Expected ')' after function call arguments.");

                expr = std::make_unique<ObjectMethodCallExpressionNode>(
                    std::move(expr), 
                    memberToken.text, 
                    std::move(arguments)
                );

                validLHS = false; // A method call cannot be an LHS
                std::cout << "[DEBUG] Parsed method call: " << memberToken.text << "\n";
            } else {
                // Property access: obj.prop
                expr = std::make_unique<ObjectPropertyAccessExpressionNode>(
                    std::move(expr), memberToken.text
                );

                std::cout << "[DEBUG] Parsed property access: " << memberToken.text << "\n";
            }
        
        }
        else if (match(TokenType::LBRACKET)) {
            // Handle subscript access after '[': e.g., arr[0]

            // Parse the index expression inside the brackets
            std::unique_ptr<ExpressionStatementNode> indexExpr = parseExpression();

            // Expect a closing bracket ']'
            consume(TokenType::RBRACKET, "Expected ']' after subscript.");

            // Create a SubscriptExpressionStatementNode representing the subscript access
            expr = std::make_unique<SubscriptExpressionStatementNode>(
                std::move(expr),             // Base array/dictionary expression (e.g., 'arr')
                std::move(indexExpr)        // Index/key expression (e.g., '0')
            );

            std::cout << "[DEBUG] Parsed subscript access.\n";
        }
        else {
            // No further property/method/subscript access; exit the loop
            break;
        }
    }

    // Return the fully parsed reference expression
    return expr;
}

std::vector<std::unique_ptr<ExpressionStatementNode>> Parser::parseArguments() {
    std::vector<std::unique_ptr<ExpressionStatementNode>> arguments;

    if (!check(TokenType::RPAREN)) { // If not immediately closing the parenthesis
        do {
            // Parse each argument expression
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA)); // Continue until no more commas
    }

    consume(TokenType::RPAREN, "Expected ')' after arguments.");
    return arguments;
}


// Parses a derive statement (e.g., 'derive sum = a + b\\')
std::unique_ptr<ASTNode> Parser::parseDeriveStatement() {
    // Grammar: derive IDENTIFIER (: type)? = expression STMT_TERMINATOR
    Token deriveTok = consume(TokenType::DERIVE, "Expected 'derive' keyword");
    Token idTok = consume(TokenType::IDENTIFIER, "Expected identifier after 'derive'");

    std::string declaredType;
    if (check(TokenType::COLON)) {
        advance(); // Consume ':'
        Token typeTok = consume(TokenType::IDENTIFIER, "Expected type identifier after ':'");
        declaredType = typeTok.text;
    }

    consume(TokenType::ASSIGN, "Expected '=' in derive statement");

    auto expr = parseExpression();

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after derive statement");

    auto node = std::make_unique<DeriveStatementNode>();
    node->identifier = idTok.text;
    node->declaredType = declaredType;
    node->expression = std::move(expr);
    return node;
}

// Parses an unbind statement (e.g., 'unbind sum\\')
std::unique_ptr<ASTNode> Parser::parseUnbindStatement() {
    consume(TokenType::UNBIND, "Expected 'unbind'");
    Token idTok = consume(TokenType::IDENTIFIER, "Expected identifier after 'unbind'");
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after unbind statement");

    auto node = std::make_unique<UnbindStatementNode>();
    node->identifier = idTok.text;
    return node;
}

// Parses a store statement (e.g., 'store(main) a\\')
std::unique_ptr<ASTNode> Parser::parseStoreStatement() {
    consume(TokenType::STORE, "Expected 'store' keyword");
    consume(TokenType::LPAREN, "Expected '(' after 'store'");

    // Scope can be MAIN or IDENTIFIER
    std::string scopeStr;
    if (check(TokenType::MAIN)) {
        scopeStr = "main";
        advance(); // Consume 'main'
    } else if (check(TokenType::IDENTIFIER)) {
        scopeStr = peek().text;
        advance();
    } else {
        throw std::runtime_error("Expected 'main' or identifier in store(...)");
    }

    consume(TokenType::RPAREN, "Expected ')' after scope identifier");

    Token varTok = consume(TokenType::IDENTIFIER, "Expected variable name after store(...)");
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after store statement");

    auto node = std::make_unique<StoreStatementNode>();
    node->targetScope = scopeStr;
    node->variableName = varTok.text;
    return node;
}

// Parses a show statement (e.g., 'show sum\\')
std::unique_ptr<ASTNode> Parser::parseShowStatement() {
    consume(TokenType::SHOW, "Expected 'show'");
    auto exprNode = parseExpression();  
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after show statement");

    auto showNode = std::make_unique<ShowStatementNode>();
    showNode->expr = std::move(exprNode);
    return showNode;
}

// Parses a define statement (e.g., 'define a = 2\\')
std::unique_ptr<ASTNode> Parser::parseDefineStatement() {
    consume(TokenType::DEFINE, "Expected 'define'");
    Token idTok = consume(TokenType::IDENTIFIER, "Expected identifier after 'define'");
    consume(TokenType::ASSIGN, "Expected '=' in define statement");
    auto exprNode = parseExpression();
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after define statement");

    auto defNode = std::make_unique<DefineStatementNode>();
    defNode->identifier = idTok.text;
    defNode->expression = std::move(exprNode);
    return defNode;
}

// Parses a single top-level JtmlElement (e.g., '#div ... \\#div')
std::unique_ptr<JtmlElementNode> Parser::parseJtmlElement() {

        std::cout << "[DEBUG] Parsing JtmlElement at token position: " << m_pos << "\n";

        auto elem = std::make_unique<JtmlElementNode>();

        // Expect "element" keyword
        consume(TokenType::ELEMENT, "Expected 'element' keyword.");
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected element name after 'element'.");

        elem->tagName = nameToken.text;

        // Parse attributes list
        std::vector<JtmlAttribute> attributes;      
        while (check(TokenType::IDENTIFIER)) {
            Token attrName = consume(TokenType::IDENTIFIER, "Expected attribute name.");

            consume(TokenType::ASSIGN, "Expected '=' after attribute name.");
            auto exprNode = parseExpression();
            attributes.push_back({attrName.text, std::move(exprNode)});
                  
            // Handle optional commas between attributes
            if (!check(TokenType::COMMA)) {
                continue;
            }
            advance(); // Consume the comma
        }

        // Expect end of attribute list
        consume(TokenType::STMT_TERMINATOR, "Expected '\\' after attribute list.");

        elem->attributes = attributes;

        // Parse the element body
        std::vector<std::unique_ptr<ASTNode>> body;
        while (!check(TokenType::HASH) && !isAtEnd()) {
            if (auto stmt = parseStatement()) {
                body.push_back(std::move(stmt));
            } else {
                break;
            }
        }

        elem->content = std::move(body);

        consume(TokenType::HASH, "Expected '#' at the end of element body.");

        std::cout << "[DEBUG] Successfully parsed JtmlElement '" << nameToken.text << "' with "
                << attributes.size() << " attributes and " << body.size() << " body nodes.\n";

        return elem;
    
}

// Parses an if-else statement
std::unique_ptr<ASTNode> Parser::parseIfElseStatement() {
    consume(TokenType::IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condExpr = parseExpression();

    consume(TokenType::RPAREN, "Expected ')' after condition");

    auto ifNode = std::make_unique<IfStatementNode>();
    ifNode->condition = std::move(condExpr);

    // Parse the block of statements
    parseBlockStatementList(ifNode->thenStatements);

    if (check(TokenType::ELSE)) {
        advance(); // Consume 'else'
        parseBlockStatementList(ifNode->elseStatements);
    }
    return ifNode;
}

// Parses a while statement 
std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
    loopContextStack.push_back("while");
    consume(TokenType::WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto conditionExpr = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");

    auto whileNode = std::make_unique<WhileStatementNode>();
    whileNode->condition = std::move(conditionExpr);

    // Parse the body (a block of statements delimited by '\' lines)
    parseBlockStatementList(whileNode->body);

    loopContextStack.pop_back();

    return whileNode;
}

// Parses a break statement 
std::unique_ptr<ASTNode> Parser::parseBreakStatement() {
    if (loopContextStack.empty()) {
        throw std::runtime_error("Error: 'break' used outside of a loop at line " + std::to_string(m_line));
    }
    consume(TokenType::BREAK, "Expected 'break'");

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after break statement");

    auto breakNode = std::make_unique<BreakStatementNode>();
    
    return breakNode;
}

// Parses a continue statement 
std::unique_ptr<ASTNode> Parser::parseContinueStatement() {
    if (loopContextStack.empty()) {
        throw std::runtime_error("Error: 'break' used outside of a loop at line " + std::to_string(m_line));
    }
    consume(TokenType::CONTINUE, "Expected 'continue'");

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after continue statement");

    auto continueNode = std::make_unique<ContinueStatementNode>();
    
    return continueNode;
}

// Parses a for statement 
std::unique_ptr<ASTNode> Parser::parseForStatement() {
    loopContextStack.push_back("for");

    consume(TokenType::FOR, "Expected 'for'");
    
    consume(TokenType::LPAREN, "Expected '(' after 'for'");

    // Parse the iterator variable name
    Token iteratorTok = consume(TokenType::IDENTIFIER, "Expected iterator variable name after 'for'");

    consume(TokenType::IN, "Expected 'in' after iterator name in for statement");

    // Parse the iterable expression (e.g., 'arr' or '1')
    auto iterableExpr = parseExpression();

    // Check for optional '..'
    std::unique_ptr<ExpressionStatementNode> rangeEndExpr = nullptr;
    if (match(TokenType::DOTS)) { // if the next token is '..'
        // parse the second expression, e.g. '5'

        rangeEndExpr = parseExpression();
        
    }

    consume(TokenType::RPAREN, "Expected ')' after for(...) expression(s)");

    // Parse the body (a block of statements)
    std::vector<std::unique_ptr<ASTNode>> body;
    std::cout << "[DEBUG FOR statement] Parsed range end expression: " << rangeEndExpr->toString() << "\n";
    parseBlockStatementList(body);
    std::cout << "[DEBUG FOR statement] Parsed body " << "\n";
    // Build the ForStatementNode
    auto forNode = std::make_unique<ForStatementNode>();
    forNode->iteratorName = iteratorTok.text;
    forNode->iterableExpression = std::move(iterableExpr);
    forNode->rangeEndExpr = std::move(rangeEndExpr);
    forNode->body = std::move(body);

    loopContextStack.pop_back();

    return forNode;
}


std::unique_ptr<ASTNode> Parser::parseTryExceptThenStatement() {
    consume(TokenType::TRY, "Expected 'try'");
    std::vector<std::unique_ptr<ASTNode>> tryBlock;
    parseBlockStatementList(tryBlock);

    bool hasCatch = false;
    bool hasFinally = false;
    std::string catchName;
    std::vector<std::unique_ptr<ASTNode>> catchBlock;
    std::vector<std::unique_ptr<ASTNode>> finallyBlock;

    if (check(TokenType::EXCEPT)) {
        hasCatch = true;
        advance(); // consume 'except'
        consume(TokenType::LPAREN, "Expected '(' after 'except'");
        Token idTok = consume(TokenType::IDENTIFIER, "Expected identifier in except(...)");
        catchName = idTok.text;
        consume(TokenType::RPAREN, "Expected ')' after except identifier");
        parseBlockStatementList(catchBlock);
    }

    if (check(TokenType::THEN)) {
        hasFinally = true;
        advance(); // consume 'then'
        parseBlockStatementList(finallyBlock);
    }

    auto node = std::make_unique<TryExceptThenNode>();
    node->tryBlock = std::move(tryBlock);
    node->hasCatch = hasCatch;
    node->catchIdentifier = catchName;
    node->catchBlock = std::move(catchBlock);
    node->hasFinally = hasFinally;
    node->finallyBlock = std::move(finallyBlock);

    return node;
}

std::unique_ptr<ASTNode> Parser::parseFunctionDeclaration() {
    consume(TokenType::FUNCTION, "Expected 'function' keyword");
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name after 'function'");

    consume(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<Parameter> parameters;

    // Parse parameters
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
            std::string paramType;

            if (check(TokenType::COLON)) {
                advance(); // Consume ':'
                Token typeToken = consume(TokenType::IDENTIFIER, "Expected parameter type after ':'");
                paramType = typeToken.text;
            }

            parameters.push_back({paramName.text, paramType});
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "Expected ')' after parameter list");

    // Optional return type
    std::string returnType;
    if (check(TokenType::COLON)) {
        advance(); // Consume ':'
        Token typeToken = consume(TokenType::IDENTIFIER, "Expected return type after ':'");
        returnType = typeToken.text;
    }

    // Parse the function body
    std::vector<std::unique_ptr<ASTNode>> body;
    parseBlockStatementList(body);

    return std::make_unique<FunctionDeclarationNode>(nameToken.text, parameters, returnType, std::move(body));
}

// Parses a return statement 
std::unique_ptr<ASTNode> Parser::parseReturnStatement() {
    consume(TokenType::RETURN, "Expected 'return'");

    std::unique_ptr<ExpressionStatementNode> retExpr = nullptr;
    // If the next token is not the statement terminator, parse an expression
    if (!check(TokenType::STMT_TERMINATOR)) {
        retExpr = parseExpression();
    }

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after return statement");

    auto returnNode = std::make_unique<ReturnStatementNode>();
    returnNode->expr = std::move(retExpr);

    return returnNode;
}

// Parses a subscribe statement 
std::unique_ptr<ASTNode> Parser::parseSubscribeStatement() {
    consume(TokenType::SUBSCRIBE, "Expected 'subscribe'");

    Token functionNameToken = consume(TokenType::IDENTIFIER, "Expected function name after 'subscribe'");

    consume(TokenType::TO, "Expected 'to' after 'subscribe' keyword");

    Token variableNameToken = consume(TokenType::IDENTIFIER, "Expected variable name after 'subscribe to'");

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after return statement");

    auto subscribeNode = std::make_unique<SubscribeStatementNode>();
    subscribeNode->functionName = functionNameToken.text;
    subscribeNode->variableName = variableNameToken.text;

    return subscribeNode;
}

// Parses a unsubscribe statement 
std::unique_ptr<ASTNode> Parser::parseUnsubscribeStatement() {
    consume(TokenType::UNSUBSCRIBE, "Expected 'unsubscribe'");

    Token functionNameToken = consume(TokenType::IDENTIFIER, "Expected function name after 'unsubscribe'");

    consume(TokenType::FROM, "Expected 'from' after 'unsubscribe' keyword");

    Token variableNameToken = consume(TokenType::IDENTIFIER, "Expected variable name after 'unsubscribe from'");

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after return statement");

    auto unsubscribeNode = std::make_unique<UnsubscribeStatementNode>();
    unsubscribeNode->functionName = functionNameToken.text;
    unsubscribeNode->variableName = variableNameToken.text;

    return unsubscribeNode;
}

std::unique_ptr<ASTNode> Parser::parseClassDeclaration() {
    consume(TokenType::OBJECT, "Expected 'object'");
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected class name");

    std::string parentName;
    if (check(TokenType::DERIVES)) {
        advance(); // consume 'derives'
       
        consume(TokenType::FROM, "Expected 'from' after 'derives' keyword");
       
        Token parentToken = consume(TokenType::IDENTIFIER, "Expected parent class name after 'derives from' keyword");
        parentName = parentToken.text;
    }
    std::vector<std::unique_ptr<ASTNode>> emptyMembers;

    auto clsNode = std::make_unique<ClassDeclarationNode>(nameToken.text, parentName, std::move(emptyMembers));

    parseClassBody(*clsNode);
    return clsNode;
}

void Parser::parseClassBody(ClassDeclarationNode& classNode) {
    consume(TokenType::STMT_TERMINATOR, "Expected '\\' to start class body");

    while (!check(TokenType::STMT_TERMINATOR) && !isAtEnd()) {
        if (check(TokenType::DEFINE)) {
            classNode.members.push_back(parseDefineStatement());
        }
        else if (check(TokenType::DERIVE)) {
            classNode.members.push_back(parseDeriveStatement());
        }
        else if (check(TokenType::FUNCTION)) {
            classNode.members.push_back(parseFunctionDeclaration());
        }
        else {
            // Optionally allow: subscribe/unsubscribe, etc., if valid in class
            throw std::runtime_error("Unexpected token in class body: " + peek().text);
        }
    }

    consume(TokenType::STMT_TERMINATOR, "Expected '\\' to end class body");
    
}


std::unique_ptr<ASTNode> Parser::parseThrowStatement() {
    consume(TokenType::THROW, "Expected 'throw'");

    std::unique_ptr<ExpressionStatementNode> throwExpr = nullptr;
    if (!check(TokenType::STMT_TERMINATOR)) {
        throwExpr = parseExpression();
    }

    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' after throw statement");

    auto throwNode = std::make_unique<ThrowStatementNode>();
    throwNode->expression = std::move(throwExpr);

    return throwNode;
}


// Parses a block of statements, enclosed by statement terminators
void Parser::parseBlockStatementList(std::vector<std::unique_ptr<ASTNode>>& stmts) {
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' to start block");
    while (!check(TokenType::STMT_TERMINATOR) && !check(TokenType::END_OF_FILE)) {
        if (auto stmt = parseStatement()) {
            stmts.push_back(std::move(stmt));
        } else {
            break;
        }
    }
    consume(TokenType::STMT_TERMINATOR, "Expected '\\\\' to end block");
}

// Utility Methods

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::checkNext(TokenType type) const {
    if (m_pos + 1 >= m_tokens.size()) return false;
    return m_tokens[m_pos + 1].type == type;
}

bool Parser::checkNextNext(TokenType type) const {
    if (m_pos + 2 >= m_tokens.size()) return false;
    return m_tokens[m_pos + 2].type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& errMsg) {
    if (check(type)) {
        return advance();
    }
    auto& badToken = peek();
    std::string fullMsg = errMsg + " (line " + std::to_string(badToken.line) +
        ", col " + std::to_string(badToken.column) + "). Found: '" + badToken.text + "'";
    recordError(fullMsg);
    // synchronize();
    return Token{TokenType::ERROR, "<error>", (int)m_pos, badToken.line, badToken.column};
}

Token Parser::advance() {
    if (!isAtEnd()) m_pos++;
    return m_tokens[m_pos - 1];
}

Token Parser::previous() const {
    if (m_pos == 0) {
        throw std::runtime_error("No previous token exists.");
    }
    return m_tokens[m_pos - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    return m_tokens[m_pos];
}

bool Parser::matchNumberLiteral() {
    if (!isAtEnd()) {
        const Token& t = peek();
        if (t.type == TokenType::NUMBER_LITERAL) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::recordError(const std::string& message) {
    m_errors.push_back(message);
}

void Parser::synchronize() {
    int nestingLevel = 0;
    while (!isAtEnd()) {
        switch (peek().type) {
            case TokenType::HASH:
                nestingLevel++;
                advance();
                break;
            case TokenType::BACKSLASH_HASH:
                nestingLevel--;
                advance();
                if (nestingLevel <= 0) return;
                break;
            case TokenType::STMT_TERMINATOR:
                advance();
                if (nestingLevel == 0) return;  // Stop after one statement
                break;
            default:
                advance();
        }
    }
}

// ------------------- ExpressionParser Nested Class Implementations -------------------

// Parses an expression and returns an ExpressionStatementNode
std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenType::OR)) {
        Token opTok = previous();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpressionStatementNode>(
            opTok, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseLogicalAnd() {
    auto left = parseEquality();
    while (match(TokenType::AND)) {
        Token opTok = previous();
        auto right = parseEquality();
        left = std::make_unique<BinaryExpressionStatementNode>(
            opTok, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        Token opTok = advance();
        auto right = parseComparison();
        left = std::make_unique<BinaryExpressionStatementNode>(
            opTok, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseComparison() {
    auto left = parseAddition();
    while (true) {
        if (match(TokenType::LT) || match(TokenType::LTEQ) ||
            match(TokenType::GT) || match(TokenType::GTEQ)) {
            Token opTok = previous();
            auto right = parseAddition();
            left = std::make_unique<BinaryExpressionStatementNode>(
                opTok, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseAddition() {
    auto left = parseMultiplication();
        while (true) {
        if (match(TokenType::PLUS) || match(TokenType::MINUS)) {
            Token opTok = previous();
            auto right = parseMultiplication();
            left = std::make_unique<BinaryExpressionStatementNode>(
                opTok, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseMultiplication() {
    auto left = parseUnary();
    while (true) {
        if (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MODULUS)) {
            Token opTok = previous();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpressionStatementNode>(
                opTok, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseUnary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        Token opTok = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpressionStatementNode>(
            opTok, std::move(right));
    }
    return parsePrimary();
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parsePrimary() {
    if (match(TokenType::IDENTIFIER)) {
        Token identifierToken = previous();
        std::cout << "[DEBUG] Parsing identifier: " << identifierToken.text << "\n";

        if (check(TokenType::LPAREN)) {
            // parseFunctionCall can do the real work
            return parseFunctionCall(identifierToken);
        }

        // Start with the base variable
        std::unique_ptr<ExpressionStatementNode> expr;
        expr = std::make_unique<VariableExpressionStatementNode>(identifierToken);

        // Handle chained accesses (dot, subscript)
        while (true) {
            if (match(TokenType::DOT)) {
                // Property access or method call
                Token memberToken = m_parentParser.consume(TokenType::IDENTIFIER, "Expected property or method name after '.'");
                if (match(TokenType::LPAREN)) {
                    // Method Call
                    std::vector<std::unique_ptr<ExpressionStatementNode>> methodArgs;
                    if (!m_parentParser.check(TokenType::RPAREN)) {
                        do {
                            methodArgs.push_back(parseExpression());
                        } while (match(TokenType::COMMA));
                    }
                    m_parentParser.consume(TokenType::RPAREN, "Expected ')' after method arguments.");

                    expr = std::make_unique<ObjectMethodCallExpressionNode>(
                        std::move(expr),
                        memberToken.text,
                        std::move(methodArgs)
                    );

                    std::cout << "[DEBUG] Parsed method call: " << memberToken.text << "\n";
                }
                else {
                    // Property Access
                    expr = std::make_unique<ObjectPropertyAccessExpressionNode>(
                        std::move(expr),
                        memberToken.text
                    );

                    std::cout << "[DEBUG] Parsed property access: " << memberToken.text << "\n";
                }
            }
            else if (match(TokenType::LBRACKET)) {
                // Subscript Access
                std::unique_ptr<ExpressionStatementNode> indexExpr = parseExpression();
                m_parentParser.consume(TokenType::RBRACKET, "Expected ']' after subscript.");

                expr = std::make_unique<SubscriptExpressionStatementNode>(
                    std::move(expr),
                    std::move(indexExpr)
                );

                std::cout << "[DEBUG] Parsed subscript access.\n";
            }
            else {
                // No further chaining
                break;
            }
        }

        return expr;
    }
    if (match(TokenType::LBRACKET)) {
        return parseArrayLiteral();
    }
    if (match(TokenType::LBRACE)) {
        return parseDictionaryLiteral();
    }
    if (match(TokenType::BOOLEAN_LITERAL)) {
        Token boolTok = previous();
        bool value = (boolTok.text == "true");  // Convert string to boolean value
        return std::make_unique<BooleanLiteralExpressionStatementNode>(value);
    }
    if (match(TokenType::STRING_LITERAL)) {
        Token strTok = previous();

        std::vector<std::unique_ptr<ExpressionStatementNode>> parts;
        size_t pos = 0;

        while (pos < strTok.text.size()) {
            size_t varStart = strTok.text.find("#(", pos);

            // Literal before embedded expression
            if (varStart != std::string::npos) {
                if (varStart > pos) {
                    parts.push_back(std::make_unique<StringLiteralExpressionStatementNode>(
                        Token{TokenType::STRING_LITERAL, strTok.text.substr(pos, varStart - pos)}));
                }

                size_t varEnd = strTok.text.find(")", varStart);
                if (varEnd == std::string::npos) {
                    throw std::runtime_error("Unmatched #( in composite string");
                }

                // Parse embedded variable
                auto embeddedExpr = parseEmbeddedString();
                parts.push_back(std::move(embeddedExpr));

                pos = varEnd + 1;
            } else {
                // Remaining literal
                parts.push_back(std::make_unique<StringLiteralExpressionStatementNode>(
                    Token{TokenType::STRING_LITERAL, strTok.text.substr(pos)}));
                break;
            }
        }

        if (parts.size() == 1) {
            return std::move(parts[0]);
        }
        return std::make_unique<CompositeStringExpressionStatementNode>(std::move(parts));
    }

    if (matchNumberLiteral()) {
        Token tk = previous();
        return std::make_unique<NumberLiteralExpressionStatementNode>(tk);
    }
    if (match(TokenType::LPAREN)) {
  
        std::cout << "Parsing expression inside parens... \n";  
        
        auto expr = parseExpression();
  
        if (!match(TokenType::RPAREN)) {
            throw std::runtime_error("Expected ')' after expression");
        }
        m_parentParser.consume(TokenType::RPAREN, "Expected ')' to close expression");
        std::cout << "Parsing expression inside parens " << expr->toString() << "\n";
        return expr;
    }
    // Error handling
    Token bad = peek();
    throw std::runtime_error("Unexpected token '" + bad.text + "' in expression parsePrimary");
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseEmbeddedString() {
    m_parentParser.consume(TokenType::HASH, "Expected '#' for embedded string");
    m_parentParser.consume(TokenType::LPAREN, "Expected '(' after '#' for embedded string");

    Token varToken = m_parentParser.consume(TokenType::IDENTIFIER, "Expected variable name inside embedded string");

    m_parentParser.consume(TokenType::RPAREN, "Expected ')' to close embedded string");

    return std::make_unique<EmbeddedVariableExpressionStatementNode>(
        std::make_unique<VariableExpressionStatementNode>(varToken));
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseFunctionCall(const Token& nameToken) {

    m_parentParser.consume(TokenType::LPAREN, "Expected '(' after function name");

    std::vector<std::unique_ptr<ExpressionStatementNode>> arguments;

    // Parse arguments
    if (!check(TokenType::RPAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }

    m_parentParser.consume(TokenType::RPAREN, "Expected ')' after function arguments");

    return std::make_unique<FunctionCallExpressionStatementNode>(
        nameToken.text, std::move(arguments));
}

std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseArrayLiteral() {
    // We matched '['
    std::vector<std::unique_ptr<ExpressionStatementNode>> elements;

    if (!match(TokenType::RBRACKET)) {
        do {
            auto elemExpr = parseExpression();
            elements.push_back(std::move(elemExpr));
        } while (match(TokenType::COMMA));
    }

    m_parentParser.consume(TokenType::RBRACKET, "Expected ']' after array literal");
    return std::make_unique<ArrayLiteralExpressionStatementNode>(std::move(elements));
}
std::unique_ptr<ExpressionStatementNode> Parser::ExpressionParser::parseDictionaryLiteral() {
    // We matched '{'
    std::vector<DictionaryEntry> dictEntries;

    if (!match(TokenType::RBRACE)) {
        do {
            // key can be a STRING_LITERAL or IDENTIFIER
            Token keyToken = peek();
            if (keyToken.type != TokenType::STRING_LITERAL &&
                keyToken.type != TokenType::IDENTIFIER)
            {
                throw std::runtime_error("Dictionary key must be string literal or identifier");
            }
            advance(); // consume the key token

            m_parentParser.consume(TokenType::COLON, "Expected ':' after dictionary key");

            // parse the value expression
            auto valueExpr = parseExpression();

            dictEntries.push_back({ keyToken, std::move(valueExpr) });
        } while (match(TokenType::COMMA));
    }

    m_parentParser.consume(TokenType::RBRACE, "Expected '}' after dictionary literal");
    return std::make_unique<DictionaryLiteralExpressionStatementNode>(std::move(dictEntries));
}


// Utility Methods for ExpressionParser

bool Parser::ExpressionParser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::ExpressionParser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::ExpressionParser::advance() {
    if (!isAtEnd()) m_posRef++;
    return m_tokens[m_posRef - 1];
}

Token Parser::ExpressionParser::previous() const {
    return m_tokens[m_posRef - 1];
}

bool Parser::ExpressionParser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::ExpressionParser::peek() const {
    return m_tokens[m_posRef];
}

bool Parser::ExpressionParser::matchNumberLiteral() {
    if (!isAtEnd() && peek().type == TokenType::NUMBER_LITERAL) {
        advance();
        return true;
    }
    return false;
}
