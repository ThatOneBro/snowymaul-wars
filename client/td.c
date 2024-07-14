#include "raylib.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#ifdef __GNUC__ // GCC, Clang, ICC
#define UNREACHABLE() (__builtin_unreachable())
#endif

#ifdef _MSC_VER // MSVC
#define UNREACHABLE() (__assume(false))
#endif

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450
#define SQUARE_SIZE 31
#define MAX_TOWERS 100
#define MAX_MINIONS 100
#define MAX_PROJECTILES 5000
#define STARTING_MINION_WAVE_SIZE 5
#define TOWER_COST 10
#define STARTING_GOLD 100

// Towers
#define DEFAULT_TOWER_HEALTH 100
#define DEFAULT_TOWER_POWER 10
#define DEFAULT_TOWER_COLOR LIGHTGRAY

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef enum DrawStyle {
    SOLID,
    BORDER_ONLY,
} DrawStyle;

typedef struct Cursor {
    Vector2 position;
    Vector2 size;
    Color color;
    DrawStyle style;
} Cursor;

typedef struct Tower {
    bool alive;
    Vector2 position;
    Vector2 size;
    int max_health;
    int curr_health;
    Color color;
} Tower;

typedef struct Bullet {
    bool alive;
    Vector2 position;
    Vector2 size;
    int base_power;
    Vector2 velocity;
    Color color;
} Bullet;

typedef struct Minion {
    bool alive;
    Vector2 position;
    Vector2 size;
    int max_health;
    int curr_health;
    Vector2 velocity;
    Color color;
} Minion;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = SCREEN_WIDTH;
static const int screenHeight = SCREEN_HEIGHT;

static int framesCounter = 0;
static bool gameOver = false;
static bool pause = false;
static int gold = STARTING_GOLD;
static unsigned int currentTowers = 0;
static unsigned int currentBullets = 0;
static unsigned int currentMinions = 0;

static Cursor cursor = { 0 };
static Tower towers[MAX_TOWERS] = { 0 };
static Bullet bullets[MAX_PROJECTILES] = { 0 };
static Minion minions[MAX_MINIONS] = { 0 };
static bool allowMove = false;
static Vector2 offset = { 0 };

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void); // Initialize game
static void UpdateGame(void); // Update game (one frame)
static void DrawGame(void); // Draw game (one frame)
static void UnloadGame(void); // Unload game
static void UpdateDrawFrame(void); // Update and Draw (one frame)

// Cleanup
static void run_defrag(void); // Run defrag on entity arrays

// Custom draw functions
static void draw_rect(Vector2 pos, Vector2 size, Color color, DrawStyle style); // Draws a rectangle

// Custom logic functions
static bool maybe_purchase_tower(Cursor cursor); // Attempt to purchase tower

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Snowymaul TD");

    InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame(); // Unload loaded data (textures, sounds, models...)

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

void run_defrag()
{
    int i = 0;
    int j = MAX_MINIONS - 1;
    while (i < j) {
        if (!minions[j].alive) {
            j--;
            continue;
        }
        if (minions[i].alive) {
            i++;
            continue;
        }
        minions[i] = minions[j];
        minions[j].alive = false;
    }

    i = 0;
    j = MAX_TOWERS - 1;
    while (i < j) {
        if (!towers[j].alive) {
            j--;
            continue;
        }
        if (towers[i].alive) {
            i++;
            continue;
        }
        towers[i] = towers[j];
        towers[j].alive = false;
    }

    i = 0;
    j = MAX_PROJECTILES - 1;
    while (i < j) {
        if (!bullets[j].alive) {
            j--;
            continue;
        }
        if (bullets[i].alive) {
            i++;
            continue;
        }
        bullets[i] = bullets[j];
        bullets[j].alive = false;
    }
}

void draw_rect(Vector2 pos, Vector2 size, Color color, DrawStyle style)
{
    switch (style) {
    case BORDER_ONLY:
        DrawRectangleLines(pos.x, pos.y, size.x, size.y, color);
        break;
    case SOLID:
        DrawRectangleV(pos, size, color);
        break;
    default:
        UNREACHABLE();
    }
}

static inline bool is_same_position(Vector2 pos1, Vector2 pos2)
{
    return pos1.x == pos2.x && pos1.y == pos2.y;
}

static inline bool is_tower_at_position(Vector2 pos)
{
    int tower_count = currentTowers;
    for (int i = 0; i < MAX_TOWERS; i++) {
        if (!towers[i].alive) {
            continue;
        }
        if (is_same_position(towers[i].position, pos)) {
            return true;
        }
        if (--tower_count == 0) {
            break;
        }
    }
    return false;
}

static inline void create_tower_at_pos(Vector2 pos)
{
    for (int i = 0; i < MAX_TOWERS; i++) {
        if (!towers[i].alive) {
            towers[i].alive = true;
            towers[i].max_health = DEFAULT_TOWER_HEALTH;
            towers[i].curr_health = DEFAULT_TOWER_HEALTH;
            towers[i].color = DEFAULT_TOWER_COLOR;
            towers[i].position = pos;
            towers[i].size = (Vector2) { SQUARE_SIZE, SQUARE_SIZE };

            if (i >= MAX_TOWERS - 10) {
                run_defrag();
            }
            break;
        }
    }
    currentTowers++;
}

bool maybe_purchase_tower(Cursor cursor)
{
    if (gold >= TOWER_COST && !is_tower_at_position(cursor.position)) {
        gold -= TOWER_COST;
        create_tower_at_pos(cursor.position);
        return true;
    }
    return false;
}

// Initialize game variables
void InitGame(void)
{
    framesCounter = 0;
    gameOver = false;
    pause = false;

    allowMove = false;

    offset.x = screenWidth % SQUARE_SIZE;
    offset.y = screenHeight % SQUARE_SIZE;

    cursor.style = BORDER_ONLY;
    cursor.position = (Vector2) { SQUARE_SIZE + offset.x / 2, SQUARE_SIZE + offset.y / 2 };
    cursor.size = (Vector2) { SQUARE_SIZE, SQUARE_SIZE };
    cursor.color = DARKBLUE;

    // for (int i = 0; i < STARTING_MINION_WAVE_SIZE; i++) {
    //     minions[i].position = (Vector2) { offset.x / 2, offset.y / 2 };
    //     minions[i].size = (Vector2) { SQUARE_SIZE, SQUARE_SIZE };
    //     minions[i].velocity = (Vector2) { SQUARE_SIZE, 0 };
    //     minions[i].alive = true;
    //     currentMinions++;
    // }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver) {
        if (IsKeyPressed('P'))
            pause = !pause;

        allowMove = true;

        if (!pause) {
            // Player control
            if (IsKeyPressed(KEY_RIGHT) && allowMove) {
                cursor.position.x += SQUARE_SIZE;
                allowMove = false;
            }
            if (IsKeyPressed(KEY_LEFT) && allowMove) {
                cursor.position.x -= SQUARE_SIZE;
                allowMove = false;
            }
            if (IsKeyPressed(KEY_UP) && allowMove) {
                cursor.position.y -= SQUARE_SIZE;
                allowMove = false;
            }
            if (IsKeyPressed(KEY_DOWN) && allowMove) {
                cursor.position.y += SQUARE_SIZE;
                allowMove = false;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                maybe_purchase_tower(cursor);
            }

            framesCounter++;
        }
    } else {
        if (IsKeyPressed(KEY_ENTER)) {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    if (!gameOver) {
        // Draw grid lines
        for (int i = 0; i < screenWidth / SQUARE_SIZE + 1; i++) {
            DrawLineV((Vector2) { SQUARE_SIZE * i + offset.x / 2, offset.y / 2 }, (Vector2) { SQUARE_SIZE * i + offset.x / 2, screenHeight - offset.y / 2 }, LIGHTGRAY);
        }

        for (int i = 0; i < screenHeight / SQUARE_SIZE + 1; i++) {
            DrawLineV((Vector2) { offset.x / 2, SQUARE_SIZE * i + offset.y / 2 }, (Vector2) { screenWidth - offset.x / 2, SQUARE_SIZE * i + offset.y / 2 }, LIGHTGRAY);
        }

        // Draw cursor
        // We draw this first after drawing the grid since renderer will already be in line mode
        // Switching between line mode and normal draw mode triggers a flush
        draw_rect(cursor.position, cursor.size, cursor.color, cursor.style);

        // Iterate all towers
        int tower_count = currentTowers;
        for (int i = 0; i < MAX_TOWERS; i++) {
            if (!towers[i].alive) {
                continue;
            }
            draw_rect(towers[i].position, towers[i].size, towers[i].color, SOLID);
            // We can break early if we already have found all of the towers and drawn them
            if (--tower_count == 0) {
                break;
            }
        }

        const char *gold_text = TextFormat("GOLD: %d", gold);
        DrawText(gold_text, screenWidth - MeasureText(gold_text, 25) - 10, 10, 25, GRAY);

        if (pause)
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40, GRAY);
    } else
        DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth() / 2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2, GetScreenHeight() / 2 - 50, 20, GRAY);

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}
