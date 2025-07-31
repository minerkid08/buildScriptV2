#!/bin/bash

echo "building build script"

g++ -o buildScript/bin/main -Iproject/src/export -Ijson/export -g buildScript/src/*.cpp project/src/*.cpp
echo "done"
echo "building configure"
g++ -o configure/bin/main -Iproject/src/export -Ijson/export -g configure/src/*.cpp project/src/*.cpp
echo "done"
