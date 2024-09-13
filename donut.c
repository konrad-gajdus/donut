#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define CANVAS_WIDTH 110
#define CANVAS_HEIGHT 50

#define THETA_STEP 0.02
#define PHI_STEP 0.01

const char* COLOR_CODES[] = {
    "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m",
    "\033[91m", "\033[92m", "\033[93m", "\033[94m", "\033[95m", "\033[96m"};

#define COLOR_COUNT (sizeof(COLOR_CODES) / sizeof(COLOR_CODES[0]))
const char* LUMINANCE_CHARS = " .:-=+*#%@";
#define LUMINANCE_LEVELS (sizeof(LUMINANCE_CHARS) - 1)

#define BUFFER_SIZE (CANVAS_HEIGHT * (CANVAS_WIDTH * 20 + 1) + 1)

void clear_screen() {
    printf("\033[2J\033[H");
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void render_donut(double A, double B, char* buffer, double tube_radius, double donut_radius, double viewer_distance) {
    char output[CANVAS_HEIGHT][CANVAS_WIDTH][20];
    double z_buffer[CANVAS_HEIGHT][CANVAS_WIDTH];

    memset(output, 0, sizeof(output));
    for (int y = 0; y < CANVAS_HEIGHT; y++)
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            strcpy(output[y][x], " ");
            z_buffer[y][x] = -1e9;
        }

    double projection_factor = CANVAS_WIDTH * viewer_distance * 3 / (8 * (tube_radius + donut_radius));

    for (double theta = 0; theta < 2 * M_PI; theta += THETA_STEP) {
        double cos_theta = cos(theta), sin_theta = sin(theta);
        for (double phi = 0; phi < 2 * M_PI; phi += PHI_STEP) {
            double cos_phi = cos(phi), sin_phi = sin(phi);

            double circle_x = donut_radius + tube_radius * cos_theta;
            double circle_y = tube_radius * sin_theta;

            double x = circle_x * (cos(B) * cos_phi + sin(A) * sin(B) * sin_phi) - circle_y * cos(A) * sin(B);
            double y = circle_x * (sin(B) * cos_phi - sin(A) * cos(B) * sin_phi) + circle_y * cos(A) * cos(B);
            double z = viewer_distance + cos(A) * circle_x * sin_phi + circle_y * sin(A);
            double inv_z = 1 / z;

            int xp = (int)(CANVAS_WIDTH / 2 + projection_factor * inv_z * x);
            int yp = (int)(CANVAS_HEIGHT / 2 - projection_factor * inv_z * y / 2);

            double normal_x = cos_phi * cos_theta;
            double normal_y = cos_phi * sin_theta;
            double normal_z = sin_phi;

            double light_dir_x = 0.5;
            double light_dir_y = -0.5;
            double light_dir_z = -1;
            double light_intensity = normal_x * light_dir_x + normal_y * light_dir_y + normal_z * light_dir_z;

            double ambient = 0.2;
            light_intensity = fmax(light_intensity, ambient);

            double view_x = -x;
            double view_y = -y;
            double view_z = viewer_distance - z;
            double view_length = sqrt(view_x * view_x + view_y * view_y + view_z * view_z);
            view_x /= view_length;
            view_y /= view_length;
            view_z /= view_length;

            double reflect_x = 2 * light_intensity * normal_x - light_dir_x;
            double reflect_y = 2 * light_intensity * normal_y - light_dir_y;
            double reflect_z = 2 * light_intensity * normal_z - light_dir_z;

            double specular = pow(fmax(0, view_x * reflect_x + view_y * reflect_y + view_z * reflect_z), 20);

            light_intensity = fmin(1, light_intensity + 0.6 * specular);

            if (light_intensity > 0 && xp >= 0 && xp < CANVAS_WIDTH && yp >= 0 && yp < CANVAS_HEIGHT) {
                if (inv_z > z_buffer[yp][xp]) {
                    z_buffer[yp][xp] = inv_z;
                    int luminance_index = (int)(pow(light_intensity, 0.9) * (LUMINANCE_LEVELS - 1));
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

void print_controls() {
    printf("\033[%d;0H", CANVAS_HEIGHT + 2);  // Move cursor below the donut
    printf("Controls:\n");
    printf("W/S: +/- Tube radius | A/D: +/- Donut radius | Q/E: +/- Viewer distance | R: Reset | X: Exit\n");
}

int main() {
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        return 1;
    }

    double A = 0, B = 0;
    double tube_radius = 1;
    double donut_radius = 2;
    double viewer_distance = 5;

    while (1) {
        render_donut(A, B, buffer, tube_radius, donut_radius, viewer_distance);
        clear_screen();
        fwrite(buffer, 1, strlen(buffer), stdout);
        printf("Tube Radius: %.2f, Donut Radius: %.2f, Viewer Distance: %.2f\n", tube_radius, donut_radius, viewer_distance);
        print_controls();
        fflush(stdout);

        if (kbhit()) {
            char c = getchar();
            switch (c) {
                case 'w':
                case 'W': tube_radius += 0.1; break;
                case 's':
                case 'S': tube_radius = fmax(0.1, tube_radius - 0.1); break;
                case 'a':
                case 'A': donut_radius = fmax(tube_radius + 0.1, donut_radius - 0.1); break;
                case 'd':
                case 'D': donut_radius += 0.1; break;
                case 'q':
                case 'Q': viewer_distance = fmax(3, viewer_distance - 0.5); break;
                case 'e':
                case 'E': viewer_distance += 0.5; break;
                case 'r':
                case 'R':
                    tube_radius = 1;
                    donut_radius = 2;
                    viewer_distance = 5;
                    break;
                case 'x':
                case 'X': goto cleanup;
            }
        }

        usleep(30000);
        A += 0.04;
        B += 0.02;
    }

cleanup:
    free(buffer);
    return 0;
}
