#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*image borders*/
#define X_MIN -2.125
#define X_MAX 0.75
#define Y_MINMAX 1.25

typedef unsigned int uint;
typedef unsigned char uchar;
typedef _Bool bool;
typedef union val value_t;
typedef struct rgb24 rgb24_t;
typedef struct thread_args thread_args_t;

union val {
    uint iter_count;
    float hue;
};

struct rgb24 {
    uchar r;
    uchar g;
    uchar b;
};

struct thread_args {
    value_t** values;
    uint k;
    uint scale;
    uint size_x;
    uint output_mode;
    uint max_iter;
};

static rgb24_t hue_rgb(float hue) {
    rgb24_t tmp = {
        (uchar) ((hue >= 0.5 ? hue * 2 - 1 : 0) * 255),
        (uchar) (hue * 255),
        (uchar) ((hue <= 0.5 ? hue * 2 : 1) * 255)
    };
    return tmp;
}

static rgb24_t palettize(rgb24_t color, uint p) {
    rgb24_t tmp = {
        !(p     % 3) ? color.r : p     % 3 == 1 ? color.g : color.b,
        !(p / 3 % 3) ? color.r : p / 3 % 3 == 1 ? color.g : color.b,
        !(p / 9 % 3) ? color.r : p / 9 % 3 == 1 ? color.g : color.b
    };
    return tmp;
}

static void* etime_thread(void* argp) {
    thread_args_t args = *(thread_args_t*)argp;
    for (uint i = 0; i < args.size_x; ++i) {
            double x0 = i * (0.125 / args.scale) + X_MIN;
            double y0 = args.k * ((args.output_mode ? 0.125 : 0.25) / args.scale) - Y_MINMAX;
            double x = 0, y = 0, x2 = 0, y2 = 0;

            while (x2 + y2 <= 4 && args.values[i][args.k].iter_count < args.max_iter) {
                y = (x + x) * y + y0;
                x = x2 - y2 + x0;
                x2 = x * x;
                y2 = y * y;
                ++args.values[i][args.k].iter_count;
            }

            if (++args.values[i][args.k].iter_count >= args.max_iter) args.values[i][args.k].iter_count = 0;
        }
    return NULL;
}

int main(int argc, char** argv) {
	uint scale = 4;
	uint max_iter = 1024;
    uchar output_mode = 0;
    uint palette = 19;

	if (argc > 1) scale = atoi(argv[1]);
    if (argc > 2) max_iter = atoi(argv[2]);
    if (argc > 3) palette = atoi(argv[3]);
    if (argc > 4 && argv[4][0] == 'p') output_mode = 1;
    if (argc > 5) return 1;

    uint size_x = 23 * scale + 1;
    uint size_y = 10 * (output_mode + 1) * scale + 1;
    uint size_yh = 5 * (output_mode + 1) * scale + 1;

    /*allocate iteration count / hue union array*/
    value_t** values = calloc(size_x, sizeof(value_t*));
    if (values == NULL) return 1;

    for (uint i = 0; i < size_x; ++i) {
	    values[i] = calloc(size_y, sizeof(value_t));
        if (values[i] == NULL) return 1;
    }

    fprintf(stderr, "%ix%i, %i iterations\n", size_x, size_y, max_iter);

    /*calculate escape time*/
    /*for (uint k = 0; k < size_yh; ++k) {
        for (uint i = 0; i < size_x; ++i) {
            double x0 = i * (0.125 / scale) + X_MIN;
            double y0 = k * ((output_mode ? 0.125 : 0.25) / scale) - Y_MINMAX;
            double x = 0, y = 0, x2 = 0, y2 = 0;

            while (x2 + y2 <= 4 && values[i][k].iter_count < max_iter) {
                y = (x + x) * y + y0;
                x = x2 - y2 + x0;
                x2 = x * x;
                y2 = y * y;
                ++values[i][k].iter_count;
            }

            if (++values[i][k].iter_count >= max_iter) values[i][k].iter_count = 0;
        }
    }*/

    /*symmetry*/
    for (uint k = 1; k < size_yh; ++k) {
        for (uint i = 0; i < size_x; ++i) {
            values[i][size_yh - 1 + k].iter_count = values[i][size_yh - 1 - k].iter_count;
        }
    }

    /*histogram*/
    int* px_per_iter = calloc(max_iter, sizeof(int));
    if (px_per_iter == NULL) return 1;

    for (uint k = 0; k < size_y; ++k) {
        for (uint i = 0; i < size_x; ++i) {
            ++px_per_iter[values[i][k].iter_count];
        }
    }

    long long total = 0;
    for (uint i = 0; i < max_iter; ++i) total += px_per_iter[i];

    for (uint k = 0; k < size_y; ++k) {
        for (uint i = 0; i < size_x; ++i) {
            uint ct = values[i][k].iter_count;
            values[i][k].hue = 0;
            for (uint l = 0; l < ct; ++l) {
                values[i][k].hue += (float)px_per_iter[l] / total;
            }
        }
    }

    /*output data*/
    if (!output_mode) {
        for (uint k = 0; k < size_y; ++k) {
            for (uint i = 0; i < size_x; ++i) {
                rgb24_t color = palettize(hue_rgb(values[i][k].hue), palette);
                printf("\033[48;2;%i;%i;%im ", color.r, color.g, color.b);
            }
            printf("\033[0m\n");
        }
    } else if (output_mode) {
        printf("P6\n%i\n%i\n255\n", size_x, size_y); /* binary ppm format*/

        for (uint k = 0; k < size_y; ++k) {
            for (uint i = 0; i < size_x; ++i) {
                rgb24_t color = palettize(hue_rgb(values[i][k].hue), palette);
                putchar(color.r);
                putchar(color.g);
                putchar(color.b);
            }
        }
    }

    return 0;
}
