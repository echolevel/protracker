# Protracker 2.3D
A fork of 8bitbubsy's Protracker 2.3d clone for cross-platform SDL

All credit to [http://www.16-bits.org](8bitbubsy). Official version [https://sourceforge.net/projects/protracker/](here).

Released by Olav "8bitbubsy" Sorensen under the 'DO WHAT THE FUCK YOU WANT TO" public license:

```

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
Version 2, December 2004

Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

0. You just DO WHAT THE FUCK YOU WANT TO.

```

## PT.Config-XX files

I was having trouble getting this build of PT to load my Amiga PT.Config files, which it's supposed to, but there was a small
bug that I've fixed here, and passed back to 8bitbubsy so he can update the SVN. Now that they do work, you can follow the latest
'correct' practice by putting your protracker.ini and your PT.Config files (they can be numbered -XX as on the Amiga, or you can miss the hyphen and number off and they'll
still be read) into \~/.protracker/ (which you may have to create). If there's no PT.Config there, PT looks for PT.Config files in the defaultdir
that's set in protracker.ini - bear in mind, though, that this doesn't seem to parse '~/'. Make sure you use the full path. Setting this is a good idea
either way, as it's Disk Op's starting directory on load and saves navigating painstakingly to your samples/modules folder each time.

## Custom graphics

8bitbubsy has written a great little program that converts BMP graphics to a C buffer for inclusion in Protracker. 
I've used it before, when hacking my own graphics into his PT2.3E for 68k Amiga (I don't know what I'm doing in ASM but
I can just about assemble a program from source), and the same is used here.

First, use the bitmaps in src/gfx/bmp as a guide to dimensions and what will/won't mess up other parts of the GUI.
Then you'll need to make sure your custom BMP is restricted to pixels of ONLY these colours: 000000, 555555, 888888 and BBBBBB.
They're placeholders for the palette entries held by PT.Config (or PT's defaults if there's no config file found). 

There's a Windows exe of bmp2pth, though you might need to build your own from bmp2pth.c in src/gfx/bmp. On Mac, 


`
gcc bmp2pth.c -o bmp2pth && chmod a+x bmp2pth
`

then run it from the commandline with your new BMP as an argument:


`
bmp2pth myLegitFireGrafx.bmp
`

You'll end up with myLegitFireGrafx.bmp.c, which you should move to the src/gfx directory. Next,

* rename the bmp.c you're replacing to something else
* open the OLD one and copy the const name (e.g. _aboutScreenPackedBMP_) 
* paste that into the NEW bmp.c
* copy the buffer array size from the NEW bmp.c (e.g. _1872_)
* open src/pt\_tables.h and find the relevant extern line (e.g. _extern const uint8\_t aboutScreenPackedBMP[2003];_)
* update the array length there

Now you should be able to build Protracker as per 8bitbubsy's detailed instructions in _compiling.txt_ and see your fancy new graphics!

Caveat: greating the BMP in the first place can be a big hassle, and bmp2pth will tell you when you've got it wrong. If you're
using Photoshop, it needs to be a Truecolour (ie 16, 24 or 32 bit) bitmap, but you should be working in RGB mode and 8bits/channel. Make sure 
there are no rogue pixels with colours that aren't those listed above. When saving, go for 'Windows' and '32 bit' in the export options. Then
hopefully it'll work... If it sort of works but looks messed up when you run PT, you've probably forgotten to update the buffer array size
in pt\_tables.h. If you forget something else, like the const name, PT won't build anyway (and should throw an error that gives you a hint).

Good luck!