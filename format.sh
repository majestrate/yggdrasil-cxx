#!/bin/bash

for f in test/*.cpp src/*.cpp include/*/*.hpp ; do clang-format -i $f ; done
 
