// TODO: Optimize drawing so larger canvases don't lag. Mostly including drawing in the functions
// that manipulate the state, so it doesn't need to be iterated over twice.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#include "raylib.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TILE_SIZE 5
#define ROWS WINDOW_HEIGHT / TILE_SIZE
#define COLS WINDOW_WIDTH / TILE_SIZE
#define CLOCK_STEP PI / 30  // 6 degrees in radians

typedef enum Screen {
    MENU = 0,
    LINES,
    CLOCK,
    DVD,
    CUBE,
} Screen;

typedef struct {
    int rows;
    int cols;
    int spacing;
    int titleBarHeight;
    Vector2 selectedTile;
} MenuState;

typedef struct {
    Vector2 p1;
    Vector2 p2;
} LinesState;

typedef struct {
    int radius;
    Vector2 handOrigin;
    Vector2 handDest;
} ClockState;

typedef struct {
    int maskWidth;
    int maskHeight;
    bool *mask;
    Vector2 direction;
    Vector2 origin;
} DvdState;

typedef struct {
    Vector3 points[8];
    Vector2 projectedPoints[8];
    double rollAngle;
    double pitchAngle;
    double yawAngle;
    double rollStep;
    double pitchStep;
    double yawStep;
} CubeState;

int getSign(int n) {
    if (n > 0)
        return 1;
    else if (n < 0)
        return -1;
    else
        return 0;
}

int euclideanModulo(int a, int b) {
    return (a % b + b) % b;
}

Vector3 rotate(Vector3 p, Vector3 rotMat[3]) {
    Vector3 result = {
        .x = rotMat[0].x * p.x + rotMat[0].y * p.y + rotMat[0].z * p.z,
        .y = rotMat[1].x * p.x + rotMat[1].y * p.y + rotMat[1].z * p.z,
        .z = rotMat[2].x * p.x + rotMat[2].y * p.y + rotMat[2].z * p.z,
    };
    return result;
}

Vector2 project(Vector3 p, double rollAngle, double pitchAngle, double yawAngle) {
    Vector3 rotMatYaw[3] = {
        {cos(yawAngle), -sin(yawAngle), 0},
        {sin(yawAngle), cos(yawAngle), 0},
        {0, 0, 1}
    };

    Vector3 rotMatPitch[3] = {
        {cos(pitchAngle), 0, sin(pitchAngle)},
        {0, 1, 0},
        {-sin(pitchAngle), 0, cos(pitchAngle)}
    };

    Vector3 rotMatRoll[3] = {
        {1, 0, 0},
        {0, cos(rollAngle), -sin(rollAngle)},
        {0, sin(rollAngle), cos(rollAngle)}
    };

    Vector3 rotated = rotate(p, rotMatYaw);
    rotated = rotate(rotated, rotMatPitch);
    rotated = rotate(rotated, rotMatRoll);

    Vector2 projected = {
        rotated.x,
        rotated.y
    };

    return projected;
}

void initGrid(bool grid[ROWS][COLS]) {
    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            grid[y][x] = GetRandomValue(0, 1);
        }
    }
}

void drawGrid(bool grid[ROWS][COLS]) {
    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            if (grid[y][x])
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, BLACK);
        }
    }
}

const char *tileNames[] = {"lines", "clock", "dvd", "cube", "placeholder", "placeholder"};
const Screen screens[] = {LINES, CLOCK, DVD, CUBE, MENU, MENU};
void drawMenuTiles(MenuState menuState) {
    float outlineWidth = (WINDOW_WIDTH - (menuState.cols + 1) * menuState.spacing) / menuState.cols;
    float outlineHeight = (WINDOW_HEIGHT - menuState.titleBarHeight - (menuState.rows + 1) * menuState.spacing) / menuState.rows;
    for (int i = 0; i < menuState.rows; i++) {
        for (int j = 0; j < menuState.cols; j++) {
            short tileIdx = i * menuState.cols + j;

            float x = menuState.spacing * (j + 1) + outlineWidth * j;
            float y = menuState.titleBarHeight + menuState.spacing * (i + 1) + outlineHeight * i;
            float width = outlineWidth;
            float height = outlineHeight;

            Rectangle tile = {
                .x = x,
                .y = y,
                .width = width,
                .height = height,
            };
            Color tileColor = (i == menuState.selectedTile.y && j == menuState.selectedTile.x) ? MAROON : BLACK;
            DrawRectangleLinesEx(tile, 5.0f, tileColor);

            const char *tileName = tileNames[tileIdx];
            DrawText(tileName,
                     x + width / 2 - MeasureText(tileName, 20) / 2, y + height / 2 - 10,
                     20, BLACK);
        }
    }
}

// Flip the pixels between (x1, y1) and (x2, y2) using Bresenham's algorithm generalized to work
// with any slope. Credit: https://www.uobabylon.edu.iq/eprints/publication_2_22893_6215.pdf.
void line(bool grid[ROWS][COLS], int x1, int y1, int x2, int y2) {
    int dx, dy, x, y, e, a, b, s1, s2, swapped = 0, temp;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    s1 = getSign(x2 - x1);
    s2 = getSign(y2 - y1);

    if (dy > dx) {
        temp = dx;
        dx = dy;
        dy = temp;
        swapped = 1;
    }

    e = 2 * dy - dx;
    a = 2 * dy;
    b = 2 * dy - 2 * dx;

    x = x1;
    y = y1;
    for (int i = 1; i < dx; i++) {
        grid[y][x] = !grid[y][x];

        if (e < 0) {
            if (swapped)
                y = y + s2;
            else
                x = x + s1;
            e = e + a;
        } else {
            y = y + s2;
            x = x + s1;
            e = e + b;
        }
    }
}

void lineV(bool grid[ROWS][COLS], Vector2 p1, Vector2 p2) {
    line(grid, p1.x, p1.y, p2.x, p2.y);
}

void rectangle(bool grid[ROWS][COLS], Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4) {
    lineV(grid, p1, p2);
    lineV(grid, p2, p3);
    lineV(grid, p3, p4);
    lineV(grid, p4, p1);
}

void circle(bool grid[ROWS][COLS], Vector2 origin, int radius) {
    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            if (round(sqrt((x - origin.x) * (x - origin.x) + (y - origin.y) * (y - origin.y))) == radius) {
                grid[y][x] = !grid[y][x];
            }
        }
    }
}

void parseMaskFromPbm(const char *filePath, DvdState *dvdState) {
    Nob_String_Builder sbContent = {0};
    if (!nob_read_entire_file(filePath, &sbContent)) exit(1);
    Nob_String_View svContent = nob_sv_from_parts(sbContent.items, sbContent.count);

    size_t linesCount = 0;
    bool *mask;
    for (; svContent.count > 0; ++linesCount) {
        Nob_String_View line = nob_sv_chop_by_delim(&svContent, '\n');
        if (linesCount == 0) {
            if (!nob_sv_eq(line, nob_sv_from_cstr("P1"))) {
                nob_log(NOB_ERROR, "Invalid file format. Expected .pbm with magic number P1, got " SV_Fmt ".", SV_Arg(line));
                exit(1);
            }
        }

        if (linesCount == 1) {
            dvdState->maskWidth = strtol(nob_sv_chop_by_delim(&line, ' ').data, NULL, 10);
            dvdState->maskHeight = strtol(line.data, NULL, 10);
            if (dvdState->maskWidth == 0 || dvdState->maskHeight == 0) {
                nob_log(NOB_ERROR, "Unexpected dimension in the %s file: %dx%d", filePath, dvdState->maskWidth, dvdState->maskHeight);
                exit(1);
            }
            if (dvdState->maskWidth >= COLS) {
                nob_log(NOB_ERROR, "Mask too wide, should be less than %d, got %d.", COLS, dvdState->maskWidth);
                exit(1);
            }
            if (dvdState->maskHeight >= ROWS) {
                nob_log(NOB_ERROR, "Mask too tall, should be less than %d, got %d.", ROWS, dvdState->maskHeight);
                exit(1);
            }

            mask = malloc(dvdState->maskWidth * dvdState->maskHeight * sizeof(bool));
            if (!mask) {
                nob_log(NOB_ERROR, "No RAM?");
                exit(1);
            }
        } else {
            for (int i = 0; i < dvdState->maskWidth; i++) {
                int color;  // 0 for white, 1 for black in the .pbm format
                if (i == dvdState->maskWidth - 1) {
                    color = strtol(nob_temp_sv_to_cstr(line), NULL, 10);
                } else {
                    color = strtol(nob_temp_sv_to_cstr(nob_sv_chop_by_delim(&line, ' ')), NULL, 10);
                }

                if (color < 0 || color > 1) {
                    nob_log(NOB_ERROR, "Unexpected color in the %s file, expected 0 or 1, got: %d.", filePath, color);
                    exit(1);
                }

                mask[(linesCount - 2) * dvdState->maskWidth + i] = !color;
            }
        }
    }

    dvdState->mask = mask;
}

void dvd(bool grid[ROWS][COLS], DvdState dvdState) {
    for (int y = dvdState.origin.y; y < dvdState.origin.y + dvdState.maskHeight; y++) {
        for (int x = dvdState.origin.x; x < dvdState.origin.x + dvdState.maskWidth; x++) {
            int maskX = x - dvdState.origin.x;
            int maskY = y - dvdState.origin.y;

            if (dvdState.mask[dvdState.maskWidth * maskY + maskX]) grid[y][x] = !grid[y][x];
        }
    }
}

int main(void) {
    SetRandomSeed(time(NULL));

    bool grid[ROWS][COLS] = {false};
    initGrid(grid);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "pov: brain is weird");
    SetExitKey(KEY_NULL);
    SetWindowIcon(LoadImage("./resources/pov-you-wake-up-in-poland.png"));
    SetTargetFPS(60);

    Screen currentScreen = MENU;
    MenuState menuState = {
        .rows = 2,
        .cols = 3,
        .spacing = 40,
        .titleBarHeight = 40,
        .selectedTile = {0, 0},
    };

    LinesState linesState = {
        .p1 = {0},
        .p2 = {0}};

    ClockState clockState = {
        .radius = ROWS / 2 * 3 / 4,
        .handOrigin = {COLS / 2, ROWS / 2},
        .handDest = {COLS / 2, ROWS / 2 - clockState.radius}};

    DvdState dvdState = {0};
    parseMaskFromPbm("./resources/dvd.pbm", &dvdState);
    dvdState.direction = (Vector2){1, 1};
    int originX = GetRandomValue(0, COLS - dvdState.maskWidth);
    int originY = GetRandomValue(0, ROWS - dvdState.maskHeight);
    dvdState.origin = (Vector2){originX, originY};

    CubeState cubeState = {
        .points = {
            {-1, -1,  1},
            { 1, -1,  1},
            { 1,  1,  1},
            {-1,  1,  1},
            {-1, -1, -1},
            { 1, -1, -1},
            { 1,  1, -1},
            {-1,  1, -1}},
        .projectedPoints = {{0}},
        // from -2 to 2 radians with a 0.1 step
        .rollAngle = GetRandomValue(-20, 20) / 20.0,
        .pitchAngle = GetRandomValue(-20, 20) / 20.0,
        .yawAngle = GetRandomValue(-20, 20) / 20.0,
        // from 0.01 to 0.05 with a 0.001 step
        .rollStep = GetRandomValue(10, 50) / 1000.0,
        .pitchStep = GetRandomValue(10, 50) / 1000.0,
        .yawStep = GetRandomValue(10, 50) / 1000.0,
    };

    printf("%f", cubeState.rollStep);
    printf("%f", cubeState.pitchStep);
    printf("%f", cubeState.yawStep);
        
    bool paused = false;
    unsigned int frameCount = 0;
    while (!WindowShouldClose()) {
        frameCount = (frameCount + 1) % 60;

        switch (currentScreen) {
            case MENU: {
                if (IsKeyPressed(KEY_ENTER)) {
                    short screenIdx = menuState.selectedTile.y * menuState.cols + menuState.selectedTile.x;
                    currentScreen = screens[screenIdx];
                }

                if (IsKeyPressed(KEY_LEFT))
                    menuState.selectedTile.x = euclideanModulo((int)menuState.selectedTile.x - 1, menuState.cols);
                if (IsKeyPressed(KEY_UP))
                    menuState.selectedTile.y = euclideanModulo((int)menuState.selectedTile.y - 1, menuState.rows);
                if (IsKeyPressed(KEY_RIGHT))
                    menuState.selectedTile.x = ((int)menuState.selectedTile.x + 1) % menuState.cols;
                if (IsKeyPressed(KEY_DOWN))
                    menuState.selectedTile.y = ((int)menuState.selectedTile.y + 1) % menuState.rows;
            } break;

            case LINES:
            case CLOCK:
            case DVD:
            case CUBE:
            default: {
                if (IsKeyPressed(KEY_ESCAPE)) currentScreen = MENU;

                if (IsKeyPressed(KEY_P)) paused = !paused;
            } break;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        switch (currentScreen) {
            case MENU: {
                DrawText("pov: brain is weird",
                         WINDOW_WIDTH / 2 - MeasureText("pov: brain is weird", 20) / 2, 10,
                         20, BLACK);

                Vector2 separatorStart = {0, menuState.titleBarHeight};
                Vector2 separatorEnd = {WINDOW_WIDTH, menuState.titleBarHeight};
                DrawLineEx(separatorStart, separatorEnd, 3, BLACK);

                drawMenuTiles(menuState);
            } break;

            case LINES: {
                if (!paused && frameCount % 15 == 0) {
                    linesState.p1.x = GetRandomValue(0, COLS);
                    linesState.p1.y = GetRandomValue(0, ROWS);
                    linesState.p2.x = GetRandomValue(0, COLS);
                    linesState.p2.y = GetRandomValue(0, ROWS);
                    lineV(grid, linesState.p1, linesState.p2);
                }

                drawGrid(grid);
            } break;

            case CLOCK: {
                if (!paused && frameCount % 3 == 0) circle(grid, clockState.handOrigin, clockState.radius);

                if (!paused && frameCount == 0) {
                    Vector2 v = {clockState.handDest.x - clockState.handOrigin.x, clockState.handDest.y - clockState.handOrigin.y};
                    v.x = v.x * cos(CLOCK_STEP) - v.y * sin(CLOCK_STEP);
                    v.y = v.x * sin(CLOCK_STEP) + v.y * cos(CLOCK_STEP);

                    clockState.handDest.x = round(clockState.handOrigin.x + v.x);
                    clockState.handDest.y = round(clockState.handOrigin.y + v.y);

                    lineV(grid, clockState.handOrigin, clockState.handDest);
                }

                drawGrid(grid);
            } break;

            case DVD: {
                if (!paused && frameCount % 2 == 0) {
                    // collision checks
                    // top
                    if (dvdState.origin.y == 0)
                        dvdState.direction.y = 1;
                    // right
                    if (dvdState.origin.x + dvdState.maskWidth == COLS)
                        dvdState.direction.x = -1;
                    // bottom
                    if (dvdState.origin.y + dvdState.maskHeight == ROWS)
                        dvdState.direction.y = -1;
                    // left
                    if (dvdState.origin.x == 0)
                        dvdState.direction.x = 1;

                    dvdState.origin.x += dvdState.direction.x;
                    dvdState.origin.y += dvdState.direction.y;
                    dvd(grid, dvdState);
                }

                drawGrid(grid);
            } break;

            case CUBE: {
                if (!paused) {
                    for (int i = 0; i < 8; i++) {
                        Vector2 projectedPoint = project(cubeState.points[i], cubeState.rollAngle, cubeState.pitchAngle, cubeState.yawAngle);
                        
                        projectedPoint.x = floor(projectedPoint.x * ROWS / 4);
                        projectedPoint.y = floor(projectedPoint.y * ROWS / 4);
                        projectedPoint.x += COLS / 2;
                        projectedPoint.y += ROWS / 2;

                        cubeState.projectedPoints[i] = projectedPoint;
                    }

                    lineV(grid, cubeState.projectedPoints[0], cubeState.projectedPoints[1]);
                    lineV(grid, cubeState.projectedPoints[1], cubeState.projectedPoints[2]);
                    lineV(grid, cubeState.projectedPoints[2], cubeState.projectedPoints[3]);
                    lineV(grid, cubeState.projectedPoints[3], cubeState.projectedPoints[0]);

                    lineV(grid, cubeState.projectedPoints[4], cubeState.projectedPoints[5]);
                    lineV(grid, cubeState.projectedPoints[5], cubeState.projectedPoints[6]);
                    lineV(grid, cubeState.projectedPoints[6], cubeState.projectedPoints[7]);
                    lineV(grid, cubeState.projectedPoints[7], cubeState.projectedPoints[4]);

                    lineV(grid, cubeState.projectedPoints[0], cubeState.projectedPoints[4]);
                    lineV(grid, cubeState.projectedPoints[1], cubeState.projectedPoints[5]);
                    lineV(grid, cubeState.projectedPoints[2], cubeState.projectedPoints[6]);
                    lineV(grid, cubeState.projectedPoints[3], cubeState.projectedPoints[7]);

                    cubeState.rollAngle += cubeState.rollStep;
                    cubeState.pitchAngle += cubeState.pitchStep;
                    cubeState.yawAngle += cubeState.yawStep;

                    if (cubeState.rollAngle < -2 * PI || cubeState.rollAngle > 2 * PI) {
                        cubeState.rollStep *= -1;
                    }

                    if (cubeState.pitchAngle < -2 * PI || cubeState.pitchAngle > 2 * PI) {
                        cubeState.pitchStep *= -1;
                    }

                    if (cubeState.yawAngle < -2 * PI || cubeState.yawAngle > 2 * PI) {
                        cubeState.yawStep *= -1;
                    }
                }

                drawGrid(grid);
            } break;

            default: {
                DrawText("you shouldn't be here",
                         WINDOW_WIDTH / 2 - MeasureText("you shouldn't be here", 20) / 2, WINDOW_HEIGHT / 2 - 10,
                         20, BLACK);
            } break;
        }

        EndDrawing();
    }

    CloseWindow();

    free(dvdState.mask);

    return 0;
}