#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

// All your JTML headers
#include "include/jtml_lexer.h"
#include "include/jtml_parser.h"
#include "include/jtml_interpreter.h"
#include "include/transpiler.h"

// For the HTTP server
#include "httplib.h"

static void usage() {
    std::cout << "Usage:\n"
              << "  jtml interpret <input.jtml>\n"
              << "  jtml transpile <input.jtml> -o <output.html>\n"
              << "  jtml serve <input.jtml> [--port <num>]\n";
    std::exit(1);
}

std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage();
    }

    std::string command = argv[1]; // "interpret", "transpile", or "serve"
    std::string inputFile = argv[2];

    std::string outputFile;
    int port = 8080; // default port

    // Parse additional arguments
    for (int i = 3; i < argc; ++i) {
        if ((std::strcmp(argv[i], "-o") == 0 || std::strcmp(argv[i], "--out") == 0) && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else {
            usage(); // Unrecognized argument
        }
    }

    try {
        // Step 1: Read the .jtml file
        std::string inputText = readFile(inputFile);

        // Step 2: Lex + Parse
        Lexer lexer(inputText);
        auto tokens = lexer.tokenize();

        const auto& errors = lexer.getErrors();
        if (!errors.empty()) {
            for (const auto& error : errors) {
                std::cerr << "Lexer Error: " << error << std::endl;
            }
            return 1;
        }

        Parser parser(std::move(tokens));
        auto root = parser.parseJtmlElement();
        auto program = parser.parseProgram(); 

        if (command == "interpret") {
            // Interpret the parsed JTML server-side
            JtmlTranspiler transpiler;

        
            Interpreter interpreter(transpiler);
            interpreter.interpret(*root); // Interpret to populate variables

        } else if (command == "transpile") {
            // Transpile JTML to HTML
            JtmlTranspiler transpiler;

        
            Interpreter interpreter(transpiler);
            interpreter.interpret(*root); // Interpret to populate variables

           
            std::string html = transpiler.transpile(program);

            if (!outputFile.empty()) {
                std::ofstream ofs(outputFile);
                if (!ofs.is_open()) {
                    throw std::runtime_error("Cannot open output file: " + outputFile);
                }
                ofs << html;
                ofs.close();
                std::cout << "Wrote transpiled HTML to " << outputFile << "\n";
            } else {
                std::cout << html << "\n";
            }

        } else if (command == "serve") {
            // Transpile JTML to HTML and serve via HTTP
            JtmlTranspiler transpiler;

        
            Interpreter interpreter(transpiler);
            interpreter.interpret(*root); // Interpret to populate variables
            std::string html = transpiler.transpile(program);

            httplib::Server svr;
            svr.Get("/", [html](const httplib::Request&, httplib::Response& res) {
                res.set_content(html, "text/html");
            });

            std::cout << "Serving JTML on http://localhost:" << port << "\n";
            svr.listen("0.0.0.0", port);

        } else {
            usage();
        }

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
