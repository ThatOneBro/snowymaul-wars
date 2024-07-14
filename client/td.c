#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

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
#define SQUARE_SIZE 32

// Rendering stuff
#define BORDER_THICKNESS 2
#define CURSOR_COLOR GOLD

// Entity constants
#define MAX_TOWERS 100
#define MAX_MINIONS 100
#define MAX_PROJECTILES 5000
#define STARTING_MINION_WAVE_SIZE 5
#define TOWER_COST 10
#define STARTING_GOLD 100

// Towers
#define DEFAULT_TOWER_HEALTH 100
#define DEFAULT_TOWER_POWER 10
#define DEFAULT_TOWER_COLOR SKYBLUE

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef enum DrawStyle {
    SOLID,
    BORDER_ONLY,
} DrawStyle;

typedef struct SlotVector2 {
    unsigned int x;
    unsigned int y;
} SlotVector2;

typedef struct Cursor {
    Vector2 position;
    Vector2 size;
    Color color;
    DrawStyle style;
} Cursor;

typedef struct Tower {
    bool alive;
    SlotVector2 slot_pos;
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

// TODO: Use a bitset
static bool slots_occupied[SCREEN_WIDTH / (SQUARE_SIZE + 1)][SCREEN_HEIGHT / (SQUARE_SIZE + 1)] = { 0 };
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

static inline Rectangle pos_and_size_to_rect(Vector2 pos, Vector2 size)
{
    return (Rectangle) { .x = pos.x, .y = pos.y, .width = size.x, .height = size.y };
}

static inline SlotVector2 world_pos_to_slot_space(Vector2 pos)
{
    return (SlotVector2) { .x = pos.x / SQUARE_SIZE, .y = pos.y / SQUARE_SIZE };
}

static inline Vector2 slot_pos_to_world_space_origin(SlotVector2 slot_pos)
{
    return (Vector2) { .x = slot_pos.x * SQUARE_SIZE, .y = slot_pos.y * SQUARE_SIZE };
}

static inline bool is_same_world_pos(Vector2 pos1, Vector2 pos2)
{
    return pos1.x == pos2.x && pos1.y == pos2.y;
}

static inline bool is_same_slot_pos(SlotVector2 pos1, SlotVector2 pos2)
{
    return pos1.x == pos2.x && pos1.y == pos2.y;
}

static inline bool is_slot_occupied(SlotVector2 slot_pos)
{
    return slots_occupied[slot_pos.x][slot_pos.y];
}

static inline Vector2 get_slot_origin(SlotVector2 slot_pos)
{
    return (Vector2) { .x = (slot_pos.x * SQUARE_SIZE) - SQUARE_SIZE / 2, .y = (slot_pos.y * SQUARE_SIZE) - SQUARE_SIZE / 2 };
}

static inline Vector2 calc_position_centered_at_origin(Vector2 origin, Vector2 size)
{
    return (Vector2) { .x = origin.x + 1.5 * size.x, .y = origin.y + 1.5 * size.y };
}

static inline void create_tower_at_position(Vector2 pos)
{
    for (int i = 0; i < MAX_TOWERS; i++) {
        if (!towers[i].alive) {
            towers[i].alive = true;
            towers[i].max_health = DEFAULT_TOWER_HEALTH;
            towers[i].curr_health = DEFAULT_TOWER_HEALTH;
            towers[i].color = DEFAULT_TOWER_COLOR;
            towers[i].slot_pos = world_pos_to_slot_space(pos);
            towers[i].size = (Vector2) { SQUARE_SIZE / 2, SQUARE_SIZE / 2 };

            slots_occupied[towers[i].slot_pos.x][towers[i].slot_pos.y] = true;
            printf("DEBUG: slot_pos: %d, %d\n", towers[i].slot_pos.x, towers[i].slot_pos.y);

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
    if (gold >= TOWER_COST && !is_slot_occupied(world_pos_to_slot_space(cursor.position))) {
        gold -= TOWER_COST;
        create_tower_at_position(cursor.position);
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
    cursor.color = CURSOR_COLOR;
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

        // Iterate all towers
        int tower_count = currentTowers;
        for (int i = 0; i < MAX_TOWERS; i++) {
            if (!towers[i].alive) {
                continue;
            }
            Rectangle rect = pos_and_size_to_rect((Vector2) { 0, 0 }, towers[i].size);
            Vector2 origin = slot_pos_to_world_space_origin(towers[i].slot_pos);
            DrawRectangleV(calc_position_centered_at_origin(get_slot_origin(towers[i].slot_pos), towers[i].size), towers[i].size, towers[i].color);
            // We can break early if we already have found all of the towers and drawn them
            if (--tower_count == 0) {
                break;
            }
        }

        // // Iterate again, add borders
        // tower_count = currentTowers;
        // for (int i = 0; i < MAX_TOWERS; i++) {
        //     if (!towers[i].alive) {
        //         continue;
        //     }
        //     DrawRectangleLinesEx(pos_and_size_to_rect(slot_pos_to_world_space_origin(towers[i].slot_pos), Vector2Multiply(towers[i].size, (Vector2) { 2, 2 })), BORDER_THICKNESS, DARKBLUE);
        //     // We can break early if we already have found all of the towers and drawn them
        //     if (--tower_count == 0) {
        //         break;
        //     }
        // }

        // Draw cursor
        // We draw this last after drawing the grid since renderer will already be in line mode
        // Switching between line mode and normal draw mode triggers a flush
        DrawRectangleLinesEx(pos_and_size_to_rect(cursor.position, cursor.size), BORDER_THICKNESS, cursor.color);

        const char *gold_text = TextFormat("GOLD: %d", gold);
        DrawText(gold_text, screenWidth - MeasureText(gold_text, 25) - 10, 10, 25, GRAY);

        if (pause)
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40, GRAY);
#ifdef DEBUG
        else {
            DrawFPS(screenWidth - 90, screenHeight - 25);
        }
#endif
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
