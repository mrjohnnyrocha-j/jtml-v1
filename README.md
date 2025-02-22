# JTML

JTML is a reactive templating language that compiles to HTML+JavaScript with real-time DOM synchronization. It combines server-side logic with client-side reactivity through WebSocket communication.

## Key Features
Reactive DOM: Automatic UI updates on variable changes, 
Control Structures: Native support for loops, conditionals, and events

Component Architecture: Reusable elements with encapsulated logic

Bidirectional Binding: WebSocket-based sync between server and client

Reactive Collections: Auto-updating arrays and dictionaries

Project Structure
Core Components
Transpiler (transpiler.h)

Converts JTML AST to HTML with reactivity markers

Generates unique DOM IDs and data attributes

Outputs WebSocket client integration

Interpreter (jtml_interpreter.cpp)

Manages reactive variable graph

Executes server-side logic

WebSocket server implementation

Event propagation system

Lexer (jtml_lexer.cpp)

Tokenizes JTML syntax

Handles string interpolation

Error recovery and position tracking

Parser (jtml_parser.cpp)

Constructs AST from token stream

Supports complex expressions and nested structures

Implements grammar rules and validation

Environment System (Environment.h)

Manages variable scoping

Dependency tracking

Change propagation

Reactive collections (Array.h, Dict.h)

Architecture Overview
Compilation Pipeline
mermaid

graph LR
    JTML --> Lexer --> Tokens --> Parser --> AST --> Transpiler --> HTML/JS
Reactivity System
Dependency Graph

Track variable relationships

Automatic dirty marking

Topological update ordering

Reactive Types

Derived variables

Auto-updating arrays

Observable dictionaries

Change Propagation

WebSocket message queue

DOM diffing algorithm

Batched updates

Getting Started
Prerequisites
C++17 compiler

CMake 3.12+

websocketpp

nlohmann/json

Installation

git clone https://github.com/yourusername/jtml.git
cd jtml
mkdir build && cd build
cmake -DWEBSOCKETPP_DIR=/path/to/websocketpp ..
make
Example Component
counter.jtml

element Counter(initial=0) {
  button onClick="increment()" {
    "Clicks: " + count
  }
  
  derive count = initial
  
  function increment() {
    count += 1
  }
}
Transpiled Output


<div id="counter_1" data-jtml-component="Counter">
  <button onclick="sendEvent('counter_1', 'increment', [])">
    {{count}}
  </button>
</div>
Run HTML
Development Guide
Key Concepts
Reactive Variables


derive fullName = firstName + " " + lastName
Collections


define users = ["Alice", "Bob"]
users.push("Charlie")
Component Lifecycle

Element parsing

Environment creation

Attribute binding

Subscription management

Dependency Management
The Environment system tracks relationships using a directed graph:


// Add dependency between variables
env->addDependency(depKey, dependentKey);

// Trigger update cascade
env->markDirty(varKey);
Event System
mermaid
Copy
sequenceDiagram
    Client->>Server: WS Message {type: "event", element: "btn1"}
    Server->>Interpreter: Execute bound handler
    Interpreter->>Environment: Update variables
    Environment->>Client: WS Updates
    Client->>DOM: Apply changes
API Reference
Core Classes
Class	Responsibility
JtmlTranspiler	AST to HTML conversion
Interpreter	Runtime execution & WS management
Environment	Variable scoping & reactivity
ReactiveArray	Observable array implementation
ReactiveDict	Observable dictionary implementation
Contributing
Fork the repository

Create feature branch (git checkout -b feature/improvement)

Add tests for new functionality

Submit Pull Request with documentation updates

License
MIT License - See LICENSE for details
