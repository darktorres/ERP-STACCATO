#!/bin/bash

shopt -s globstar
shopt -s nullglob

clang-format-15 -i -style=file **/*.c **/*.cpp **/*.h **/*.hpp
