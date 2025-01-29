#ifndef JTML_TRANSPILER_H
#define JTML_TRANSPILER_H

#include <string>
#include <vector>
#include <memory>
#include "jtml_ast.h"      // Where ASTNode, JtmlElementNode, etc. are declared

/**
 * A Transpiler that converts a JTML AST into HTML+JS placeholders.
 * 
 * Key Points:
 *  - Only conditionals/loops/show are allowed in an element node.
 *  - We produce {{varName}} placeholders for expressions,
 *    letting the Interpreter handle the logic.
 */
class JtmlTranspiler {
public:
    explicit JtmlTranspiler();
    std::unordered_map<int, std::unordered_map<std::string, std::string>> nodeDerivedMap;

    /**
     * Transpile a vector of AST nodes into a full HTML page string.
     */
    std::string transpile(const std::vector<std::unique_ptr<ASTNode>>& program);

private:
    int uniqueElemId = 0;
    int uniqueVarId  = 0;
    int nodeID = 0;

   

    // Internal dispatch
    std::string transpileNode(const ASTNode& node, bool insideElement);
    std::string transpileElement(const JtmlElementNode& elem);
    std::string transpileIfTopLevel(const IfStatementNode& node);
    std::string transpileIfInsideElement(const IfStatementNode& node);
    std::string transpileForTopLevel(const ForStatementNode& node);
    std::string transpileForInsideElement(const ForStatementNode& node);
    std::string transpileWhileTopLevel(const WhileStatementNode& node);
    std::string transpileWhileInsideElement(const WhileStatementNode& node);
    std::string transpileShow(const ShowStatementNode& node);


    // Helper for child nodes
    std::string transpileChildren(const std::vector<std::unique_ptr<ASTNode>>& children, bool insideElement);

    // Insert minimal <script> for placeholders
    std::string generateScriptBlock();
    std::string escapeHTML(const std::string& input);
    std::string escapeJS(const std::string& input);
};

#endif // JTML_TRANSPILER_H
