#!/bin/bash

echo "building build script"
mkdir buildScript/bin
g++ -o buildScript/bin/main -Iproject/src/export -Ijson/export -g buildScript/src/*.cpp project/src/*.cpp
echo "done"
echo "building configure"
mkdir configure/bin
g++ -o configure/bin/main -Iproject/src/export -Ijson/export -g configure/src/*.cpp project/src/*.cpp
echo "done"
echo "building run"
mkdir run/bin
g++ -o run/bin/main -Iproject/src/export -Ijson/export -g run/src/*.cpp project/src/*.cpp
echo "done"
