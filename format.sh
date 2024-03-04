#!/bin/bash

for f in src/*.cpp include/*/*.hpp ; do clang-format -i $f ; done
 