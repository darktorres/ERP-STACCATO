#!/bin/bash

shopt -s globstar
shopt -s nullglob

clang-format-13 -i -style=file **/*.c **/*.cpp **/*.h **/*.hpp
