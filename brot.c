#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* image border defines */
#define X_MIN -2.125
#define X_MAX 0.75
#define Y_MINMAX 1.25

/* typedefs */
typedef unsigned int uint;
typedef unsigned char uchar;
typedef _Bool bool;
typedef union value value_t;
typedef struct rgb24 rgb24_t;
typedef struct etime_thread_args etime_thread_args_t;

/* data structure declarations */
union value {
    uint iter_count;
    float hue;
};

struct rgb24 {
    uchar r;
    uchar g;
    uchar b;
};

struct etime_thread_args {
    value_t* column;
    double x0;
};

/* global variables */
uint scale = 4;
uint max_iter = 1024;
uchar output_mode = 0;
uint palette = 19;

uint size_x;
uint size_y;
uint size_yh;

/* converts float value to color */
static rgb24_t hue_rgb(float hue)
{
    rgb24_t tmp = {
        (uchar) ((hue >= 0.5 ? hue * 2 - 1 : 0) * 255),
        (uchar) (hue * 255),
        (uchar) ((hue <= 0.5 ? hue * 2 : 1) * 255)
    };
    return tmp;
}

/* assigns base color to palette */
static rgb24_t palettize(rgb24_t color, uint p)
{
    rgb24_t tmp = {
        !(p     % 3) ? color.r : p     % 3 == 1 ? color.g : color.b,
        !(p / 3 % 3) ? color.r : p / 3 % 3 == 1 ? color.g : color.b,
        !(p / 9 % 3) ? color.r : p / 9 % 3 == 1 ? color.g : color.b
    };
    return tmp;
}

/* escape time algorithm thread function */
static void* etime_thread(void* argp)
{
    etime_thread_args_t args = *(etime_thread_args_t*)argp;

    for (uint i = 0; i < size_yh; ++i) {
        double y0 = i * ((output_mode ? 0.125 : 0.25) / scale) - Y_MINMAX;
        double x = 0, y = 0, x2 = 0, y2 = 0;

        while (x2 + y2 <= 4 && args.column[i].iter_count < max_iter) {
            y = (x + x) * y + y0;
            x = x2 - y2 + args.x0;
            x2 = x * x;
            y2 = y * y;
            ++args.column[i].iter_count;
        }

        if (++args.column[i].iter_count >= max_iter) args.column[i].iter_count = 0;
        args.column[size_y - 1 - i].iter_count = args.column[i].iter_count; /* symmetry */
    }

    return NULL;
}

/* histogram thread function */
static void* histogram_thread(void* argp)
{
    return NULL;
}

/* main function */
int main(int argc, char** argv)
{
	/*if (argc > 1) scale = atoi(argv[1]);
    if (argc > 2) max_iter = atoi(argv[2]);
    if (argc > 3) palette = atoi(argv[3]);
    if (argc > 4 && argv[4][0] == 'p') output_mode = 1;
    if (argc > 5) return 1;*/

    switch (argc) {
        case 5:
            if (argv[4][0] == 'p') output_mode = 1;
        case 4:
            palette = atoi(argv[3]);
        case 3:
            max_iter = atoi(argv[2]);
        case 2:
            scale = atoi(argv[1]);
        case 1:
            break;
        default:
            return 1;
    }

    size_x = 23 * scale + 1;
    size_y = 10 * (output_mode + 1) * scale + 1;
    size_yh = 5 * (output_mode + 1) * scale + 1;

    /* allocate pixel value array */
    value_t** values = calloc(size_x, sizeof(value_t*));
    if (values == NULL) return 1;

    for (uint i = 0; i < size_x; ++i) {
	    values[i] = calloc(size_y, sizeof(value_t));
        if (values[i] == NULL) return 1;
    }

    fprintf(stderr, "%ux%u, %u iterations\n", size_x, size_y, max_iter);

    /* calculate escape time */
    /*for (uint i = 0; i < size_x; ++i) {
        double x0 = i * (0.125 / scale) + X_MIN;

        for (uint k = 0; k < size_yh; ++k) {
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

    /* calculate escape time */
    {
        pthread_t* threads = malloc(size_x * sizeof(pthread_t));
        etime_thread_args_t* etime_thread_args = malloc(size_x * sizeof(etime_thread_args_t));
        int err;

        for (uint i = 0; i < size_x; ++i) {
            etime_thread_args[i].column = values[i];
            etime_thread_args[i].x0 = i * (0.125 / scale) + X_MIN;

            err = pthread_create(&threads[i], NULL, etime_thread, &etime_thread_args[i]);
            if (err) {
                fprintf(stderr, "error %d at thread %u", err, i);
                return 1;
            }
        }

        for (uint i = 0; i < size_x; ++i) {
            err = pthread_join(threads[i], NULL);
            if (err) {
                fprintf(stderr, "error %d at thread %u", err, i);
                return 1;
            }
        }

        free(etime_thread_args);
        free(threads);
    }

    /* symmetry */
    /*for (uint k = 1; k < size_yh; ++k) {
        for (uint i = 0; i < size_x; ++i) {
            values[i][size_yh - 1 + k].iter_count = values[i][size_yh - 1 - k].iter_count;
        }
    }*/

    /* histogram */
    {
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
    }

    /* output data */
    {
        if (!output_mode) {
            for (uint k = 0; k < size_y; ++k) {
                for (uint i = 0; i < size_x; ++i) {
                    rgb24_t color = palettize(hue_rgb(values[i][k].hue), palette);
                    fprintf(stdout, "\033[48;2;%hhu;%hhu;%hhum ", color.r, color.g, color.b);
                }
                fputs("\033[0m\n", stdout);
            }
        } else if (output_mode) {
            fprintf(stdout, "P6\n%u\n%u\n255\n", size_x, size_y); /* binary ppm format*/

            for (uint k = 0; k < size_y; ++k) {
                for (uint i = 0; i < size_x; ++i) {
                    rgb24_t color = palettize(hue_rgb(values[i][k].hue), palette);
                    putchar(color.r);
                    putchar(color.g);
                    putchar(color.b);
                }
            }
        }
    }

    return 0;
}
