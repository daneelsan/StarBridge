/*******************************************************************************************
*
*   raylib 9years gamejam template
*
*   Template originally created with raylib 4.5-dev, last time updated with raylib 4.5-dev
*
*   Template licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for: 

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

#define SCREEN_WIDTH_PIXELS 256
#define SCREEN_HEIGHT_PIXELS 256

#define MINIMAP_WIDTH_PIXELS 60
#define MINIMAP_HEIGHT_PIXELS 80
#define MINIMAP_BORDER_PIXELS 2
#define MINIMAP_STAR_SPACING_PIXELS 5

#define SPRITESHEET_FROG_OFFSET_X_PIXELS 0
#define SPRITESHEET_FROG_OFFSET_Y_PIXELS 0
#define SPRITESHEET_STAR_OFFSET_X_PIXELS 384
#define SPRITESHEET_STAR_OFFSET_Y_PIXELS 0

#define STAR_COUNT_X 11
#define STAR_COUNT_Y 15
#define STAR_SPACING_PIXELS 64
#define STAR_SPRITE_WIDTH_PIXELS 32
#define STAR_SPRITE_HEIGHT_PIXELS 32
#define STAR_REC_WIDTH_PIXELS 16
#define STAR_REC_HEIGHT_PIXELS 16

#define PLAYER_SPRITE_WIDTH_PIXELS 64
#define PLAYER_SPRITE_HEIGHT_PIXELS 64
#define PLAYER_REC_WIDTH_PIXELS 20
#define PLAYER_REC_HEIGHT_PIXELS 25
#define PLAYER_SPEED 72.0f
#define PLAYER_BOOST 24.0f
#define PLAYER_STUN_COOLDOWN_SECONDS 1.0f
#define PLAYER_JUMP_COOLDOWN_SECONDS 0.5f
#define PLAYER_FLAPPING_DURATION_SECONDS 0.7f

#define CONSTELLATION_MAX_BRIDGES_COUNT 20
#define CONSTELLATION_BRIDGE_LINE_THICKNESS 3.5f

#define GAMESTATE_STAGES_COUNT 3

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

// TODO: Define your custom data types here

enum PlayerSpriteFrames {
    PLAYER_SPRITE_IDLE_WITHOUT_STAR = 0,
    PLAYER_SPRITE_IDLE_WITH_STAR,
    PLAYER_SPRITE_JUMPING_WITHOUT_STAR,
    PLAYER_SPRITE_JUMPING_WITH_STAR,
    PLAYER_SPRITE_STUNNED,
    PLAYER_SPRITE_FRAMES,
};

enum PlayerState {
    PLAYER_IDLE,
    PLAYER_STUNNED,
    PLAYER_JUMPING,
};

struct Player {
    enum PlayerState state;
    Vector2 position;
    Vector2 direction;
    float speed;
    float movementDurationSeconds;
    bool isGrabbingStar;
    int grabbedStarX;
    int grabbedStarY;
    float flappingDurationSeconds;
    bool flappingUp;
    bool isFacingRight;
};

enum StarSpriteFrames {
    STAR_SPRITE_OFF = 0,
    STAR_SPRITE_ON,
    STAR_SPRITE_FRAMES
};

enum BridgeState {
    BRIDGE_OFF_DEFAULT,
    BRIDGE_ON_DEFAULT,
    BRIDGE_ON,
    BRIDGE_DISABLED,
};

struct ConstellationBridge {
    int x1;
    int y1;
    int x2;
    int y2;
    enum BridgeState state;
};

struct Constellation {
    int count;
    int startingScore;
    struct ConstellationBridge bridges[CONSTELLATION_MAX_BRIDGES_COUNT];
};

enum GameStateState {
    GAMESTATE_START,
    GAMESTATE_GAMEPLAY,
    GAMESTATE_CLEAR,
    GAMESTATE_RESULT,
};

struct GameStateStage {
    int constellationId;
    int score;
    float timerSeconds;
};

struct GameState {
    enum GameStateState state;
    float clockSeconds;
    int stageId;
    struct GameStateStage stages[GAMESTATE_STAGES_COUNT];
};

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

static unsigned int screenScale = 1; 
static unsigned int prevScreenScale = 1;

static RenderTexture2D mainRender = { 0 };  // Initialized at init

static RenderTexture2D minimapRender = { 0 };  // Initialized at init

// TODO: Define global variables here, recommended to make them static

// https://lospec.com/palette-list/oil-6
static Color palette[6] = {
    (Color){ 0xfb, 0xf5, 0xef, 0xff },
    (Color){ 0xf2, 0xd3, 0xab, 0xff },
    (Color){ 0xc6, 0x9f, 0xa5, 0xff },
    (Color){ 0x8b, 0x6d, 0x9c, 0xff },
    (Color){ 0x49, 0x4d, 0x7e, 0xff },
    (Color){ 0x27, 0x27, 0x44, 0xff },
};

static bool debugMode = false;

static struct Player player = { 0 };

static Camera2D camera = { 0 };

static Texture2D spritesheet = { 0 };

static Font font = { 0 };

// Constellations
static struct Constellation constellations[] =
    {
        {
            17,
            2,
            {
                { 9, 0, 8, 3, BRIDGE_ON_DEFAULT },
                { 8, 3, 10, 4, BRIDGE_ON_DEFAULT },
                { 10, 4, 9, 9, BRIDGE_OFF_DEFAULT },
                { 9, 9, 7, 10, BRIDGE_OFF_DEFAULT },
                { 7, 10, 3, 10, BRIDGE_OFF_DEFAULT },
                { 3, 10, 1, 10, BRIDGE_OFF_DEFAULT },
                { 1, 10, 0, 13, BRIDGE_OFF_DEFAULT },
                { 3, 10, 4, 14, BRIDGE_OFF_DEFAULT },
                { 4, 14, 0, 13, BRIDGE_OFF_DEFAULT },
                { 1, 10, 2, 8, BRIDGE_OFF_DEFAULT },
                { 2, 8, 4, 6, BRIDGE_OFF_DEFAULT },
                { 4, 6, 6, 4, BRIDGE_OFF_DEFAULT },
                { 6, 4, 6, 2, BRIDGE_OFF_DEFAULT },
                { 6, 2, 8, 3, BRIDGE_OFF_DEFAULT },
                { 1, 4, 4, 6, BRIDGE_OFF_DEFAULT },
                { 0, 0, 1, 4, BRIDGE_OFF_DEFAULT },
                { 1, 4, 6, 2, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            15,
            2,
            {
                { 0, 3, 3, 0, BRIDGE_ON_DEFAULT },
                { 0, 3, 4, 6, BRIDGE_OFF_DEFAULT },
                { 4, 6, 6, 7, BRIDGE_OFF_DEFAULT },
                { 6, 7, 8, 4, BRIDGE_OFF_DEFAULT },
                { 8, 4, 10, 3, BRIDGE_OFF_DEFAULT },
                { 10, 3, 10, 1, BRIDGE_OFF_DEFAULT },
                { 1, 8, 4, 6, BRIDGE_OFF_DEFAULT },
                { 1, 8, 1, 10, BRIDGE_OFF_DEFAULT },
                { 6, 7, 8, 10, BRIDGE_OFF_DEFAULT },
                { 8, 10, 5, 11, BRIDGE_OFF_DEFAULT },
                { 5, 11, 6, 14, BRIDGE_OFF_DEFAULT },
                { 8, 10, 6, 14, BRIDGE_OFF_DEFAULT },
                { 10, 13, 8, 10, BRIDGE_OFF_DEFAULT },
                { 1, 14, 3, 14, BRIDGE_ON_DEFAULT },
                { 3, 14, 6, 14, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            3,
            {
                { 9, 13, 9, 11, BRIDGE_OFF_DEFAULT },
                { 9, 11, 7, 11, BRIDGE_OFF_DEFAULT },
                { 9, 13, 5, 14, BRIDGE_OFF_DEFAULT },
                { 4, 12, 7, 11, BRIDGE_ON_DEFAULT },
                { 4, 12, 5, 14, BRIDGE_OFF_DEFAULT },
                { 7, 11, 7, 8, BRIDGE_OFF_DEFAULT },
                { 4, 12, 3, 9, BRIDGE_OFF_DEFAULT },
                { 0, 11, 3, 9, BRIDGE_OFF_DEFAULT },
                { 6, 6, 7, 8, BRIDGE_OFF_DEFAULT },
                { 6, 6, 7, 4, BRIDGE_OFF_DEFAULT },
                { 7, 4, 9, 3, BRIDGE_OFF_DEFAULT },
                { 4, 5, 6, 6, BRIDGE_OFF_DEFAULT },
                { 2, 4, 4, 5, BRIDGE_OFF_DEFAULT },
                { 4, 2, 7, 4, BRIDGE_OFF_DEFAULT },
                { 1, 2, 2, 4, BRIDGE_ON_DEFAULT },
                { 2, 4, 4, 2, BRIDGE_ON_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            2,
            {
                { 5, 14, 7, 12, BRIDGE_ON_DEFAULT },
                { 7, 12, 10, 13, BRIDGE_OFF_DEFAULT },
                { 10, 13, 10, 10, BRIDGE_OFF_DEFAULT },
                { 8, 6, 10, 10, BRIDGE_OFF_DEFAULT },
                { 8, 6, 10, 5, BRIDGE_OFF_DEFAULT },
                { 7, 1, 10, 5, BRIDGE_ON_DEFAULT },
                { 7, 1, 9, 1, BRIDGE_OFF_DEFAULT },
                { 6, 3, 7, 1, BRIDGE_OFF_DEFAULT },
                { 5, 7, 8, 6, BRIDGE_OFF_DEFAULT },
                { 5, 7, 3, 6, BRIDGE_OFF_DEFAULT },
                { 3, 6, 3, 3, BRIDGE_OFF_DEFAULT },
                { 3, 3, 6, 3, BRIDGE_OFF_DEFAULT },
                { 5, 7, 7, 12, BRIDGE_OFF_DEFAULT },
                { 1, 11, 5, 14, BRIDGE_OFF_DEFAULT },
                { 0, 4, 1, 1, BRIDGE_OFF_DEFAULT },
                { 1, 1, 3, 3, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            3,
            {
                { 10, 14, 10, 12, BRIDGE_OFF_DEFAULT },
                { 7, 12, 10, 14, BRIDGE_ON_DEFAULT },
                { 7, 12, 7, 10, BRIDGE_OFF_DEFAULT },
                { 10, 12, 9, 10, BRIDGE_OFF_DEFAULT },
                { 9, 10, 7, 10, BRIDGE_OFF_DEFAULT },
                { 7, 10, 8, 8, BRIDGE_OFF_DEFAULT },
                { 8, 8, 9, 6, BRIDGE_OFF_DEFAULT },
                { 7, 1, 9, 6, BRIDGE_OFF_DEFAULT },
                { 7, 1, 9, 0, BRIDGE_ON_DEFAULT },
                { 1, 11, 0, 9, BRIDGE_ON_DEFAULT },
                { 7, 1, 3, 0, BRIDGE_OFF_DEFAULT },
                { 3, 0, 1, 1, BRIDGE_OFF_DEFAULT },
                { 3, 6, 3, 4, BRIDGE_OFF_DEFAULT },
                { 3, 4, 1, 1, BRIDGE_OFF_DEFAULT },
                { 3, 6, 0, 9, BRIDGE_OFF_DEFAULT },
                { 3, 6, 8, 8, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            2,
            {
                { 7, 1, 10, 1, BRIDGE_ON_DEFAULT },
                { 10, 1, 10, 4, BRIDGE_OFF_DEFAULT },
                { 9, 7, 10, 4, BRIDGE_OFF_DEFAULT },
                { 7, 10, 9, 7, BRIDGE_OFF_DEFAULT },
                { 6, 12, 7, 10, BRIDGE_OFF_DEFAULT },
                { 6, 12, 10, 14, BRIDGE_OFF_DEFAULT },
                { 2, 12, 6, 12, BRIDGE_OFF_DEFAULT },
                { 0, 11, 2, 12, BRIDGE_OFF_DEFAULT },
                { 7, 10, 5, 9, BRIDGE_OFF_DEFAULT },
                { 0, 11, 5, 9, BRIDGE_ON_DEFAULT },
                { 5, 9, 3, 7, BRIDGE_OFF_DEFAULT },
                { 3, 7, 3, 5, BRIDGE_OFF_DEFAULT },
                { 3, 5, 2, 3, BRIDGE_OFF_DEFAULT },
                { 2, 3, 2, 1, BRIDGE_OFF_DEFAULT },
                { 7, 1, 7, 3, BRIDGE_OFF_DEFAULT },
                { 7, 3, 2, 1, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            2,
            {
                { 0, 13, 2, 11, BRIDGE_OFF_DEFAULT },
                { 2, 11, 2, 9, BRIDGE_OFF_DEFAULT },
                { 2, 11, 5, 12, BRIDGE_ON_DEFAULT },
                { 5, 12, 7, 13, BRIDGE_OFF_DEFAULT },
                { 7, 13, 10, 12, BRIDGE_OFF_DEFAULT },
                { 5, 12, 6, 9, BRIDGE_OFF_DEFAULT },
                { 6, 9, 10, 12, BRIDGE_OFF_DEFAULT },
                { 6, 9, 8, 7, BRIDGE_OFF_DEFAULT },
                { 8, 7, 9, 5, BRIDGE_OFF_DEFAULT },
                { 9, 5, 8, 2, BRIDGE_OFF_DEFAULT },
                { 8, 2, 7, 0, BRIDGE_OFF_DEFAULT },
                { 8, 7, 4, 5, BRIDGE_ON_DEFAULT },
                { 4, 5, 7, 0, BRIDGE_OFF_DEFAULT },
                { 4, 5, 3, 3, BRIDGE_OFF_DEFAULT },
                { 3, 3, 1, 4, BRIDGE_OFF_DEFAULT },
                { 1, 4, 0, 6, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        },
        {
            16,
            3,
            {
                { 9, 0, 10, 2, BRIDGE_OFF_DEFAULT },
                { 10, 2, 8, 2, BRIDGE_ON_DEFAULT },
                { 6, 3, 8, 2, BRIDGE_ON_DEFAULT },
                { 6, 3, 3, 2, BRIDGE_ON_DEFAULT },
                { 3, 2, 3, 0, BRIDGE_OFF_DEFAULT },
                { 6, 3, 5, 5, BRIDGE_OFF_DEFAULT },
                { 5, 5, 8, 6, BRIDGE_OFF_DEFAULT },
                { 8, 6, 10, 2, BRIDGE_OFF_DEFAULT },
                { 9, 10, 8, 6, BRIDGE_OFF_DEFAULT },
                { 5, 5, 5, 9, BRIDGE_OFF_DEFAULT },
                { 5, 9, 2, 9, BRIDGE_OFF_DEFAULT },
                { 2, 9, 2, 11, BRIDGE_OFF_DEFAULT },
                { 2, 11, 2, 13, BRIDGE_OFF_DEFAULT },
                { 2, 13, 4, 13, BRIDGE_OFF_DEFAULT },
                { 4, 13, 6, 13, BRIDGE_OFF_DEFAULT },
                { 6, 13, 9, 10, BRIDGE_OFF_DEFAULT },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED },
                { -1, -1, -1, -1, BRIDGE_DISABLED }
            }
        }
    };

static int numberOfConstellations = sizeof(constellations)/sizeof(struct Constellation);

static struct GameState gameState = { 0 };

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame

static void ResetPlayer(struct Player *player);
static void ResetCamera(Camera2D *camera, struct Player *player);
static void ResetConstellations(void);
static void ResetGameState(struct GameState *gameState);
static int GetRandomNewConstellationId(struct GameState *gameState);
static void UpdatePlayer(struct Player *player, float delta);
static void UpdateCameraCenterSmoothFollow(Camera2D *camera, struct Player *player, float delta);
static void InteractPlayerAndStars(struct GameState *gameState, struct Player *player, int constellationId);
static int GetConstellationRequiredScore(int constellationId);
static Vector2 GetStarPosition(int x, int y);
static Rectangle GetStarRec(Vector2 position);
static Rectangle GetPlayerRec(Vector2 position);
static int GetConstellationBridgeIndex(int constellationId, int x1, int y1, int x2, int y2);
static void DrawSprite(int spriteOffsetX, int spriteOffsetY, int spriteWidth, int spriteHeight, int frameNumber, Vector2 position);
static void DrawStar(int x, int y, int frameNumber);
static void DrawDebugGrid(int spacingPixels);
static void DrawStars(void);
static void DrawBridges(int constellationId);
static void DrawPlayer(struct Player *player, float deltaTime);
static Vector2 GetMinimapStarPosition(int x, int y);
static void DrawMinimapFrame(void);
static void DrawMinimapConstellation(int constellationId);
static void DrawStagePanel(struct GameState *gameState);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messsages
#endif

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS, "raylib 9yr gamejam");
    
    // TODO: Load resources / Initialize variables at this point

    // Load spritesheet
    spritesheet = LoadTexture("resources/spritesheet2.png");

    font = LoadFont("resources/Autriche-4n84.ttf");

    ResetPlayer(&player);

    ResetCamera(&camera, &player);

    ResetConstellations();

    ResetGameState(&gameState);

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    mainRender = LoadRenderTexture(SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS);
    SetTextureFilter(mainRender.texture, TEXTURE_FILTER_BILINEAR);

    minimapRender = LoadRenderTexture(MINIMAP_WIDTH_PIXELS, MINIMAP_HEIGHT_PIXELS);
    SetTextureFilter(minimapRender.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);     // Set our game frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(minimapRender);

    UnloadRenderTexture(mainRender);

    UnloadFont(font);

    UnloadTexture(spritesheet);
    
    // TODO: Unload all loaded resources at this point

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
// Update and draw frame
void UpdateDrawFrame(void)
{
    float deltaTime = GetFrameTime();

    // Update
    //----------------------------------------------------------------------------------

    // Screen scale logic (x2)
    if (IsKeyPressed(KEY_ONE)) screenScale = 1;
    else if (IsKeyPressed(KEY_TWO)) screenScale = 2;
    else if (IsKeyPressed(KEY_THREE)) screenScale = 3;
    
    if (screenScale != prevScreenScale)
    {
        // Scale window to fit the scaled render texture
        SetWindowSize(SCREEN_WIDTH_PIXELS*screenScale, SCREEN_HEIGHT_PIXELS*screenScale);
        
        // Scale mouse proportionally to keep input logic inside the 256x256 screen limits
        SetMouseScale(1.0f/(float)screenScale, 1.0f/(float)screenScale);
        
        prevScreenScale = screenScale;
    }

    // TODO: Update variables / Implement example logic at this point
    //----------------------------------------------------------------------------------

    if (IsKeyPressed(KEY_F3))
    {
        if (debugMode)
        {
            LOG("Debug mode is ON\n");
        } else
        {
            LOG("Debug mode is OFF\n");
        }
        debugMode = !debugMode;
    }

    gameState.clockSeconds += deltaTime;
    switch (gameState.state)
    {
        case GAMESTATE_START:
        {
            UpdatePlayer(&player, deltaTime);
            UpdateCameraCenterSmoothFollow(&camera, &player, deltaTime);

            if (gameState.clockSeconds >= 5.0f)
            {
                gameState.clockSeconds = 0;
                // TODO: make this random
                gameState.stages[gameState.stageId].constellationId = GetRandomNewConstellationId(&gameState);
                gameState.state = GAMESTATE_GAMEPLAY;
            }
        } break;
        case GAMESTATE_GAMEPLAY:
        {
            struct GameStateStage *stage = &gameState.stages[gameState.stageId];

            stage->timerSeconds += deltaTime;

            UpdatePlayer(&player, deltaTime);
            UpdateCameraCenterSmoothFollow(&camera, &player, deltaTime);
            InteractPlayerAndStars(&gameState, &player, stage->constellationId);

            if (stage->score == GetConstellationRequiredScore(stage->constellationId))
            {
                gameState.clockSeconds = 0;
                gameState.state = GAMESTATE_CLEAR;
            }
        } break;
        case GAMESTATE_CLEAR:
        {
            UpdatePlayer(&player, deltaTime);
            UpdateCameraCenterSmoothFollow(&camera, &player, deltaTime);

            if (gameState.clockSeconds <= 5.0f)
            {
                break;
            }

            if (gameState.stageId == GAMESTATE_STAGES_COUNT - 1)
            {
                gameState.clockSeconds = 0;
                gameState.state = GAMESTATE_RESULT;
            } else
            {
                gameState.clockSeconds = 0;
                gameState.stageId += 1;
                gameState.state = GAMESTATE_START;
            }
        } break;
        case GAMESTATE_RESULT:
        {
            if (gameState.clockSeconds >= 2.0f)
            {
                if (IsKeyPressed(KEY_R))
                {
                    ResetPlayer(&player);
                    ResetCamera(&camera, &player);
                    ResetConstellations();
                    ResetGameState(&gameState);
                }
            }
        } break;
    }

    // Draw
    //----------------------------------------------------------------------------------
    // Render all screen to texture (for scaling)
    BeginTextureMode(mainRender);
        ClearBackground(palette[0]);
        
        // TODO: Draw screen at 256x256
        DrawRectangle(0, 0, SCREEN_WIDTH_PIXELS, SCREEN_HEIGHT_PIXELS, palette[5]);

        BeginMode2D(camera);

            if (debugMode)
            {
                DrawDebugGrid(STAR_SPACING_PIXELS);
            }

            switch (gameState.state)
            {
                case GAMESTATE_START:
                {
                    DrawStars();
                    DrawPlayer(&player, deltaTime);
                } break;
                case GAMESTATE_GAMEPLAY:
                {
                    DrawStars();
                    DrawBridges(gameState.stages[gameState.stageId].constellationId);
                    DrawPlayer(&player, deltaTime);
                } break;
                case GAMESTATE_CLEAR:
                {
                    DrawStars();
                    DrawBridges(gameState.stages[gameState.stageId].constellationId);
                    DrawPlayer(&player, deltaTime);
                } break;
                case GAMESTATE_RESULT:
                {
                } break;
            }

        EndMode2D();

        switch (gameState.state)
        {
            case GAMESTATE_START:
            {
                if ((gameState.clockSeconds >= 1.0f) && (gameState.clockSeconds <= 4.0f))
                {
                    const int seconds = (int)gameState.clockSeconds;
                    const Vector2 textPos = (Vector2){ 100, 160};
                    DrawTextEx(font, TextFormat("%i", 4 - seconds), textPos, 60, 1.0f, palette[0]);
                }

                DrawStagePanel(&gameState);
            } break;
            case GAMESTATE_GAMEPLAY:
            {
                if ((gameState.clockSeconds >= 0.0f) && (gameState.clockSeconds <= 1.5f))
                {
                    const Vector2 textPos = (Vector2){ 40, 170};
                    DrawTextEx(font, "START", textPos, 40, 1.0f, palette[0]);
                }
                DrawStagePanel(&gameState);
            } break;
            case GAMESTATE_CLEAR:
            {
                if ((gameState.clockSeconds >= 1.0f) && (gameState.clockSeconds <= 5.0f))
                {
                    const Vector2 textPos = (Vector2){ 40, 170};
                    DrawTextEx(font, "CLEAR", textPos, 40, 1.0f, palette[0]);
                }
                DrawStagePanel(&gameState);
            } break;
            case GAMESTATE_RESULT:
            {
                if (gameState.clockSeconds >= 1.0f)
                {
                    // TODO: Rename variables
                    const int stageId = (int)(gameState.clockSeconds - 1.0f);
                    const int max = (stageId < GAMESTATE_STAGES_COUNT - 1) ? stageId : GAMESTATE_STAGES_COUNT - 1;
                    for (int i = 0; i <= max; i += 1)
                    {
                        int seconds = (int)(gameState.stages[i].timerSeconds);
                        int minutes = seconds/60;
                        seconds -= minutes*60;
                        // TODO: Share the constellation id
                        DrawTextEx(font,
                                TextFormat("STAGE %i: %02i:%02i", i + 1, minutes, seconds),
                                (Vector2){ 20, 40 + 30*i},
                                20,
                                1.0f,
                                palette[0]);
                    }
                }

                if (gameState.clockSeconds >= 5.0f)
                {
                    if (gameState.clockSeconds - (int)(gameState.clockSeconds) >= 0.5)
                    {
                        DrawTextEx(font, "  PRESS (R)  ", (Vector2){ 40, 210}, 20, 1.0f, palette[0]);
                        DrawTextEx(font, "TO PLAY AGAIN", (Vector2){ 15, 230}, 20, 1.0f, palette[0]);
                    }
                }
            } break;
        }

    EndTextureMode();

    BeginTextureMode(minimapRender);

        switch (gameState.state)
        {
            case GAMESTATE_START:
            {
                DrawMinimapFrame();
            } break;
            case GAMESTATE_GAMEPLAY:
            {
                DrawMinimapFrame();
                DrawMinimapConstellation(gameState.stages[gameState.stageId].constellationId);
            } break;
            case GAMESTATE_CLEAR:
            {
                DrawMinimapFrame();
                DrawMinimapConstellation(gameState.stages[gameState.stageId].constellationId);
            } break;
            case GAMESTATE_RESULT:
            {
            } break;
        }

    EndTextureMode();
    
    BeginDrawing();
        ClearBackground(palette[0]);
        
        // Draw render texture to screen scaled as required
        DrawTexturePro(mainRender.texture,
                       (Rectangle){ 0, 0, (float)mainRender.texture.width, -(float)mainRender.texture.height },
                       (Rectangle){ 0, 0, (float)mainRender.texture.width*screenScale, (float)mainRender.texture.height*screenScale },
                       (Vector2){ 0, 0 },
                       0.0f, 
                       WHITE);

        if (gameState.state != GAMESTATE_RESULT)
        {
            DrawTexturePro(minimapRender.texture,
                           (Rectangle){ 0, 0, (float)minimapRender.texture.width, -(float)minimapRender.texture.height },
                           (Rectangle){ (SCREEN_WIDTH_PIXELS - MINIMAP_WIDTH_PIXELS - 5)*screenScale, 5*screenScale, (float)minimapRender.texture.width*screenScale, (float)minimapRender.texture.height*screenScale },
                           (Vector2){ 0, 0 },
                           0.0f, 
                           WHITE);
        }

        // Draw equivalent mouse position on the main render-texture
        DrawCircleLines(GetMouseX()*screenScale, GetMouseY()*screenScale, 10, MAROON);

        // TODO: Draw everything that requires to be drawn at this point:
        if (debugMode)
        {
            DrawFPS(0, 0);
        }
    EndDrawing();
    //----------------------------------------------------------------------------------  
}

void ResetPlayer(struct Player *player)
{
    player->state = PLAYER_IDLE;
    player->position = (Vector2){ SCREEN_WIDTH_PIXELS/2.0f, SCREEN_HEIGHT_PIXELS/2.0f };
    player->direction = (Vector2){ 0.0f, 0.0f };
    player->speed = PLAYER_SPEED;
    player->movementDurationSeconds = 0.0f;
    player->isGrabbingStar = false;
    player->grabbedStarX = -1;
    player->grabbedStarY = -1;
    player->flappingDurationSeconds = 0.0f;
    player->flappingUp = true;
    player->isFacingRight = false;
}

void ResetCamera(Camera2D *camera, struct Player *player)
{
    camera->target = player->position;
    camera->offset = (Vector2){ SCREEN_WIDTH_PIXELS/2.0f, SCREEN_HEIGHT_PIXELS/2.0f };
    camera->rotation = 0.0f;
    camera->zoom = 1.0f;
}

void ResetConstellations(void)
{
    for (int i = 0; i < numberOfConstellations; i += 1)
    {
        struct Constellation *constellation = &constellations[i];
        for (int j = 0; j < constellation->count; j += 1)
        {
            if (constellation->bridges[j].state == BRIDGE_ON)
            {
                constellation->bridges[j].state = BRIDGE_OFF_DEFAULT;
            }
        }
    }
}

void ResetGameState(struct GameState *gameState)
{
    gameState->state = GAMESTATE_START;
    gameState->clockSeconds = 0.0f;
    for (int i = 0; i < GAMESTATE_STAGES_COUNT; i += 1)
    {
        gameState->stages[i] = (struct GameStateStage){ -1, 0, 0.0f };
    }
    gameState->stageId = 0;
}

int GetRandomNewConstellationId(struct GameState *gameState)
{
    int randId = -1;
    while (true)
    {
        randId = GetRandomValue(0, numberOfConstellations - 1);
        // TODO: Don't hardcode these values
        if (
            (randId != gameState->stages[0].constellationId) &&
            (randId != gameState->stages[1].constellationId) &&
            (randId != gameState->stages[2].constellationId))
        {
            break;
        }
    }
    if (debugMode)
    {
        LOG("RANDOM CONSTELLATION: %i\n", randId);
    }
    return randId;
}

void MovePlayer(struct Player *player, float deltaTime)
{
    player->position.x += player->direction.x*player->speed*deltaTime;
    player->position.y += player->direction.y*player->speed*deltaTime;

    // The positions of the first and the last stars
    // are used to determine the "walls" of the screen.
    const Vector2 firstStar = GetStarPosition(0, 0);
    const Vector2 lastStar = GetStarPosition(STAR_COUNT_X - 1, STAR_COUNT_Y - 1);

    if (player->position.x < firstStar.x)
    {
        player->position.x = firstStar.x;
    } else if (player->position.x > lastStar.x)
    {
        player->position.x = lastStar.x;
    }

    if (player->position.y < firstStar.y)
    {
        player->position.y = firstStar.y;
    } else if (player->position.y > lastStar.y)
    {
        player->position.y = lastStar.y;
    }
}

void UpdatePlayer(struct Player *player, float deltaTime)
{
    player->movementDurationSeconds += deltaTime;

    switch (player->state)
    {
        case PLAYER_STUNNED:
        {
            if (player->movementDurationSeconds >= PLAYER_STUN_COOLDOWN_SECONDS)
            {
                player->movementDurationSeconds = 0.0f;
                player->state = PLAYER_IDLE;
            }
        } break;
        case PLAYER_IDLE:
        {
            player->direction.x = 0;
            player->direction.y = 0;

            if (IsKeyDown(KEY_LEFT)) player->direction.x -= 1;
            if (IsKeyDown(KEY_RIGHT)) player->direction.x += 1;
            if (IsKeyDown(KEY_UP)) player->direction.y -= 1;
            if (IsKeyDown(KEY_DOWN)) player->direction.y += 1;

            if (player->direction.x < 0)
            {
                player->isFacingRight = false;
            } else if (player->direction.x > 0)
            {
                player->isFacingRight = true;
            }

            if (IsKeyDown(KEY_LEFT_ALT))
            {
                player->speed += PLAYER_BOOST;
                player->movementDurationSeconds = 0.0f;
                player->state = PLAYER_JUMPING;
            }

            MovePlayer(player, deltaTime);
        } break;
        case PLAYER_JUMPING:
        {
            // Note that the direction cannot change while jumping.
            // This is a way to nerf the jump mechanic.

            if (player->movementDurationSeconds >= PLAYER_JUMP_COOLDOWN_SECONDS)
            {
                player->speed -= PLAYER_BOOST;
                player->movementDurationSeconds = 0.0f;
                player->state = PLAYER_IDLE;
            }

            MovePlayer(player, deltaTime);
        } break;
    }
}

// TODO: Study this...
void UpdateCameraCenterSmoothFollow(Camera2D *camera, struct Player *player, float delta)
{
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){ SCREEN_WIDTH_PIXELS/2.0f, SCREEN_HEIGHT_PIXELS/2.0f };
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed*length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length));
    }
}

void InteractPlayerAndStars(struct GameState *gameState, struct Player *player, int constellationId)
{
    if ((player->state == PLAYER_STUNNED) || !IsKeyPressed(KEY_SPACE))
    {
           return;
    }

    const Rectangle playerRec = GetPlayerRec(player->position);

    int closestStarX = lrintf(player->position.x/(float)STAR_SPACING_PIXELS);
    int closestStarY = lrintf(player->position.y/(float)STAR_SPACING_PIXELS);

    const Vector2 closestStarPos = GetStarPosition(closestStarX, closestStarY);
    const Rectangle closestStarRec = GetStarRec(closestStarPos);

    if (!CheckCollisionRecs(playerRec, closestStarRec))
    {
        return;
    }

    if (player->isGrabbingStar)
    {
        if ((player->grabbedStarX == closestStarX) && (player->grabbedStarY == closestStarY))
        {
            // Do not allow to "drop" a grabbed star onto itself
            return;
        }

        const int constellationBridgeId = GetConstellationBridgeIndex(constellationId,
                                                                      player->grabbedStarX, player->grabbedStarY,
                                                                      closestStarX, closestStarY);
        if ((constellationBridgeId == -1) || (constellations[constellationId].bridges[constellationBridgeId].state) != BRIDGE_OFF_DEFAULT)
        {
            player->movementDurationSeconds = 0.0f;
            player->state = PLAYER_STUNNED;
        } else
        {
            constellations[constellationId].bridges[constellationBridgeId].state = BRIDGE_ON;
            gameState->stages[gameState->stageId].score += 1;
        }
        player->grabbedStarX = -1;
        player->grabbedStarY = -1;
        player->isGrabbingStar = false;
    } else
    {
        player->grabbedStarX = closestStarX;
        player->grabbedStarY = closestStarY;
        player->isGrabbingStar = true;
    }
}

int GetConstellationRequiredScore(int constellationId)
{
    struct Constellation *constellation = &constellations[constellationId];
    int requiredScore = constellation->count - constellation->startingScore;
    return requiredScore;
}

Vector2 GetStarPosition(int x, int y)
{
    Vector2 position = { x*STAR_SPACING_PIXELS, y*STAR_SPACING_PIXELS };
    return position;
}

Rectangle GetStarRec(Vector2 position)
{
    Rectangle starRec = { 0 };
    starRec.x = position.x - (float)STAR_REC_WIDTH_PIXELS/2.0f;
    starRec.y = position.y - (float)STAR_REC_HEIGHT_PIXELS/2.0f;
    starRec.width = (float)STAR_REC_WIDTH_PIXELS;
    starRec.height = (float)STAR_REC_HEIGHT_PIXELS;
    return starRec;
}

Rectangle GetPlayerRec(Vector2 position)
{
    Rectangle playerRec = { 0 };
    playerRec.x = position.x - (float)PLAYER_REC_WIDTH_PIXELS/2.0f;
    playerRec.y = position.y - (float)PLAYER_REC_HEIGHT_PIXELS/2.0f;
    playerRec.width = (float)PLAYER_REC_WIDTH_PIXELS;
    playerRec.height = (float)PLAYER_REC_HEIGHT_PIXELS;
    return playerRec;
}

int GetConstellationBridgeIndex(int constellationId, int x1, int y1, int x2, int y2)
{
    struct Constellation *constellation = &constellations[constellationId];
    for (int i = 0; i < constellation->count; i += 1)
    {
        const struct ConstellationBridge bridge = constellation->bridges[i];
        if (
            ((x1 == bridge.x1) && (y1 == bridge.y1) && (x2 == bridge.x2) && (y2 == bridge.y2))
            ||
            ((x2 == bridge.x1) && (y2 == bridge.y1) && (x1 == bridge.x2) && (y1 == bridge.y2))
        )
        {
            return i;
        }
    }
    return -1;
}

void DrawSprite(int spriteOffsetX, int spriteOffsetY, int spriteWidth, int spriteHeight, int frameNumber, Vector2 position)
{
    Rectangle source = { spriteOffsetX, spriteOffsetY, spriteWidth, spriteHeight };
    Rectangle dest = { position.x, position.y, spriteWidth, spriteHeight };

    source.x += frameNumber*spriteWidth;

    DrawTexturePro(spritesheet, source, dest, (Vector2){ spriteWidth/2.0f, spriteHeight/2.0f }, 0.0f, WHITE);
}

void DrawStar(int x, int y, int frameNumber)
{
    const Vector2 position = GetStarPosition(x, y);

    DrawSprite(SPRITESHEET_STAR_OFFSET_X_PIXELS, SPRITESHEET_STAR_OFFSET_Y_PIXELS,
               STAR_SPRITE_WIDTH_PIXELS, STAR_SPRITE_HEIGHT_PIXELS,
               frameNumber,
               position);

    if (debugMode)
    {
        const Rectangle rec = GetStarRec(position);
        DrawRectangleLinesEx(rec, 1.0f, RED);
    }
}

void DrawDebugGrid(int spacingPixels)
{
    rlPushMatrix();
    rlTranslatef(0, 25*spacingPixels, 0);
    rlRotatef(90, 1, 0, 0);
    DrawGrid(100, spacingPixels);
    rlPopMatrix();
}

void DrawStars(void)
{
    for (int y = 0; y < STAR_COUNT_Y; y += 1)
    {
        for (int x = 0; x < STAR_COUNT_X; x += 1)
        {
            DrawStar(x, y, STAR_SPRITE_OFF);
        }
    }
}

void DrawBridges(int constellationId)
{
    struct Constellation *constellation = &constellations[constellationId];
    for (int i = 0; i < constellation->count; i += 1)
    {
        const struct ConstellationBridge bridge = constellation->bridges[i];
        if ((bridge.state == BRIDGE_ON) || (bridge.state == BRIDGE_ON_DEFAULT))
        {
            const Vector2 star1Pos = GetStarPosition(bridge.x1, bridge.y1);
            const Vector2 star2Pos = GetStarPosition(bridge.x2, bridge.y2);

            // TODO: Bridge sprite?
            DrawLineEx(star1Pos, star2Pos, CONSTELLATION_BRIDGE_LINE_THICKNESS, palette[0]);

            if (debugMode)
            {
                DrawLineEx(star1Pos, star2Pos, 1.0f, RED);
            }
        }
    }

    for (int i = 0; i < constellation->count; i += 1)
    {
        const struct ConstellationBridge bridge = constellation->bridges[i];
        if ((bridge.state == BRIDGE_ON) || (bridge.state == BRIDGE_ON_DEFAULT))
        {
            // NOTE: Stars might be drawn on top of themselves more than once
            // TODO: Does this cause significant performance issues?
            DrawStar(bridge.x1, bridge.y1, STAR_SPRITE_ON);
            DrawStar(bridge.x2, bridge.y2, STAR_SPRITE_ON);
        }
    }
}

void DrawPlayer(struct Player *player, float deltaTime)
{
    if (player->isGrabbingStar)
    {
        // Draw dragged bridge
        const Vector2 grabbedStarPos = GetStarPosition(player->grabbedStarX, player->grabbedStarY);
        DrawLineEx(grabbedStarPos,
                   Vector2Add(player->position, (Vector2){ 0, 10 }),
                   CONSTELLATION_BRIDGE_LINE_THICKNESS,
                   palette[0]);

        if (debugMode)
        {
            DrawLineEx(grabbedStarPos, player->position, 1.0f, PURPLE);
        }

        // Draw star at beginning of path
        DrawStar(player->grabbedStarX, player->grabbedStarY, 1);
    }

    player->flappingDurationSeconds += deltaTime;
    if (player->flappingDurationSeconds >= PLAYER_FLAPPING_DURATION_SECONDS)
    {
        player->flappingDurationSeconds = 0.0f;
        player->flappingUp = !player->flappingUp;
    }

    int spriteOffsetX = SPRITESHEET_FROG_OFFSET_X_PIXELS;
    switch (player->state)
    {
        case PLAYER_IDLE:
        {
            if (player->isGrabbingStar)
            {
                spriteOffsetX += 2*PLAYER_SPRITE_WIDTH_PIXELS;
            } else
            {
                spriteOffsetX += 0;
            }
        } break;
        case PLAYER_JUMPING:
        {
            if (player->isGrabbingStar)
            {
                spriteOffsetX += 2*PLAYER_SPRITE_WIDTH_PIXELS;
            } else
            {
                spriteOffsetX += 0;
            }
        } break;
        case PLAYER_STUNNED:
        {
            spriteOffsetX += 4*PLAYER_SPRITE_WIDTH_PIXELS;
        } break;
    }

    if (player->flappingUp)
    {
        spriteOffsetX += PLAYER_SPRITE_WIDTH_PIXELS;
    }

    int spriteOffsetY = SPRITESHEET_FROG_OFFSET_Y_PIXELS;
    if (player->isFacingRight)
    {
        spriteOffsetY += PLAYER_SPRITE_HEIGHT_PIXELS;
    }

    DrawSprite(spriteOffsetX, spriteOffsetY,
               PLAYER_SPRITE_WIDTH_PIXELS, PLAYER_SPRITE_HEIGHT_PIXELS,
               0,
               player->position);

    if (debugMode)
    {
        Rectangle playerRec = GetPlayerRec(player->position);
        // TODO:
        Color color;
        switch (player->state)
        {
            case PLAYER_IDLE:
            {
                color = (player->isGrabbingStar) ? YELLOW : GREEN;
            } break;
            case PLAYER_JUMPING:
            {
                color = (player->isGrabbingStar) ? YELLOW : BLUE;
            } break;
            case PLAYER_STUNNED:
            {
                color = ORANGE;
            } break;
        }
        DrawRectangleLinesEx(playerRec, 1.0f, color);
    }
}

void DrawMinimapStar(int x, int y)
{
    int posX = x*MINIMAP_STAR_SPACING_PIXELS + MINIMAP_STAR_SPACING_PIXELS;
    int posY = y*MINIMAP_STAR_SPACING_PIXELS + MINIMAP_STAR_SPACING_PIXELS;
    DrawCircle(posX, posY, 1.0f, palette[1]);
}

Vector2 GetMinimapStarPosition(int x, int y)
{
    Vector2 pos = { 0 };
    pos.x = (x + 1)*MINIMAP_STAR_SPACING_PIXELS;
    pos.y = (y + 1)*MINIMAP_STAR_SPACING_PIXELS;
    return pos;
}

void DrawMinimapFrame(void)
{
    ClearBackground(palette[0]);

    if (debugMode)
    {
        DrawDebugGrid(5);
    }

    DrawRectangle(MINIMAP_BORDER_PIXELS,
                  MINIMAP_BORDER_PIXELS,
                  MINIMAP_WIDTH_PIXELS - 2*MINIMAP_BORDER_PIXELS,
                  MINIMAP_HEIGHT_PIXELS - 2*MINIMAP_BORDER_PIXELS,
                  palette[4]);
}

void DrawMinimapConstellation(int constellationId)
{
    struct Constellation *constellation = &constellations[constellationId];
    for (int i = 0; i < constellation->count; i += 1)
    {
        struct ConstellationBridge bridge = constellation->bridges[i];
        if (bridge.state == BRIDGE_DISABLED)
        {
            continue;
        }

        Vector2 star1Pos = GetMinimapStarPosition(bridge.x1, bridge.y1);
        Vector2 star2Pos = GetMinimapStarPosition(bridge.x2, bridge.y2);

        DrawLineEx(star1Pos, star2Pos, 1.0f, ((bridge.state == BRIDGE_ON) || (bridge.state == BRIDGE_ON_DEFAULT)) ? palette[1] : palette[3]);

        DrawCircleV(star1Pos, 1.0f, palette[1]);
        DrawCircleV(star2Pos, 1.0f, palette[1]);
    }
}

void DrawStagePanel(struct GameState *gameState)
{
    const int segments = 60;
    const float roundness = 0.5f;

    const int fontSize = 10;
    const int fontPosY = SCREEN_HEIGHT_PIXELS - 16;

    const int recPosY = SCREEN_HEIGHT_PIXELS - 20;
    const int recHeight = 16;

    Rectangle rec = { 0 };
    Vector2 textPos = { 0 };

    struct GameStateStage *stage = &gameState->stages[gameState->stageId];

    // Clock panel
    rec = (Rectangle){ 4, recPosY, 76, recHeight };
    DrawRectangleRounded(rec, roundness, segments, palette[3]);

    int seconds = (gameState->state == GAMESTATE_GAMEPLAY) ? (int)(gameState->clockSeconds) : 0.0f;
    int minutes = seconds/60;
    seconds -= minutes*60;

    textPos = (Vector2) { 7, fontPosY };
    DrawTextEx(font, TextFormat("TIME: %02i:%02i", minutes, seconds), textPos, fontSize - 2, roundness, palette[0]);

    // Stage panel
    rec = (Rectangle){ SCREEN_WIDTH_PIXELS/2 - 42, recPosY, 77, recHeight };
    DrawRectangleRounded(rec, roundness, segments, palette[3]);

    textPos = (Vector2) { SCREEN_WIDTH_PIXELS/2 - 36, fontPosY };
    DrawTextEx(font, TextFormat("STAGE: %i", gameState->stageId + 1), textPos, fontSize, 1.0f, palette[0]);

    // Score panel
    rec = (Rectangle){ SCREEN_WIDTH_PIXELS - 87, recPosY, 86, recHeight };
    DrawRectangleRounded(rec, roundness, segments, palette[3]);

    textPos = (Vector2) { SCREEN_WIDTH_PIXELS - 85, fontPosY };
    if (stage->constellationId == -1)
    {
        DrawTextEx(font, TextFormat("SCORE: %02i-??", 0), textPos, fontSize - 2, 1.0f, palette[0]);
    } else
    {
        DrawTextEx(font, TextFormat("SCORE: %02i-%02i", stage->score, GetConstellationRequiredScore(stage->constellationId)), textPos, fontSize - 2, 1.0f, palette[0]);
    }
}
