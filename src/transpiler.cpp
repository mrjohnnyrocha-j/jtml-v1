#include "../include/transpiler.h"
#include <sstream>
#include <iostream>
#include <fstream>

JtmlTranspiler::JtmlTranspiler() {
    // Initialize any required state for the transpiler
}
std::string JtmlTranspiler::transpile(const std::vector<std::unique_ptr<ASTNode>>& program) {
    uniqueElemId = 0;
    uniqueVarId  = 0;
    nodeID = 0; 

    std::ostringstream out;
    out << "<!DOCTYPE html>\n<html>\n<head>\n"
        << "  <meta charset=\"utf-8\">\n"
        << "  <title>JTML Final Example</title>\n"
        << "</head>\n<body>\n";

    // Top-level statements => insideElement=false
    for (auto& node : program) {
        out << transpileNode(*node, /*insideElement=*/false);
    }

    out << generateScriptBlock();
    out << "\n</body>\n</html>\n";
    return out.str();
}

//--------------------------------------------------
// Distinguish node type and top-level vs. inside-element
//--------------------------------------------------
std::string JtmlTranspiler::transpileNode(const ASTNode& node, bool insideElement) {
    switch (node.getType()) {
    case ASTNodeType::JtmlElement:
        // For an element, we always do transpileElement
        return transpileElement(static_cast<const JtmlElementNode&>(node));

    case ASTNodeType::IfStatement: {
        const auto& ifNode = static_cast<const IfStatementNode&>(node);
        return insideElement ? 
            transpileIfInsideElement(ifNode) :
            transpileIfTopLevel(ifNode);
    }
    case ASTNodeType::ForStatement: {
        const auto& forNode = static_cast<const ForStatementNode&>(node);
        return insideElement ? 
            transpileForInsideElement(forNode) :
            transpileForTopLevel(forNode);
    }
    case ASTNodeType::WhileStatement: {
        const auto& whileNode = static_cast<const WhileStatementNode&>(node);
        return insideElement ?
            transpileWhileInsideElement(whileNode) :
            transpileWhileTopLevel(whileNode);
    }
    case ASTNodeType::ShowStatement:
        return transpileShow(static_cast<const ShowStatementNode&>(node));

    // For define, function, class, etc. we might produce minimal placeholders
    // or skip
    default:
        return "<!-- " + node.toString() + " not explicitly transpiled. -->\n";
    }
}

//--------------------------------------------------
// Transpile an element (with attributes + content)
//--------------------------------------------------
std::string JtmlTranspiler::transpileElement(const JtmlElementNode& elem) {
    ++uniqueElemId;
    nodeID++;
    std::string domId = "elem_" + std::to_string(uniqueElemId);

    std::ostringstream out;
    out << "<" << elem.tagName << " id=\"" << domId << "\"";

    // Transpile attributes, adding reactivity for expressions
    for (auto& attr : elem.attributes) {
        std::string valStr = attr.value->toString();
            
       if (attr.key == "onClick" || attr.key == "onInput" || attr.key == "onMouseOver" || attr.key == "onScroll") {
            ++uniqueVarId;
            std::string derivedVarName = "attr_" + std::to_string(uniqueVarId);

            // Store in nodeDerivedMap
            nodeDerivedMap[nodeID][attr.key] = derivedVarName;
            // Ensure that the attribute value represents the JTML function or handler
            std::string functionCall = escapeJS(attr.value->toString());

            // Handle special cases for events with arguments (e.g., onInput, onScroll)
            std::ostringstream args;
            if (attr.key == "onInput") {
                args << ", event.target.value"; // Pass the input value as an argument
            } else if (attr.key == "onScroll") {
                args << ", 'window.scrollY'"; // Pass the scroll position as an argument
            }

            // Generate the event handler
            out << " " << attr.key << "=\"sendEvent('" << derivedVarName << "', '" << attr.key << "', ['" << functionCall << "'" << args.str() << "])\"";
        
        } else {
            ++uniqueVarId;
            std::string derivedVarName = "attr_" + std::to_string(uniqueVarId);

            // Store in nodeDerivedMap
            nodeDerivedMap[nodeID][attr.key] = derivedVarName;

            // Add a data attribute for the front-end to identify
            out << " data-jtml-attr-" << attr.key << "=\"" << derivedVarName << "\"";
        }
    }

    out << ">";
    // Child statements => insideElement=true
    out << transpileChildren(elem.content, /*insideElement=*/true);
    out << "</" << elem.tagName << ">\n";

    return out.str();
}

//--------------------------------------------------
// If top-level => minimal or comment
//--------------------------------------------------
std::string JtmlTranspiler::transpileIfTopLevel(const IfStatementNode& node) {
    return "<!-- IfStatement at top-level: server logic only -->\n";
}

//--------------------------------------------------
// If inside an element => data-jtml-if
//--------------------------------------------------
std::string JtmlTranspiler::transpileIfInsideElement(const IfStatementNode& node) {
    ++uniqueVarId;
    nodeID++;
    std::string condName = "cond_" + std::to_string(uniqueVarId);
    nodeDerivedMap[nodeID]["if"] = condName;

    // transpile "then" block
    std::ostringstream thenSS;
    for (auto& stmt : node.thenStatements) {
        thenSS << transpileNode(*stmt, /*insideElement=*/true);
    }
    std::string thenHTML = escapeHTML(thenSS.str());

    // transpile "else" block
    std::string elseHTML;
    if (!node.elseStatements.empty()) {
        std::ostringstream elseSS;
        for (auto& stmt : node.elseStatements) {
            elseSS << transpileNode(*stmt, /*insideElement=*/true);
        }
        elseHTML = escapeHTML(elseSS.str());
    }

    std::ostringstream out;
    out << "<div data-jtml-if=\"" << condName << "\" "
        << "data-then=\"" << thenHTML << "\" "
        << "data-else=\"" << elseHTML << "\">"
        << "</div>\n";

    return out.str();
}

//--------------------------------------------------
// For top-level => minimal
//--------------------------------------------------
std::string JtmlTranspiler::transpileForTopLevel(const ForStatementNode& node) {
    return "<!-- ForStatement at top-level: server logic only -->\n";
}

//--------------------------------------------------
// For inside element => data-jtml-for
//--------------------------------------------------
std::string JtmlTranspiler::transpileForInsideElement(const ForStatementNode& node) {
    // 1) Generate a unique name for the loop's iterable (to store in the environment or for the front-end)
    ++uniqueVarId;
    nodeID++;
    std::string rangeName = "range_" + std::to_string(uniqueVarId);

    // 2) Register in nodeDerivedMap
    //    "for" => the derived variable for the iterable
    //    "iteratorName" => node.iteratorName
    nodeDerivedMap[nodeID]["for"] = rangeName;
    nodeDerivedMap[nodeID]["iteratorName"] = node.iteratorName;

    // 3) Transpile the loop body statements
    std::ostringstream bodyStream;
    for (auto& stmt : node.body) {
        // Recursively transpile each child statement inside the loop
        bodyStream << transpileNode(*stmt, /*insideElement=*/true);
    }
    std::string escapedBody = escapeHTML(bodyStream.str());

    // 4) Produce final HTML with data-jtml-for and data-jtml-iterator
    std::ostringstream out;
    out << "<div data-jtml-for=\"" << rangeName 
        << "\" data-jtml-iterator=\"" << node.iteratorName
        << "\" data-body=\"" << escapedBody << "\"></div>\n";

    return out.str();
}

//--------------------------------------------------
// While top-level => minimal
//--------------------------------------------------
std::string JtmlTranspiler::transpileWhileTopLevel(const WhileStatementNode& node) {
    return "<!-- WhileStatement at top-level: server logic only -->\n";
}

//--------------------------------------------------
// While inside element => data-jtml-while
//--------------------------------------------------
std::string JtmlTranspiler::transpileWhileInsideElement(const WhileStatementNode& node) {
    ++uniqueVarId;
    nodeID++;
    std::string condName = "cond_" + std::to_string(uniqueVarId);

    nodeDerivedMap[nodeID]["while"] = condName;

    std::ostringstream bodySS;
    for (auto& stmt : node.body) {
        bodySS << transpileNode(*stmt, /*insideElement=*/true);
    }
    std::string bodyHTML = escapeHTML(bodySS.str());

    std::ostringstream out;
    out << "<div data-jtml-while=\"" << condName << "\" "
        << "data-body=\"" << bodyHTML << "\">"
        << "</div>\n";
    return out.str();
}

//--------------------------------------------------
// Show => produce placeholders
//--------------------------------------------------
std::string JtmlTranspiler::transpileShow(const ShowStatementNode& node) {
    if(!node.expr) {
        return "<p><!-- show with no expr? --></p>\n";
    }
    ++uniqueVarId;
    nodeID++;
    std::string exprVarName = "expr_" + std::to_string(uniqueVarId);

    nodeDerivedMap[nodeID]["show"] = exprVarName;

    // produce placeholder
    // e.g. <p>{{someExpr}}</p>
    std::string placeholder = "{{" + node.expr->toString() + "}}";

    std::ostringstream out;
    out << "<div id=\""<< exprVarName << "\">" << placeholder << "</div>\n";
    return out.str();
}

//--------------------------------------------------
// Helper to transpile a list of child statements
//--------------------------------------------------
std::string JtmlTranspiler::transpileChildren(const std::vector<std::unique_ptr<ASTNode>>& children, bool insideElement) {
    std::ostringstream out;
    for (auto& c : children) {
        out << transpileNode(*c, insideElement);
    }
    return out.str();
}

//--------------------------------------------------
// Insert a minimal script
//--------------------------------------------------
std::string JtmlTranspiler::generateScriptBlock() {
    return R"(
  <script>
        const ws = new WebSocket('ws://localhost:8080');

        ws.onopen = () => {
            console.log('WebSocket connection established.');
        };

        ws.onmessage = (event) => {
            const message = JSON.parse(event.data);
            if (message.type === 'populateBindings') {
                const bindings = message.bindings;
                // Handle content bindings
                if (bindings.content) {
                    for (const [elementId, value] of Object.entries(bindings.content)) {
                        const elem = document.getElementById(elementId);
                        if (elem) {
                            elem.textContent = value;
                        }
                    }
                }
                // Handle attribute bindings
                if (bindings.attributes) {
                    for (const [elementId, attrs] of Object.entries(bindings.attributes)) {
                        const elem = document.getElementById(elementId);
                        if (elem) {
                            for (const [attr, value] of Object.entries(attrs)) {
                                elem.setAttribute(attr, value);
                            }
                        }
                    }
                }
            }
            else if (message.type === 'updateBinding') {
                const elementId = message.elementId;
                const value = message.value;
                const elem = document.getElementById(elementId);
                if (elem) {
                    elem.textContent = value;
                }
                if (message.type === 'attribute') {
                    const elem = document.getElementById(binding.elementId);
                    if (elem) {
                        elem.setAttribute(binding.attribute, binding.value);
                    }
                }
            }
            else if (message.type === 'acknowledgment') {
                console.log('Acknowledgment:', message.message);
            }
            else if (message.type === 'error') {
                console.error('Error from server:', message.error);
            }
        };

        ws.onclose = () => {
            console.log('WebSocket connection closed.');
        };

        // Function to send events to the server
        function sendEvent(elementId, eventType, args = []) {
            // Check if WebSocket connection is open
            if (ws && ws.readyState === WebSocket.OPEN) {
                const message = {
                    type: 'event',
                    elementId: elementId,
                    eventType: eventType,
                    args: args
                };
                try {
                    ws.send(JSON.stringify(message));
                    console.log(`[DEBUG] Sent event: ${JSON.stringify(message)}`);
                } catch (error) {
                    console.error(`[ERROR] Failed to send event: ${error.message}`);
                }
            } else {
                console.error(`[ERROR] WebSocket is not open. Event not sent: ElementID=${elementId}, EventType=${eventType}`);
            }
        }

      
    </script>
    )";
}

// Utility to escape HTML for data attributes and inner content
std::string JtmlTranspiler::escapeHTML(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch(c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '\"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:
                out += c;
        }
    }
    return out;
}

// Utility to escape JavaScript strings
std::string JtmlTranspiler::escapeJS(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch(c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '/': out += "\\/"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}