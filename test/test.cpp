#include "../include/jtml_lexer.h"
#include "../include/jtml_parser.h"
#include "../include/jtml_interpreter.h"
#include "../include/jtml_transpiler.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <memory>

// Helper function to capture interpreter output
std::string runInterpreter(const std::string& code) {
    std::ostringstream output;

    // Backup old stdout/stderr buffers
    std::streambuf* oldCoutBuf = std::cout.rdbuf(output.rdbuf());
    std::streambuf* oldCerrBuf = std::cerr.rdbuf(output.rdbuf());

    // --------------------------
    // 1) Show the raw JTML code
    // --------------------------
    std::cout << "=== RAW JTML CODE ===" << std::endl;
    std::cout << code << std::endl << std::endl;

    // -------------
    // 2) LEXER
    // -------------
    std::cout << "=== LEXER OUTPUT ===" << std::endl;
    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    for (const auto& tk : tokens) {
        std::cout << "[" << tokenTypeToString(tk.type)
                  << "] " << tk.text
                  << " (line: " << tk.line
                  << ", col: " << tk.column << ")\n";
    }
    // Show any lexer errors
    if (!lexer.getErrors().empty()) {
        std::cout << "\nLexer Errors:\n";
        for (const auto& err : lexer.getErrors()) {
            std::cout << err << std::endl;
        }
    }
    std::cout << std::endl;

    // -------------
    // 3) PARSER
    // -------------
    std::cout << "=== PARSER OUTPUT ===" << std::endl;
    Parser parser(std::move(tokens));
    auto ast = parser.parseProgram();
    // Show any parser errors
    if (!parser.getErrors().empty()) {
        std::cout << "\nParser Errors:\n";
        for (const auto& err : parser.getErrors()) {
            std::cout << err << std::endl;
        }
    }
    std::cout << "Parsed " << ast.size() << " top-level AST node(s).\n\n";

    // ---------------------
    // 4) INTERPRETER
    // ---------------------
    std::cout << "=== INTERPRETER OUTPUT ===" << std::endl;
    Interpreter interpreter;
    interpreter.interpret(ast);

    // ---------------------
    // 5) TRANSPILER (HTML)
    // ---------------------
    std::cout << "\n=== TRANSPILER OUTPUT (HTML) ===" << std::endl;
    JtmlTranspiler transpiler(interpreter);
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::JtmlElement) {
            const auto& elemNode = static_cast<const JtmlElementNode&>(*node);
            std::string html = transpiler.transpileHTML(elemNode);
            std::cout << html << "\n\n";
        }
    }

    std::cout << "=== END OF TEST ===" << std::endl;

    // Restore the original stream buffers
    std::cout.rdbuf(oldCoutBuf);
    std::cerr.rdbuf(oldCerrBuf);

    // Return everything that got captured in 'output'
    return output.str();
}

// Test cases

TEST(LexerTests, BasicLexing) {
    std::string code = R"JTML(define a = 1\\ show a\\)JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DEFINE] a = 1"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 1"), std::string::npos);
}

TEST(InterpreterTests, DeriveVariable) {
    std::string code = R"JTML(
        define a = 2\\
        define b = 4\\
        derive sum = a + b\\
        show sum\\
        a = 10\\
        show sum\\
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DEFINE] a = 2"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] b = 4"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] sum = 6"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 6"), std::string::npos);
    EXPECT_NE(output.find("[UPDATE] sum = 14"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 14"), std::string::npos);
}

TEST(InterpreterTests, CyclicDependency) {
    std::string code = R"JTML(
        define x = 1\\
        define y = x + 4\\
        derive z = x + y\\
        derive x = y + z\\
        show y\\
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("Interpreter Error: Cyclic dependency detected while propagating dirtiness for variable 'x'"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 5"), std::string::npos);
}

TEST(InterpreterTests, UnbindDerivedVariable) {
    std::string code = R"JTML(
        define a = 2\\
        define b = 4\\
        derive sum = a + b\\
        show sum\\
        a = 10\\
        show sum\\
        unbind sum\\
        define a = 20\\
        show sum\\
    )JTML";

    std::string output = runInterpreter(code);

    // Ensure expected behavior before unbinding
    EXPECT_NE(output.find("[DEFINE] a = 2"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] b = 4"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] sum = 6"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 6"), std::string::npos);
    EXPECT_NE(output.find("[UPDATE] sum = 14"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 14"), std::string::npos);
    EXPECT_NE(output.find("[UNBIND] sum"), std::string::npos);

    // Check that no further updates occur after unbinding
    size_t unbindIndex = output.find("[UNBIND] sum");
    ASSERT_NE(unbindIndex, std::string::npos);

    // Ensure no "[UPDATE] sum =" occurs after the unbind
    EXPECT_EQ(output.find("[UPDATE] sum =", unbindIndex), std::string::npos);
}


TEST(InterpreterTests, RedefineDerivedWithNoDependencies) {
    std::string code = R"JTML(
        derive constant = 42\\
        show constant\\
        derive constant = 50\\  
        show constant\\
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DERIVE] constant = 42"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 42"), std::string::npos);
    EXPECT_NE(output.find("[REDEFINE] constant as Derived"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] constant = 50"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 50"), std::string::npos);
}

TEST(InterpreterTests, PreventRedefineDerivedWithDependencies) {
    std::string code = R"JTML(
        define x = 10\\
        derive dependent = x + 5\\
        derive dependent = 50\\ 
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DERIVE] dependent = 15"), std::string::npos);
    EXPECT_NE(output.find("Cannot redefine derived variable 'dependent' with existing dependencies."), std::string::npos);
}


TEST(InterpreterTests, ReactivityTest) {
    std::string code = R"JTML(
        define x = 2\\
        derive y = x + 3\\
        derive z = x + y\\
        show z\\
        x = 5\\
        show z\\
    )JTML";

    std::string output = runInterpreter(code);

    // Ensure definitions and derivation are correct
    EXPECT_NE(output.find("[DEFINE] x = 2"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] y = 5"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] z = 7"), std::string::npos);

    // Ensure initial show works as expected
    size_t firstShowIndex = output.find("[SHOW] 7");
    EXPECT_NE(firstShowIndex, std::string::npos);

    // Ensure reactivity occurs after assignment
    size_t assignIndex = output.find("[ASSIGN] x = 5");
    ASSERT_NE(assignIndex, std::string::npos);  // Ensure assignment happens

    // [UPDATE] z = 13 should occur after x is assigned to 5
    size_t updateIndex = output.find("[UPDATE] z = 13", assignIndex);
    EXPECT_NE(updateIndex, std::string::npos);

    // [SHOW] 13 should occur after the update
    size_t secondShowIndex = output.find("[SHOW] 13", updateIndex);
    EXPECT_NE(secondShowIndex, std::string::npos);
}

TEST(InterpreterTests, FalseReactivityTest) {
    std::string code = R"JTML(
        define x = 2\\
        define y = x + 3\\
        derive z = x + y\\
        show z\\
        x = 5\\
        show z\\
    )JTML";

    std::string output = runInterpreter(code);

    // Ensure definitions and derivation are correct
    EXPECT_NE(output.find("[DEFINE] x = 2"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] y = 5"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] z = 7"), std::string::npos);

    // Ensure initial show works as expected
    size_t firstShowIndex = output.find("[SHOW] 7");
    EXPECT_NE(firstShowIndex, std::string::npos);

    // Ensure reactivity occurs after assignment
    size_t assignIndex = output.find("[ASSIGN] x = 5");
    ASSERT_NE(assignIndex, std::string::npos);  // Ensure assignment happens

    // [UPDATE] z = 13 should occur after x is assigned to 5
    size_t updateIndex = output.find("[UPDATE] z = 10", assignIndex);
    EXPECT_NE(updateIndex, std::string::npos);

    // [SHOW] 13 should occur after the update
    size_t secondShowIndex = output.find("[SHOW] 10", updateIndex);
    EXPECT_NE(secondShowIndex, std::string::npos);
}

TEST(InterpreterTests, ComplexExpression) {
    std::string code = R"JTML(
        define a = 5\\
        derive b = a * (a - 2)\\
        show b\\
        a = 10\\
        show b\\
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DERIVE] b = 15"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 15"), std::string::npos);
    EXPECT_NE(output.find("[UPDATE] b = 80"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 80"), std::string::npos);
}

TEST(InterpreterTests, UnaryExpressionTest) {
    std::string code = R"JTML(
        define x = 5\\
        define negX = -x\\
        define notX = !x\\
        show negX\\
        show notX\\
    )JTML";

    std::string output = runInterpreter(code);

    // Check for definitions
    EXPECT_NE(output.find("[DEFINE] x = 5"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] negX = -5"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] notX = 0"), std::string::npos);

    // Check for shows
    EXPECT_NE(output.find("[SHOW] -5"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 0"), std::string::npos);
}

TEST(InterpreterTests, MultipleDerivedChainTest) {
    std::string code = R"JTML(
        define a = 2\\
        derive b = a * 3\\
        derive c = b + 4\\
        show c\\  
        a = 5\\
        show c\\ 
    )JTML";

    std::string output = runInterpreter(code);

    // Initial definitions
    EXPECT_NE(output.find("[DEFINE] a = 2"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] b = 6"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] c = 10"), std::string::npos);
    // First show
    EXPECT_NE(output.find("[SHOW] 10"), std::string::npos);

    // Assignment
    EXPECT_NE(output.find("[ASSIGN] a = 5"), std::string::npos);
    // b and c should update
    EXPECT_NE(output.find("[UPDATE] b = 15"), std::string::npos);
    EXPECT_NE(output.find("[UPDATE] c = 19"), std::string::npos);
    // Final show
    EXPECT_NE(output.find("[SHOW] 19"), std::string::npos);
}


TEST(InterpreterTests, RedeclareVariableAsDerivedTest) {
    std::string code = R"JTML(
        define base = 10\\
        show base\\
        derive base = base + 5\\
        show base\\
        base = 20\\
        show base\\
    )JTML";

    std::string output = runInterpreter(code);

    // Initially, base is a normal variable = 10
    EXPECT_NE(output.find("[DEFINE] base = 10"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 10"), std::string::npos);

    // Redeclare as derived
    EXPECT_NE(output.find("[REDEFINE] base as Derived"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] base = 15"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 15"), std::string::npos);

    // Reassign base -> it’s still derived, so check logic
    EXPECT_NE(output.find("[ASSIGN] base = 20"), std::string::npos);
    // If your logic disallows reassigning a derived variable with dependencies, it might fail or revert
    // If it's allowed (no dependencies?), we might see [SHOW] 20 or a conflict
    // Expect either a redefinition or error handling
}

TEST(InterpreterTests, UnbindMultipleVariablesTest) {
    std::string code = R"JTML(
        define x = 3\\
        define y = 2\\
        derive sum = x + y\\
        derive product = x * y\\
        show sum\\
        show product\\
        unbind sum\\
        x = 10\\
        show sum\\ 
        show product\\ 
    )JTML";

    std::string output = runInterpreter(code);

    EXPECT_NE(output.find("[DEFINE] x = 3"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] y = 2"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] sum = 5"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] product = 6"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 5"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 6"), std::string::npos);
    EXPECT_NE(output.find("[UNBIND] sum"), std::string::npos);
    EXPECT_NE(output.find("[ASSIGN] x = 10"), std::string::npos);
    // sum remains old value => 5
    EXPECT_NE(output.find("[SHOW] 5"), std::string::npos);
    // product updates => 10 * 2 = 20
    EXPECT_NE(output.find("[UPDATE] product = 20"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 20"), std::string::npos);
}

TEST(InterpreterTests, ComplexBooleanExpressionsTest) {
    std::string code = R"JTML(
        define x = 0\\
        define y = !x\\
        define cond = (y && 0) || x\\
        show cond\\  
        x = 1\\
        show cond\\  
    )JTML";

    std::string output = runInterpreter(code);

    // Evaluate x=0, y=!0 => 1, cond=(1 && 0)||0 => 0
    EXPECT_NE(output.find("[DEFINE] x = 0"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] y = 1"), std::string::npos);
    EXPECT_NE(output.find("[DEFINE] cond = 0"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 0"), std::string::npos);

    // x=1 => if cond is normal, it stays 0
    // If derived, it might recalc => check logic
    EXPECT_NE(output.find("[ASSIGN] x = 1"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 0"), std::string::npos);
}


TEST(InterpreterTests, FailRedefineDerivedWithDependencies) {
    std::string code = R"JTML(
        define x = 10\\
        derive dependent = x + 5\\
        derive dependent = 50\\ 
    )JTML";
    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DEFINE] x = 10"), std::string::npos);
    EXPECT_NE(output.find("[DERIVE] dependent = 15"), std::string::npos);
    // Attempt to redefine derived with existing dependencies => should fail
    EXPECT_NE(output.find("Cannot redefine derived variable 'dependent' with existing dependencies."), std::string::npos);
}


TEST(InterpreterTests, LargeExpressionsTest) {
    std::string code = R"JTML(
        define x = 1\\
        define y = 2\\
        define z = 3\\
        derive big = x + y*2 - z/3 + (z * y) + (x + y + z)\\
        show big\\
        x = 5\\
        show big\\
    )JTML";
    // Evaluate the complexity if your interpreter supports it

    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[DERIVE] big ="), std::string::npos);
    // Check initial show, then show after x=5
}

// --------------------------------------------------------------------------
// TEST: Try/Except/Then
// --------------------------------------------------------------------------
TEST(InterpreterTests, TryExceptThenTest) {
    // This test checks if the interpreter can handle throw & catch logic
    // along with a 'finally' or 'then' block
    std::string code = R"JTML(
        try\\
            throw "SomethingWentWrong"\\
        \\
        except e\\
            show ("Caught: " + e)\\
        \\
        then\\
            show "Finally block"\\
        \\
    )JTML";

    std::string output = runInterpreter(code);

    // If everything works, you might see lines like:
    // [THROW] SomethingWentWrong
    // [SHOW] Caught: SomethingWentWrong
    // [SHOW] Finally block

    EXPECT_NE(output.find("Caught: SomethingWentWrong"), std::string::npos)
        << "Should have caught the thrown message";
    EXPECT_NE(output.find("[SHOW] Finally block"), std::string::npos)
        << "Should run the finally/then block after the except/catch";
}

// --------------------------------------------------------------------------
// TEST: Try/Except Without Then
// --------------------------------------------------------------------------
TEST(InterpreterTests, TryExceptNoFinallyTest) {
    std::string code = R"JTML(
        try\\
            throw "FatalError"\\
        \\
        except e\\
            show ("Error Caught: " + e)\\
        \\
    )JTML";

    std::string output = runInterpreter(code);

    // We expect something like:
    // [THROW] FatalError
    // [SHOW] Error Caught: FatalError
    // But no "Finally block" since there's no 'then'.

    EXPECT_NE(output.find("[THROW] FatalError"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] Error Caught: FatalError"), std::string::npos);
}

// --------------------------------------------------------------------------
// TEST: Simple If/Else
// --------------------------------------------------------------------------
TEST(InterpreterTests, IfElseTest) {
    // Example: We test that "if" and "else" evaluate properly.
    // If x == 1, it shows "X is one"; otherwise "X is not one".
    std::string code = R"JTML(
        define x = 1\\
        if (x == 1)\\
            show "X is one"\\
        else\\
            show "X is not one"\\
        \\

        if (x != 1)\\
            show "X is not one"\\
        else\\
            show "X is one"\\
        \\
    )JTML";

    std::string output = runInterpreter(code);

    // We expect "X is one" from the first if-branch
    EXPECT_NE(output.find("[SHOW] X is one"), std::string::npos)
        << "Should show 'X is one' when x == 1";

    // We expect "X is one" again from the second if-branch’s else part
    EXPECT_NE(output.find("[SHOW] X is one"), std::string::npos)
        << "Should show 'X is one' again from the else block";
}

// --------------------------------------------------------------------------
// TEST: While Loop
// --------------------------------------------------------------------------
TEST(InterpreterTests, WhileLoopTest) {
    // This test checks while loops, break conditions, etc.
    std::string code = R"JTML(
        define i = 0\\
        while (i < 3)\\
            show i\\
            i = i + 1\\
        \\
    )JTML";

    std::string output = runInterpreter(code);

    // We expect 3 show statements: 0, 1, 2
    EXPECT_NE(output.find("[SHOW] 0"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 1"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] 2"), std::string::npos);
    // Make sure "i = 3" or further is not shown
    EXPECT_EQ(output.find("[SHOW] 3"), std::string::npos);
}

TEST(InterpreterTests, NestedIfElse) {
    std::string code = R"JTML(
        define a = 2\\
        if (a > 0)\\
            if (a < 5)\\
                show "a is between 1 and 4"\\
            \\
            else\\
                show "a is >= 5"\\
            \\
        \\
        else\\
            show "a is zero or negative"\\
        \\
    )JTML";

    std::string output = runInterpreter(code);
    // Expect the nested if branch "a is between 1 and 4"
    EXPECT_NE(output.find("[SHOW] a is between 1 and 4"), std::string::npos);
}

TEST(InterpreterTests, IfNoElse) {
    std::string code = R"JTML(
        define y = 10\\
        if (y == 10)\\
            show "Ten"\\
        \\  
        show "Done"\\
    )JTML";

    std::string output = runInterpreter(code);
    EXPECT_NE(output.find("[SHOW] Ten"), std::string::npos);
    EXPECT_NE(output.find("[SHOW] Done"), std::string::npos);
}

TEST(InterpreterTests, SimpleIfElse) {
    std::string code = R"JTML(
        define x = 3\\
        if (x >= 5)\\
            show "Greater or equal 5"\\
        \\
        else\\
            show "Less than 5"\\
        \\
    )JTML";

    std::string output = runInterpreter(code);

    // We expect x = 3, so the else-branch is executed
    EXPECT_NE(output.find("[SHOW] Less than 5"), std::string::npos);
    EXPECT_EQ(output.find("[SHOW] Greater or equal 5"), std::string::npos);
}
