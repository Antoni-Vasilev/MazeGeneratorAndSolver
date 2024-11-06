#define UNICODE
#include <math.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define MAX 2000

static bool quit = false;

struct {
    int width;
    int height;
    uint32_t *pixels;
} frame = {0};

typedef struct {
    bool isEdited;
    bool top, bottom, left, right;
} MazePart;

typedef struct {
    int x, y;
} Point;

typedef struct Stack {
    int arr[MAX];
    int top; // Index of the top element
} Stack;

// Initialize the stack
void initStack(Stack *stack) {
    stack->top = -1; // Indicates an empty stack
}

// Check if the stack is empty
int isEmpty(Stack *stack) {
    return stack->top == -1;
}

// Check if the stack is full
int isFull(Stack *stack) {
    return stack->top == MAX - 1;
}

// Push an element onto the stack
void push(Stack *stack, int value) {
    if (isFull(stack)) {
        printf("Stack overflow\n");
        return;
    }
    stack->arr[++stack->top] = value; // Increment top and insert the value
    printf("Pushed %d onto the stack\n", value);
}

// Pop an element from the stack
int pop(Stack *stack) {
    if (isEmpty(stack)) {
        printf("Stack underflow\n");
        return -1; // Return -1 as an error code
    }
    return stack->arr[stack->top--]; // Return top element and decrement top
}

// Peek the top element
int peek(Stack *stack) {
    if (isEmpty(stack)) {
        printf("Stack is empty\n");
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

void fillBox(int x, int y, int width, int height, uint32_t color);

void drawBox(int x, int y, int width, int height, int stroke_width, uint32_t color);

void drawBoxWithMaze(int x, int y, int width, int height, int stroke_width, MazePart part, uint32_t color);

int generateMaze();

Point player;
MazePart maze[1600];
Stack stack;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
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
    int window_width = 1920, window_height = 1200, width = 830, height = 850;
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
    initStack(&stack);

    bool isMazeStart = false;
    int mazeHeight = 40, mazeWidth = 40;
    while (!quit) {
        static MSG message = {0};
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) { DispatchMessage(&message); }

        // Clear maze
        if (!isMazeStart) {
            int r = rand() % (4 - 1 + 1) + 1;
            player.x = rand() % (mazeWidth - 0 + 0) + 0;
            player.y = rand() % (mazeHeight - 0 + 0) + 0;
            int mazeSize = sizeof(maze) / sizeof(maze[0]);
            for (int i = 0; i < mazeSize; i++) {
                MazePart part = {false, true, true, true, true};
                maze[i] = part;
            }
            isMazeStart = true;
        }

        fillBox(0, 0, frame.width, frame.height, 0);
        int result = generateMaze();
        if (result == true) {
            isMazeStart = false;
        }

        int size = 20;
        for (int y = 0; y < mazeHeight; y++) {
            for (int x = 0; x < mazeWidth; x++) {
                int index = y * mazeWidth + x;
                if (index < sizeof(maze) / sizeof(maze[0])) {
                    drawBoxWithMaze(x * size, y * size, size, size, 1, maze[x + y * mazeWidth],
                                    RGB(50, 50, 50));
                }

                if (player.x == x && player.y == y) {
                    fillBox(x * size, y * size, size, size, RGB(0, 255, 0));
                }
            }
        }

        InvalidateRect(window_handle, NULL, FALSE);
        UpdateWindow(window_handle);
    }

    return 0;
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

Point getPointFromMaze(MazePart maze[]) {
    Point point;
    int mazeSize = sizeof(maze) / sizeof(maze[0]);
    point.x = mazeSize % 40;
    point.y = mazeSize / 40;
}

static int generateLoop = 0;

int generateMaze() {
    int r = rand() % (4 - 1 + 1) + 1;

    if (generateLoop == 100) {
        player.y = pop(&stack);
        player.x = pop(&stack);
        generateLoop = 0;
    }

    if (player.x < 0 || player.y < 0) {
        return true;
    }

    if (r == 1 && player.y - 1 > -1 && !maze[player.x + (player.y - 1) * 40].isEdited) {
        maze[player.x + player.y * 40].top = false;
        maze[player.x + player.y * 40].isEdited = true;
        player.y -= 1;
        maze[player.x + player.y * 40].bottom = false;
        maze[player.x + player.y * 40].isEdited = true;

        generateLoop = 0;
        push(&stack, player.x);
        push(&stack, player.y);
    } else if (r == 2 && player.x + 1 < 40 && !maze[(player.x + 1) + player.y * 40].isEdited) {
        maze[player.x + player.y * 40].right = false;
        maze[player.x + player.y * 40].isEdited = true;
        player.x += 1;
        maze[player.x + player.y * 40].left = false;
        maze[player.x + player.y * 40].isEdited = true;

        generateLoop = 0;
        push(&stack, player.x);
        push(&stack, player.y);
    } else if (r == 3 && player.y + 1 < 40 && !maze[player.x + (player.y + 1) * 40].isEdited) {
        maze[player.x + player.y * 40].bottom = false;
        maze[player.x + player.y * 40].isEdited = true;
        player.y += 1;
        maze[player.x + player.y * 40].top = false;
        maze[player.x + player.y * 40].isEdited = true;

        generateLoop = 0;
        push(&stack, player.x);
        push(&stack, player.y);
    } else if (r == 4 && player.x - 1 > -1 && !maze[(player.x - 1) + player.y * 40].isEdited) {
        maze[player.x + player.y * 40].left = false;
        maze[player.x + player.y * 40].isEdited = true;
        player.x -= 1;
        maze[player.x + player.y * 40].right = false;
        maze[player.x + player.y * 40].isEdited = true;

        generateLoop = 0;
        push(&stack, player.x);
        push(&stack, player.y);
    } else {
        generateLoop++;
        generateMaze();
    }

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
