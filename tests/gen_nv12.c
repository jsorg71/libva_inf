
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 640
#define HEIGHT 480
#define NUM_FRAMES 5

/* each digit is 5 wide x 7 tall in a grid */
static const char font_data[5][7][5] =
{
    /* 1 */
    {
        {0, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 1, 1, 1, 1}
    },
    /* 2 */
    {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 1},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {1, 1, 1, 1, 1}
    },
    /* 3 */
    {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 1},
        {0, 0, 0, 0, 1},
        {0, 0, 1, 1, 0},
        {0, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0}
    },
    /* 4 */
    {
        {0, 0, 0, 1, 0},
        {0, 0, 1, 1, 0},
        {0, 1, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 1, 0}
    },
    /* 5 */
    {
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0}
    }
};

int
main(int argc, char **argv)
{
    FILE *fp;
    unsigned char *y_plane;
    unsigned char *uv_plane;
    int frame;
    int y_size;
    int uv_size;
    int scale;
    int digit_w;
    int digit_h;
    int start_x;
    int start_y;
    int row;
    int col;
    int py;
    int px;

    y_size = WIDTH * HEIGHT;
    uv_size = WIDTH * (HEIGHT / 2);
    y_plane = (unsigned char *)malloc(y_size);
    uv_plane = (unsigned char *)malloc(uv_size);

    fp = fopen("test_nv12_640x480.yuv", "wb");
    if (fp == NULL)
    {
        printf("error opening output file\n");
        free(y_plane);
        free(uv_plane);
        return 1;
    }

    /* scale factor: each font pixel becomes scale x scale screen pixels */
    scale = 50;
    digit_w = 5 * scale;  /* 250 */
    digit_h = 7 * scale;  /* 350 */
    start_x = (WIDTH - digit_w) / 2;
    start_y = (HEIGHT - digit_h) / 2;

    for (frame = 0; frame < NUM_FRAMES; frame++)
    {
        /* dark gray background for Y */
        memset(y_plane, 0x10, y_size);
        /* neutral UV (128 = no color) */
        memset(uv_plane, 0x80, uv_size);

        /* draw the digit (1-5) in white on Y plane */
        for (row = 0; row < 7; row++)
        {
            for (col = 0; col < 5; col++)
            {
                if (font_data[frame][row][col])
                {
                    for (py = 0; py < scale; py++)
                    {
                        for (px = 0; px < scale; px++)
                        {
                            int x = start_x + col * scale + px;
                            int y = start_y + row * scale + py;
                            y_plane[y * WIDTH + x] = 0xEB;
                        }
                    }
                }
            }
        }

        fwrite(y_plane, 1, y_size, fp);
        fwrite(uv_plane, 1, uv_size, fp);
    }

    fclose(fp);
    free(y_plane);
    free(uv_plane);
    printf("wrote 5 NV12 frames (640x480) to test_nv12_640x480.yuv\n");
    return 0;
}
