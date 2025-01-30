g++ -std=c++17 -o quick-test test_main.cpp src/jtml_lexer.cpp src/jtml_parser.cpp src/jtml_interpreter.cpp src/jtml_transpiler.cpp src/jtml_ast.cpp
./quick-test

 g++ -g -O0 -o jtml_app quick-test.cpp src/jtml_lexer.cpp src/jtml_parser.cpp src/transpiler.cpp src/jtml_interpreter.cpp src/jtml_ast.cpp include/Environment.cpp include/Array.cpp include/Dict.cpp include/value.cpp