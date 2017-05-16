#!/bin/bash

arch=$(arch)
if [ $arch == "ppc" ]; then
    echo Sorry, PowerPC is not supported by SDL2...
else
    echo Compiling, please wait...
    
    rm release/protracker-osx.app/Contents/MacOS/protracker &> /dev/null
    
    gcc -mmacosx-version-min=10.6 -m32 -mfpmath=sse -msse2 -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks src/*.c src/gfx/*.c -O3 -lm -Wall -Wshadow -Winit-self -Wextra -Wunused -Wredundant-decls -Wswitch-default -framework SDL2 -framework Cocoa -lm -o release/protracker-osx.app/Contents/MacOS/protracker
    install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2 release/protracker-osx.app/Contents/MacOS/protracker
    
    rm src/*.o src/gfx/*.o &> /dev/null
    echo Done! The binary \(protracker-osx.app\) is in the folder named \'release\'.
fi


