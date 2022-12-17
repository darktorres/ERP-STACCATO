#!/bin/bash

shopt -s globstar
shopt -s nullglob

clang-format-14 -i -style=file **/*.c **/*.cpp **/*.h **/*.hpp
