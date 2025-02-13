#define UNICODE
#include <math.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX 12800

static bool quit = false;

struct {
    int width;
    int height;
    uint32_t *pixels;
} frame = {0};

typedef struct {
    bool isEdited;
    bool isSolved, isReturned, isSolvedTop, isSolvedBottom, isSolvedLeft, isSolvedRight;
    bool top, bottom, left, right;
} MazePart;

typedef struct {
    int x, y;
} Point;

typedef struct Stack {
    int arr[MAX];
    int top;
} Stack;


void initStack(Stack *stack) {
    stack->top = -1;
}


int isEmpty(const Stack *stack) {
    return stack->top == -1;
}


int isFull(const Stack *stack) {
    return stack->top == MAX - 1;
}


void push(Stack *stack, int value) {
    if (isFull(stack)) {
        return;
    }
    stack->arr[++stack->top] = value;
}


int pop(Stack *stack) {
    if (isEmpty(stack)) {
        return -1;
    }
    return stack->arr[stack->top--];
}


int peek(const Stack *stack) {
    if (isEmpty(stack)) {
        return -1;
    }
    return stack->arr[stack->top];
}

LRESULT CALLBACK WindowProcessMessage(HWND, UINT, WPARAM, LPARAM);
#if RAND_MAX == 32767
#define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#else
#define Rand32() rand()
#endif

static BITMAPINFO frame_bitmap_info;
static HBITMAP frame_bitmap = 0;
static HDC frame_device_context = 0;

void drawLine(int x1, int y1, int x2, int y2, uint32_t color);

void fillBox(int x, int y, int width, int height, uint32_t color);

void drawBox(int x, int y, int width, int height, int stroke_width, uint32_t color);

void drawBoxWithMaze(int x, int y, int width, int height, int stroke_width, MazePart part, uint32_t color);

int generateMaze(int size, int isReturn, int generate);

int solveMaze(int size, int solve);

MazePart maze[1600];
Stack generatorHistory;
Point playerGenerator;
Stack solverHistory;
Point playerSolver;
Point endPoint;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
    int generate, solve, wait;
    int all = sscanf(pCmdLine, "%d %d %d", &generate, &solve, &wait);
    if (all == 0) {
        generate = -1;
        solve = 0;
        wait = 1000;
    } else if (all == 1) {
        solve = 0;
        wait = 1000;
    } else if (all == 2) {
        wait = 1000;
    }

    const wchar_t window_class_name[] = L"My Window Class";
    static WNDCLASS window_class = {0};
    window_class.lpfnWndProc = WindowProcessMessage;
    window_class.hInstance = hInstance;
    window_class.lpszClassName = window_class_name;
    RegisterClass(&window_class);

    frame_bitmap_info.bmiHeader.biSize = sizeof(frame_bitmap_info.bmiHeader);
    frame_bitmap_info.bmiHeader.biPlanes = 1;
    frame_bitmap_info.bmiHeader.biBitCount = 32;
    frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    frame_device_context = CreateCompatibleDC(0);

    static HWND window_handle;
    int window_width = 1920, window_height = 1200, width = 818, height = 840;
    window_handle = CreateWindow(window_class_name,
                                 L"Maze Generator And Solver",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 window_width / 2 - width / 2,
                                 window_height / 2 - height / 2,
                                 width,
                                 height,
                                 NULL,
                                 NULL,
                                 hInstance,
                                 NULL
    );
    if (window_handle == NULL) { return -1; }
    initStack(&generatorHistory);
    initStack(&solverHistory);

    bool isMazeStart = false;
    while (!quit) {
        const int mazeWidth = 40, mazeHeight = 40;
        static MSG message = {0};
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) { DispatchMessage(&message); }

        // Clear maze
        if (!isMazeStart) {
            playerGenerator.x = rand() % (mazeWidth - 0 + 0) + 0;
            playerGenerator.y = rand() % (mazeHeight - 0 + 0) + 0;
            playerSolver.x = rand() % (mazeWidth - 0 + 0) + 0;
            playerSolver.y = rand() % (mazeHeight - 0 + 0) + 0;
            endPoint.x = rand() % (mazeWidth - 0 + 0) + 0;
            endPoint.y = rand() % (mazeHeight - 0 + 0) + 0;
            while (!isEmpty(&solverHistory)) {
                pop(&solverHistory);
            }

            while (!isEmpty(&generatorHistory)) {
                pop(&generatorHistory);
            }

            int mazeSize = sizeof(maze) / sizeof(maze[0]);
            for (int i = 0; i < mazeSize; i++) {
                MazePart part = {false, false, false, false, false, false, false, true, true, true, true};
                maze[i] = part;
            }
            isMazeStart = true;
        }

        fillBox(0, 0, frame.width, frame.height, 0);

        int result = generateMaze(mazeHeight, false, generate);
        if (generate < 0) {
            while (!result) {
                result = generateMaze(mazeHeight, true, generate);
            }
        }

        if (result == true) {
            result = solveMaze(mazeHeight, solve);
            if (solve < 0) {
                while (!result) {
                    result = solveMaze(mazeHeight, solve);
                }
            }
        }

        if (result == true) {
            if (wait > 0) Sleep(wait);
            isMazeStart = false;
        }

        for (int y = 0; y < mazeHeight; y++) {
            for (int x = 0; x < mazeWidth; x++) {
                int size = 20;
                const MazePart part = maze[y * mazeWidth + x];

                int index = y * mazeWidth + x;
                if (index < sizeof(maze) / sizeof(maze[0])) {
                    drawBoxWithMaze(x * size, y * size, size, size, 1, maze[x + y * mazeWidth],
                                    RGB(50, 50, 50));
                }

                if (playerGenerator.x == x && playerGenerator.y == y) {
                    fillBox(x * size, y * size, size, size, RGB(0, 255, 0));
                }

                if (playerSolver.x == x && playerSolver.y == y) {
                    fillBox(x * size + 4, y * size + 4, size - 8, size - 8, RGB(0, 255, 0));
                }

                if (endPoint.x == x && endPoint.y == y) {
                    fillBox(x * size + 4, y * size + 4, size - 8, size - 8, RGB(0, 0, 255));
                }

                if (!part.top && part.isSolvedTop)
                    drawLine(x * size + size / 2, y * size, x * size + size / 2,
                             y * size + size / 2, RGB(0, 0, 255));
                if (!part.right && part.isSolvedRight)
                    drawLine(x * size + size / 2, y * size + size / 2, x * size + size,
                             y * size + size / 2, RGB(0, 0, 255));
                if (!part.bottom && part.isSolvedBottom)
                    drawLine(x * size + size / 2, y * size + size / 2,
                             x * size + size / 2, y * size + size, RGB(0, 0, 255));
                if (!part.left && part.isSolvedLeft)
                    drawLine(x * size + size / 2, y * size + size / 2, x * size,
                             y * size + size / 2, RGB(0, 0, 255));
            }
        }

        InvalidateRect(window_handle, NULL, FALSE);
        UpdateWindow(window_handle);
    }

    return 0;
}

void drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    const int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    const int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (true) {
        if (frame.width != 0 && frame.height != 0)
            frame.pixels[(frame.height - y1 - 1) * frame.width + x1] = color;

        if (x1 == x2 && y1 == y2) break;

        int e2 = err;

        if (e2 > -dx) {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dy) {
            err += dx;
            y1 += sy;
        }
    }
}

void fillBox(int x, int y, int width, int height, uint32_t color) {
    for (int yy = frame.height - y; yy > frame.height - height - y; yy--) {
        for (int xx = x; xx < width + x; xx++) {
            if (yy >= 0 && yy < frame.height && xx < frame.width && xx >= 0)
                frame.pixels[xx + yy * frame.width] = color;
        }
    }
}

void drawBox(int x, int y, int width, int height, int stroke_width, uint32_t color) {
    for (int yy = frame.height - y; yy > frame.height - height - y; yy--) {
        for (int xx = x; xx < width + x; xx++) {
            if ((
                    yy > frame.height - y - stroke_width
                    || yy <= frame.height - y - height + stroke_width
                    || xx < x + stroke_width
                    || xx >= x + width - stroke_width
                ) && yy >= 0 && yy < frame.height && xx < frame.width && xx >= 0)
                frame.pixels[xx + yy * frame.width] = color;
        }
    }
}

void drawBoxWithMaze(int x, int y, int width, int height, int stroke_width, MazePart part, uint32_t color) {
    for (int yy = frame.height - y; yy > frame.height - height - y; yy--) {
        for (int xx = x; xx < width + x; xx++) {
            if ((
                    (yy > frame.height - y - stroke_width && part.top)
                    || (yy <= frame.height - y - height + stroke_width && part.bottom)
                    || (xx < x + stroke_width && part.left)
                    || (xx >= x + width - stroke_width && part.right)
                ) && yy >= 0 && yy < frame.height && xx < frame.width && xx >= 0)
                frame.pixels[xx + yy * frame.width] = color;
        }
    }
}

static int generateLoop = 0;

int generateMaze(int size, int isReturn, int generate) {
    if (!isReturn && generate > 0) Sleep(generate);

    int r = rand() % (4 - 1 + 1) + 1;

    if (generateLoop == 100) {
        playerGenerator.y = pop(&generatorHistory);
        playerGenerator.x = pop(&generatorHistory);
        generateLoop = 0;
    }

    if (playerGenerator.x < 0 || playerGenerator.y < 0) {
        return true;
    }

    if (r == 1 && playerGenerator.y - 1 > -1 && !maze[playerGenerator.x + (playerGenerator.y - 1) * size].isEdited) {
        maze[playerGenerator.x + playerGenerator.y * size].top = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;
        playerGenerator.y -= 1;
        maze[playerGenerator.x + playerGenerator.y * size].bottom = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;

        generateLoop = 0;
        push(&generatorHistory, playerGenerator.x);
        push(&generatorHistory, playerGenerator.y);
    } else if (r == 2 && playerGenerator.x + 1 < size && !maze[(playerGenerator.x + 1) + playerGenerator.y * size].
               isEdited) {
        maze[playerGenerator.x + playerGenerator.y * size].right = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;
        playerGenerator.x += 1;
        maze[playerGenerator.x + playerGenerator.y * size].left = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;

        generateLoop = 0;
        push(&generatorHistory, playerGenerator.x);
        push(&generatorHistory, playerGenerator.y);
    } else if (r == 3 && playerGenerator.y + 1 < size && !maze[playerGenerator.x + (playerGenerator.y + 1) * size].
               isEdited) {
        maze[playerGenerator.x + playerGenerator.y * size].bottom = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;
        playerGenerator.y += 1;
        maze[playerGenerator.x + playerGenerator.y * size].top = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;

        generateLoop = 0;
        push(&generatorHistory, playerGenerator.x);
        push(&generatorHistory, playerGenerator.y);
    } else if (r == 4 && playerGenerator.x - 1 > -1 && !maze[(playerGenerator.x - 1) + playerGenerator.y * size].
               isEdited) {
        maze[playerGenerator.x + playerGenerator.y * size].left = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;
        playerGenerator.x -= 1;
        maze[playerGenerator.x + playerGenerator.y * size].right = false;
        maze[playerGenerator.x + playerGenerator.y * size].isEdited = true;

        generateLoop = 0;
        push(&generatorHistory, playerGenerator.x);
        push(&generatorHistory, playerGenerator.y);
    } else {
        generateLoop++;
        generateMaze(size, true, generate);
    }

    return false;
}

int isReturn = false;

double calculateDistance(Point p1, Point p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

int solveMaze(int size, int solve) {
    if (solve > 0) Sleep(solve);

    if (isReturn == true) {
        isReturn = false;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
    }

    if (playerSolver.x < 0 || playerSolver.y < 0 || (playerSolver.x == endPoint.x && playerSolver.y == endPoint.y)) {
        return true;
    }

    int side = 0;
    int minDistance = 10000;

    const Point nPlayer1 = {playerSolver.x, playerSolver.y - 1};
    int currentDistance = calculateDistance(nPlayer1, endPoint);
    if (playerSolver.y - 1 > -1 && !maze[playerSolver.x + (playerSolver.y - 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].top && minDistance > currentDistance) {
        side = 0;
        minDistance = currentDistance;
    }

    const Point nPlayer2 = {playerSolver.x + 1, playerSolver.y};
    currentDistance = calculateDistance(nPlayer2, endPoint);
    if (playerSolver.x + 1 < size && !maze[(playerSolver.x + 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].right && minDistance > currentDistance) {
        side = 1;
        minDistance = currentDistance;
    }

    const Point nPlayer3 = {playerSolver.x, playerSolver.y + 1};
    currentDistance = calculateDistance(nPlayer3, endPoint);
    if (playerSolver.y + 1 < size && !maze[playerSolver.x + (playerSolver.y + 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].bottom && minDistance > currentDistance) {
        side = 2;
        minDistance = currentDistance;
    }

    const Point nPlayer4 = {playerSolver.x - 1, playerSolver.y};
    currentDistance = calculateDistance(nPlayer4, endPoint);
    if (playerSolver.x - 1 > -1 && !maze[(playerSolver.x - 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].left && minDistance > currentDistance) {
        side = 3;
    }

    if (side == 0 && playerSolver.y - 1 > -1 && !maze[playerSolver.x + (playerSolver.y - 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].top) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedTop = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.y -= 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedBottom = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (side == 1 && playerSolver.x + 1 < size && !maze[(playerSolver.x + 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].right) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedRight = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.x += 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedLeft = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (side == 2 && playerSolver.y + 1 < size && !maze[playerSolver.x + (playerSolver.y + 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].bottom) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedBottom = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.y += 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedTop = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (side == 3 && playerSolver.x - 1 > -1 && !maze[(playerSolver.x - 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].left) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedLeft = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.x -= 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedRight = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }


    // Return
    if (playerSolver.y - 1 > -1 && maze[playerSolver.x + (playerSolver.y - 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].top && !maze[playerSolver.x + (playerSolver.y - 1) * size].
        isReturned) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedTop = false;
        maze[playerSolver.x + playerSolver.y * size].isReturned = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.y -= 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedBottom = false;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (playerSolver.x + 1 < size && maze[(playerSolver.x + 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].right && !maze[(playerSolver.x + 1) + playerSolver.y * size].
        isReturned) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedRight = false;
        maze[playerSolver.x + playerSolver.y * size].isReturned = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.x += 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedLeft = false;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (playerSolver.y + 1 < size && maze[playerSolver.x + (playerSolver.y + 1) * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].bottom && !maze[playerSolver.x + (playerSolver.y + 1) * size].
        isReturned) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedBottom = false;
        maze[playerSolver.x + playerSolver.y * size].isReturned = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.y += 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedTop = false;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }
    if (playerSolver.x - 1 > -1 && maze[(playerSolver.x - 1) + playerSolver.y * size].isSolved && !maze[
            playerSolver.x + playerSolver.y * size].left && !maze[(playerSolver.x - 1) + playerSolver.y * size].
        isReturned) {
        maze[playerSolver.x + playerSolver.y * size].isSolvedLeft = false;
        maze[playerSolver.x + playerSolver.y * size].isReturned = true;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;
        playerSolver.x -= 1;
        maze[playerSolver.x + playerSolver.y * size].isSolvedRight = false;
        maze[playerSolver.x + playerSolver.y * size].isSolved = true;

        generateLoop = 0;
        push(&solverHistory, playerSolver.x);
        push(&solverHistory, playerSolver.y);
        return false;
    }

    printf("Stuck");
    return false;
}

LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_QUIT:
        case WM_DESTROY: {
            quit = true;
        }
        break;

        case WM_PAINT: {
            static PAINTSTRUCT paint;
            static HDC device_context;
            device_context = BeginPaint(window_handle, &paint);
            BitBlt(device_context,
                   paint.rcPaint.left,
                   paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left,
                   paint.rcPaint.bottom - paint.rcPaint.top,
                   frame_device_context,
                   paint.rcPaint.left,
                   paint.rcPaint.top,
                   SRCCOPY
            );
            EndPaint(window_handle, &paint);
        }
        break;

        case WM_SIZE: {
            frame_bitmap_info.bmiHeader.biWidth = LOWORD(lParam);
            frame_bitmap_info.bmiHeader.biHeight = HIWORD(lParam);

            if (frame_bitmap) DeleteObject(frame_bitmap);
            frame_bitmap = CreateDIBSection(NULL, &frame_bitmap_info, DIB_RGB_COLORS, (void **) &frame.pixels, 0, 0);
            SelectObject(frame_device_context, frame_bitmap);

            frame.width = LOWORD(lParam);
            frame.height = HIWORD(lParam);
        }
        break;

        default: {
            return DefWindowProc(window_handle, message, wParam, lParam);
        }
    }
    return 0;
}
