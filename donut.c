#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define CANVAS_WIDTH 110
#define CANVAS_HEIGHT 50

const char* COLOR_CODES[] = {
    "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m", "\033[91m", "\033[92m", "\033[93m", "\033[94m", "\033[95m", "\033[96m"};

#define COLOR_COUNT (sizeof(COLOR_CODES) / sizeof(COLOR_CODES[0]))
const char* LUMINANCE_CHARS = ".:-=+*#%@";
#define LUMINANCE_LEVELS (sizeof(LUMINANCE_CHARS) - 1)

#define BUFFER_SIZE (CANVAS_HEIGHT * (CANVAS_WIDTH * 20 + 1) + 1)

void clear_screen() {
    printf("\033[2J\033[H");
}

void render_donut(double A, double B, char* buffer) {
    char output[CANVAS_HEIGHT][CANVAS_WIDTH][20];
    double z_buffer[CANVAS_HEIGHT][CANVAS_WIDTH];

    memset(output, 0, sizeof(output));
    for (int y = 0; y < CANVAS_HEIGHT; y++)
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            strcpy(output[y][x], " ");
            z_buffer[y][x] = -1e9;
        }

    double tube_radius = 1;
    double donut_radius = 2;
    double viewer_distance = 5;
    double projection_factor = CANVAS_WIDTH * viewer_distance * 3 / (8 * (tube_radius + donut_radius));

    for (double theta = 0; theta < 2 * M_PI; theta += 0.07) {
        double cos_theta = cos(theta), sin_theta = sin(theta);
        for (double phi = 0; phi < 2 * M_PI; phi += 0.02) {
            double cos_phi = cos(phi), sin_phi = sin(phi);
            
            double circle_x = donut_radius + tube_radius * cos_theta;
            double circle_y = tube_radius * sin_theta;

            double x = circle_x * (cos(B) * cos_phi + sin(A) * sin(B) * sin_phi) - circle_y * cos(A) * sin(B);
            double y = circle_x * (sin(B) * cos_phi - sin(A) * cos(B) * sin_phi) + circle_y * cos(A) * cos(B);
            double z = viewer_distance + cos(A) * circle_x * sin_phi + circle_y * sin(A);
            double inv_z = 1 / z;

            int xp = (int)(CANVAS_WIDTH / 2 + projection_factor * inv_z * x);
            int yp = (int)(CANVAS_HEIGHT / 2 - projection_factor * inv_z * y / 2);

            double luminance = cos_phi * cos_theta * sin(B) - cos(A) * cos_theta * sin_phi - sin(A) * sin_theta + 0.5;
            if (luminance > 0 && xp >= 0 && xp < CANVAS_WIDTH && yp >= 0 && yp < CANVAS_HEIGHT) {
                if (inv_z > z_buffer[yp][xp]) {
                    z_buffer[yp][xp] = inv_z;
                    int luminance_index = (int)(luminance * (LUMINANCE_LEVELS - 1));
                    luminance_index = (luminance_index < 0) ? 0 : (luminance_index >= LUMINANCE_LEVELS ? LUMINANCE_LEVELS - 1 : luminance_index);
                    int color_index = (int)(fmod(theta + phi, 2 * M_PI) / (2 * M_PI) * COLOR_COUNT);
                    sprintf(output[yp][xp], "%s%c\033[0m", COLOR_CODES[color_index], LUMINANCE_CHARS[luminance_index]);
                }
            }
        }
    }

    char* ptr = buffer;
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int written = sprintf(ptr, "%s", output[y][x]);
            ptr += written;
        }
        *ptr++ = '\n';
    }
    *ptr = '\0';
}

int main() {
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        return 1;
    }

    double A = 0, B = 0;
    while (1) {
        render_donut(A, B, buffer);
        clear_screen();
        fwrite(buffer, 1, strlen(buffer), stdout);
        fflush(stdout);
        usleep(50000);
        A += 0.04;
        B += 0.02;
    }

    free(buffer);
    return 0;
}
