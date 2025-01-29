// jtml_interpreter.h
#pragma once

#include "jtml_ast.h"
#include "transpiler.h"
#include "jtml_parser.h"
#include "jtml_lexer.h" 
#include "jtml_value.h"
#include "Dict.h"
#include "Array.h"
#include "Environment.h"
#include "InstanceIDGenerator.h"
#include "jtml_value.h"  // so we can reference JTML::VarValue, ValueVariant, DictType
#include "Function.h"
#include "renderer.h"
#include "websocket_server.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "json.hpp"


namespace JTML = JTMLInterpreter;
// Interpreter class
class Interpreter {
public:
    explicit Interpreter(JtmlTranspiler& transpiler); // Constructor declaration
    ~Interpreter(); // Default destructor

    void handleFrontendMessage(const std::string& msg, websocketpp::connection_hdl hdl);
    void populateBindings(websocketpp::connection_hdl hdl);

    // Interpret methods
    void interpret(const JtmlElementNode&);
    void interpret(const std::vector<std::unique_ptr<ASTNode>>& program);
    void interpret(const std::string& code);

    std::shared_ptr<JTML::VarValue> evaluateExpression(const ExpressionStatementNode* exprNode, std::shared_ptr<JTML::Environment> env);

    
    double getNumericValue(const std::shared_ptr<JTML::VarValue>& valPtr);
    std::string getStringValue(const std::shared_ptr<JTML::VarValue>& valPtr);

    std::shared_ptr<JTML::Environment> getCurrentEnvironment() const;

    // Error handling
    
private:
    JtmlTranspiler& transpiler;
    std::shared_ptr<JTML::Environment> globalEnv;
    std::shared_ptr<JTML::Environment> currentEnv;
    bool inFunctionContext = false;

    std::thread wsThread;

    int nodeID;
    int uniqueVarID;

    int uniqueArrayVarID;
    int uniqueDictVarID ;

    std::unique_ptr<JTML::Renderer> renderer;
    std::shared_ptr<JTML::WebSocketServer> wsServer;
        
    std::unordered_map<std::string, std::shared_ptr<ClassDeclarationNode>> classDeclarations;
    
    // Function Execution
    std::shared_ptr<JTML::VarValue> executeFunction(
        const std::shared_ptr<JTML::Function>& func,
        const std::vector<std::shared_ptr<JTML::VarValue>>& args,
        const std::shared_ptr<JTML::VarValue>& thisPtr = nullptr
    );

    std::shared_ptr<JTML::VarValue> instantiateClass(
    const ClassDeclarationNode& classNode,
    const std::vector<std::unique_ptr<ExpressionStatementNode>>& arguments,
    std::shared_ptr<JTML::Environment> parentEnv
);

    // Dependency Tracking with Integer IDs

    void recalcDirty(std::shared_ptr<JTML::Environment> env);
    void updateVariable(JTML::VarID varID, std::shared_ptr<JTML::Environment> env);

    // Variable management methods
    void storeVariable(const std::string& scope, const std::string& varName);
    

    // Expression evaluation    
    bool evaluateCondition(const ExpressionStatementNode* condition, std::shared_ptr<JTML::Environment> env);
    void gatherDeps(const ExpressionStatementNode* exprNode, std::vector<JTML::CompositeKey>& out, std::shared_ptr<JTML::Environment> env);

    bool isTruthy(const std::string& value);
    bool performNumericCompare(const std::string& op, double ln, double rn);
    bool performStringCompare(const std::string& op, const std::string& ls, const std::string& rs);
    


    struct BreakException : public std::exception {};
    struct ContinueException : public std::exception {};
    
    // Interpretation methods
    void interpret(const ASTNode& node);
    void interpretNode(const ASTNode& node);
    void interpretElement(const JtmlElementNode& elem);
    void interpretElementAttributes(const JtmlElementNode& node);
    bool isEventAttribute(const std::string& attrName) const;
    std::string extractEventType(const std::string& attrName) const;
    bool containsExpression(const ExpressionStatementNode* exprNode) const;
    void interpretShowElement(const ShowStatementNode& node);
    void interpretIfElement(const IfStatementNode& node);
    void interpretWhileElement(const WhileStatementNode& node);
    void interpretForElement(const ForStatementNode& node);
    void interpretBlockStatement(const BlockStatementNode& block);
    void interpretShow(const ShowStatementNode& stmt);
    void interpretExpression(const ExpressionNode& node);
    void interpretDefine(const DefineStatementNode& stmt);
    void interpretAssignment(const AssignmentStatementNode& stmt);
    void interpretDerive(const DeriveStatementNode& stmt);
    void interpretUnbind(const UnbindStatementNode& stmt);
    void interpretStore(const StoreStatementNode& stmt);
    void interpretIf(const IfStatementNode& stmt);
    void interpretWhile(const WhileStatementNode& stmt);
    void interpretFor(const ForStatementNode& stmt);
    void interpretSubscribe(const SubscribeStatementNode& node);
    void interpretUnsubscribe(const UnsubscribeStatementNode& node);
    void interpretBreak(const BreakStatementNode& node);
    void interpretContinue(const ContinueStatementNode& node); 
    void interpretTryExceptThen(const TryExceptThenNode& stmt);
    void interpretReturn(const ReturnStatementNode& stmt);
    void interpretThrow(const ThrowStatementNode& stmt);
    void interpretFunctionDeclaration(const FunctionDeclarationNode& outerDecl);
    void interpretClassDeclaration(const ClassDeclarationNode& node);
    void collectAllNestedFunctions(
        const std::vector<std::unique_ptr<ASTNode>>& stmts,
        const std::shared_ptr<JTML::Environment>& closureEnv
    );
    void handleError(const std::string& message);
    
};
