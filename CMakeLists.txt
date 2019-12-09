cmake_minimum_required(VERSION 3.15)

project(ACC VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    ${PROJECT_NAME}
    src/token.cpp
    src/Lexer.cpp
    src/node.cpp
    src/Parser.cpp
    src/Visiter.cpp
    src/Compiler.cpp
    src/main.cpp
)
