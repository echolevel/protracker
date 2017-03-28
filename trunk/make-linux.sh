#!/bin/bash

rm release/protracker &> /dev/null

echo Compiling, please wait...
gcc src/*.c src/gfx/*.c -lSDL2 -lm -ldl -Wall -Wno-unused-result -Wc++-compat -Wshadow -Winit-self -Wextra -Wunused -Wunreachable-code -Wredundant-decls -Wswitch-default -march=native -mtune=native -O3 -o release/protracker

rm src/*.o src/gfx/*.o &> /dev/null

echo Done! The binary \(protracker\) is in the folder named \'release\'.
echo To run it, type ./protracker in the release folder.
