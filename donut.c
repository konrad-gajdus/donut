#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#define WIDTH 110
#define HEIGHT 50

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define RESET "\033[0m"

const char* colors[] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE};
const int num_colors = sizeof(colors) / sizeof(colors[0]);

const char shades[] = " .:!/r(l1Z4H9W8$@";
const int num_shades = sizeof(shades) - 1;

void clear_screen() {
    printf("\033[2J\033[H");
}

void draw_torus(double A, double B) {
    char screen[HEIGHT][WIDTH][20];
    double z_buffer[HEIGHT][WIDTH];

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            strcpy(screen[y][x], " ");
            z_buffer[y][x] = -1e10;
        }
    }

    double R1 = 1;
    double R2 = 2;
    double K2 = 5;
    double K1 = WIDTH * K2 * 3.0 / (8.0 * (R1 + R2));

    for (double theta = 0; theta < 2 * M_PI; theta += 0.07) {
        for (double phi = 0; phi < 2 * M_PI; phi += 0.02) {
            double cosP = cos(phi), sinP = sin(phi);
            double cosT = cos(theta), sinT = sin(theta);
            double circleX = R2 + R1 * cosT;
            double circleY = R1 * sinT;

            double x = circleX * (cos(B) * cosP + sin(A) * sin(B) * sinP) - circleY * cos(A) * sin(B);
            double y = circleX * (sin(B) * cosP - sin(A) * cos(B) * sinP) + circleY * cos(A) * cos(B);
            double z = K2 + cos(A) * circleX * sinP + circleY * sin(A);
            double ooz = 1 / z;  // "one over z"

            int xp = (int)(WIDTH / 2 + K1 * ooz * x);
            int yp = (int)(HEIGHT / 2 - K1 * ooz * y / 2);

            // Lighting and depth calculation
            double L = cosP * cosT * sin(B) - cos(A) * cosT * sinP - sin(A) * sinT + 0.5;
            double depth = (z - K2 + R1 + R2) / (2 * (R1 + R2));  // Normalize depth

            if (L > 0 && xp >= 0 && xp < WIDTH && yp >= 0 && yp < HEIGHT) {
                if (ooz > z_buffer[yp][xp]) {
                    z_buffer[yp][xp] = ooz;
                    
                    // Combine lighting and depth for shading
                    int shade_index = (int)((L * 0.7 + depth * 0.3) * (num_shades - 1));
                    shade_index = (shade_index < 0) ? 0 : (shade_index >= num_shades ? num_shades - 1 : shade_index);

                    // Use depth to determine color
                    int color_index = (int)(depth * (num_colors - 1));
                    color_index = (color_index < 0) ? 0 : (color_index >= num_colors ? num_colors - 1 : color_index);

                    sprintf(screen[yp][xp], "%s%c" RESET, colors[color_index], shades[shade_index]);
                }
            }
        }
    }

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%s", screen[y][x]);
        }
        printf("\n");
    }
}

int main() {
    double A = 0, B = 0;
    while (1) {
        clear_screen();
        draw_torus(A, B);
        usleep(50000);
        A += 0.04;
        B += 0.02;
    }
    return 0;
}
