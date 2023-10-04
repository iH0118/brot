# ih0118 brot

escape time / histogram mandelbrot set renderer
##
### usage:
```sh
clang -Wall -Werror -O3 -o brot brot.c

# ascii output
./brot <scale> <iterations>

# png file output
./brot <scale> <iterations> p | pnmtopng > <filename>.png
```
default settings: `brot 4 1024`
##
### other stuff
clang default optimization VS -O3

![](/timecmp.png "Arch Linux x86-64, AMD Ryzen 7 5800X @ 4.200 GHz; The binary compiled with the -O3 flag is about 3 times faster")




