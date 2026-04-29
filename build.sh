#!/bin/bash

# A simple build script so you don't have to type the massive clang++ command every time

echo "Compiling the Ant Universe Engine..."

clang++ -std=c++17 \
    src/engine.cpp \
    src/glad.c \
    src/imgui/*.cpp \
    include/imgui/imgui_impl_glfw.cpp \
    include/imgui/imgui_impl_opengl3.cpp \
    -o engine \
    -Iinclude \
    -Iinclude/imgui \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lglfw \
    -framework OpenGL \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreVideo

if [ $? -eq 0 ]; then
    echo "Build successful! Run with: ./engine"
else
    echo "Build failed."
fi
