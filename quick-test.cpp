#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#include "include/jtml_lexer.h"
#include "include/jtml_parser.h"
#include "include/jtml_ast.h"
#include "include/jtml_interpreter.h"
#include "include/transpiler.h"

// Simple helper to load a file into a string
std::string loadFile(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }
    std::string content((std::istreambuf_iterator<char>(in)),
                        (std::istreambuf_iterator<char>()));
    return content;
}

int main() {
    try {
        // 1) Load the .jtml code
        std::string code = loadFile("examples/dicts.jtml");

        // 2) Lex & Parse
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto program = parser.parseProgram(); 
        // program is std::vector<std::unique_ptr<ASTNode>>

        JtmlTranspiler transpiler;

        // 3) Create an Interpreter
        Interpreter interpreter(transpiler);

        // 5) Transpile to HTML
        std::string outputHTML = transpiler.transpile(program);

        // 6) Write to example.html
        {
            std::ofstream outFile("example.html");
            if (!outFile.is_open()) {
                throw std::runtime_error("Cannot create example.html");
            }
            outFile << outputHTML;
        }
        std::cout << "[INFO] Transpiled HTML written to example.html\n";

        // 7) Interpret the AST => set up environment, wsServer, etc.
        interpreter.interpret(program);
        std::cout << "[INFO] Interpreter finished. WebSocket server is running.\n";

        // 8) Wait so user can connect from browser
        std::cout << "[INFO] Open 'example.html' in your browser. Press Enter to exit.\n";
        std::cin.get();

        return 0;
    }
    catch(const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
