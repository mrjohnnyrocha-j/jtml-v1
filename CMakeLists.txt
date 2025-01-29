cmake_minimum_required(VERSION 3.14)
project(jtml_elements LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module
include(FetchContent)

# Fetch pybind11
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6  # Choose a suitable version
)
FetchContent_MakeAvailable(pybind11)

# ========== Python module named "jtml_engine" ==========
add_library(jtml_engine MODULE
    jtml_bindings.cpp   # The Pybind11 file
    # add additional .cpp if necessary
)

target_link_libraries(jtml_engine PRIVATE pybind11::module)

# For httplib or other external includes
include_directories(/usr/local/include)

# Adjust output name based on platform
set_target_properties(jtml_engine PROPERTIES PREFIX "")
if(WIN32)
    set_target_properties(jtml_engine PROPERTIES SUFFIX ".pyd")
elseif(APPLE)
    set_target_properties(jtml_engine PROPERTIES SUFFIX ".so")
else()
    set_target_properties(jtml_engine PROPERTIES SUFFIX ".so")
endif()

# ========== CLI-oriented main ==========
add_executable(jtml
    main.cpp
    src/jtml_ast.cpp
    src/jtml_lexer.cpp
    src/jtml_parser.cpp
    src/jtml_interpreter.cpp
    src/transpiler.cpp
    # add any other .cpp needed
) 

# ========== Test-oriented main ==========
add_executable(jtml_tests
    main_tests.cpp
    src/jtml_ast.cpp
    src/jtml_lexer.cpp
    src/jtml_parser.cpp
    src/jtml_interpreter.cpp
    src/transpiler.cpp
    # add test or mock code
)


add_executable(quick-test
    quick-test.cpp
    src/jtml_ast.cpp
    src/jtml_lexer.cpp
    src/jtml_parser.cpp
    src/jtml_interpreter.cpp
    src/transpiler.cpp
    # add test or mock code
)