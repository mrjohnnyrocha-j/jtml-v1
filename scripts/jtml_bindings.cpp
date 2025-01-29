// jtml_bindings.cpp
#include <pybind11/pybind11.h>
#include "include/jtml_parser.h"       
#include "include/jtml_interpreter.h"  
#include "include/transpiler.h" 

namespace py = pybind11;

void interpret_string(const std::string& code) {
    JtmlTranspiler transpiler;  
    Interpreter interp(transpiler);
    interp.interpret(code);
}

PYBIND11_MODULE(jtml_engine, m) {
    m.doc() = "JTML engine Python bindings";
    m.def("interpret_string", &interpret_string, "Interpret a jtml snippet");
}
