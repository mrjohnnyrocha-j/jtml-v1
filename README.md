
# JTML

JTML is a reactive templating language that compiles to HTML+JavaScript with real-time DOM synchronization. It combines server-side logic with client-side reactivity through WebSocket communication.

## Key Features

-   **Reactive DOM**: Automatic UI updates on variable changes
    
-   **Control Structures**: Native support for loops, conditionals, and events
    
-   **Component Architecture**: Reusable elements with encapsulated logic
    
-   **Bidirectional Binding**: WebSocket-based sync between server and client
    
-   **Reactive Collections**: Auto-updating arrays and dictionaries
    

## Project Structure

### Core Components

1.  **Transpiler (`transpiler.h`)**
    
    -   Converts JTML AST to HTML with reactivity markers
        
    -   Generates unique DOM IDs and data attributes
        
    -   Outputs WebSocket client integration
        
2.  **Interpreter (`jtml_interpreter.cpp`)**
    
    -   Manages reactive variable graph
        
    -   Executes server-side logic
        
    -   WebSocket server implementation
        
    -   Event propagation system
        
3.  **Lexer (`jtml_lexer.cpp`)**
    
    -   Tokenizes JTML syntax
        
    -   Handles string interpolation
        
    -   Error recovery and position tracking
        
4.  **Parser (`jtml_parser.cpp`)**
    
    -   Constructs AST from token stream
        
    -   Supports complex expressions and nested structures
        
    -   Implements grammar rules and validation
        
5.  **Environment System (`Environment.h`)**
    
    -   Manages variable scoping
        
    -   Dependency tracking
        
    -   Change propagation
        
    -   Reactive collections (`Array.h`,  `Dict.h`)
        

## Architecture Overview

### Compilation Pipeline

mermaid

Copy

graph LR
    JTML --> Lexer --> Tokens --> Parser --> AST --> Transpiler --> HTML/JS

### Reactivity System

1.  **Dependency Graph**
    
    -   Track variable relationships
        
    -   Automatic dirty marking
        
    -   Topological update ordering
        
2.  **Reactive Types**
    
    -   Derived variables
        
    -   Auto-updating arrays
        
    -   Observable dictionaries
        
3.  **Change Propagation**
    
    -   WebSocket message queue
        
    -   DOM diffing algorithm
        
    -   Batched updates
        

## Getting Started

### Prerequisites

-   C++17 compiler
    
-   CMake 3.12+
    
-   websocketpp
    
-   nlohmann/json
    

### Installation



    git clone https://github.com/yourusername/jtml.git
    cd jtml
    mkdir build && cd build
    cmake -DWEBSOCKETPP_DIR=/path/to/websocketpp ..
    make

### Example Component

**counter.jtml**


    element Counter(initial=0) {
      button onClick="increment()" {
        "Clicks: " + count
      }
      
      derive count = initial
      
      function increment() {
        count += 1
      }
    }

**Transpiled Output**



    <div id="counter_1" data-jtml-component="Counter">
      <button onclick="sendEvent('counter_1', 'increment', [])">
        {{count}}
      </button>
    </div>



## Development Guide

### Key Concepts

-   **Reactive Variables**

        
       
    derive fullName = firstName + " " + lastName
    
-   **Collections**
    
   
define users = ["Alice", "Bob"]
users.push("Charlie")
    
-   **Component Lifecycle**
    
    1.  Element parsing
        
    2.  Environment creation
        
    3.  Attribute binding
        
    4.  Subscription management
        

### Dependency Management

The Environment system tracks relationships using a directed graph:



    // Add dependency between variables
    env->addDependency(depKey, dependentKey);
    
    // Trigger update cascade
    env->markDirty(varKey);



## API Reference

### Core Classes

Class

Responsibility

`JtmlTranspiler`

AST to HTML conversion

`Interpreter`

Runtime execution & WS management

`Environment`

Variable scoping & reactivity

`ReactiveArray`

Observable array implementation

`ReactiveDict`

Observable dictionary implementation

## Contributing

1.  Fork the repository
    
2.  Create feature branch (`git checkout -b feature/improvement`)
    
3.  Add tests for new functionality
    
4.  Submit Pull Request with documentation updates
    

## License

MIT License - See  [LICENSE](https://chat.deepseek.com/a/chat/s/LICENSE)  for details
