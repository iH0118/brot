# h0118/brot

escape time / histogram mandelbrot set renderer

usage:
```sh
clang -Wall -Werror -O3 -o brot brot.c

# ascii output
./brot <scale> <iterations>

# png file output
./brot <scale> <iterations> p | pnmtopng > <filename>.png
```
default settings: `brot 4 1024`
