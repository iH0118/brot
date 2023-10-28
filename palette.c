#include <stdio.h>
#include <stdlib.h>

#define STEP_STDERR (double) 1 / 64
#define HEIGHT_STDERR 1
//#define STEP (double) 1 / 256
//#define HEIGHT 32
//#define BASE_WIDTH 16
//#define BASE_OFFSET 8

typedef unsigned int uint;
typedef unsigned char uchar;
typedef struct rgb24 rgb24;

struct rgb24 {
    uchar r;
    uchar g;
    uchar b;
};

static rgb24 hue_rgb(double hue) {
    rgb24 tmp = {
        (uchar) ((hue >= 0.5 ? hue * 2 - 1 : 0) * 255),
        (uchar) (hue * 255),
        (uchar) ((hue <= 0.5 ? hue * 2 : 1) * 255)
    };
    return tmp;
}

static rgb24 palettize(rgb24 color, uint p) {
    rgb24 tmp = {
        !(p     % 3) ? color.r : p     % 3 == 1 ? color.g : color.b,
        !(p / 3 % 3) ? color.r : p / 3 % 3 == 1 ? color.g : color.b,
        !(p / 9 % 3) ? color.r : p / 9 % 3 == 1 ? color.g : color.b
    };
    return tmp;
}

int main(void) {
    rgb24 base = hue_rgb(0.5);

    for (uint i = 0; i < 27; ++i) {
        for (uint k = 0; k < HEIGHT_STDERR; ++k){
            rgb24 col = palettize(base, i);

            fprintf(stderr, "%2d: \033[48;2;%i;%i;%im  \033[0m ", i, col.r, col.g, col.b);

            for (double f = 0; f <= 1; f += STEP_STDERR) {
                col = palettize(hue_rgb(f), i);

                fprintf(stderr, "\033[48;2;%i;%i;%im ", col.r, col.g, col.b);
            }

            fputs("\033[0m\n", stderr);
        }
    }
    
    return 0;
}
