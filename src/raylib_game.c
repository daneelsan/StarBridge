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

#define STAR_COUNT_X 11
#define STAR_COUNT_Y 15
#define STAR_SPACING_PIXELS 64
#define STAR_REC_WIDTH_PIXELS 16
#define STAR_REC_HEIGHT_PIXELS 16

#define PLAYER_REC_WIDTH_PIXELS 15
#define PLAYER_REC_HEIGHT_PIXELS 20
#define PLAYER_SPEED 64.0f
#define PLAYER_BOOST 32.0f
#define PLAYER_STUN_COOLDOWN_SECONDS 1.0f
#define PLAYER_JUMP_COOLDOWN_SECONDS 0.5f

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
    float clockSeconds;
};

struct Star {
    bool isLighted;
};

struct ConstellationElement {
    int x1;
    int y1;
    int x2;
    int y2;
    bool isLighted;
};

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screenWidth = 256;
static const int screenHeight = 256;

static unsigned int screenScale = 1; 
static unsigned int prevScreenScale = 1;

static RenderTexture2D target = { 0 };  // Initialized at init

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

static struct Star stars[STAR_COUNT_Y][STAR_COUNT_X] = { 0 };

static struct Player player = { 0 };

static Camera2D camera = { 0 };

static Texture2D spritesheet = { 0 };

// Constellations
static struct ConstellationElement constellation1[] =
    {
        { 9, 0, 8, 3, true },
        { 8, 3, 10, 4, true },
        { 10, 4, 9, 9, false },
        { 9, 9, 7, 10, false },
        { 7, 10, 3, 10, false },
        { 3, 10, 1, 10, false },
        { 1, 10, 0, 13, false },
        { 3, 10, 4, 14, false },
        { 4, 14, 0, 13, false },
        { 1, 10, 2, 8, false },
        { 2, 8, 4, 6, false },
        { 4, 6, 6, 4, false },
        { 6, 4, 6, 2, false },
        { 6, 2, 8, 3, false },
        { 1, 4, 4, 6, false },
        { 0, 0, 1, 4, false },
        { 1, 4, 6, 2, false }
    };

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame

static void ResetPlayer(struct Player *player);
static void ResetCamera(Camera2D *camera, struct Player *player);
static void ResetStars();
static void UpdatePlayer(struct Player *player, float delta);
static void UpdateCameraCenterSmoothFollow(Camera2D *camera, struct Player *player, float delta);
static void InteractPlayerAndStars(struct Player *player);
static Vector2 GetStarPosition(int x, int y);
static Rectangle GetStarRec(Vector2 position);
static Rectangle GetPlayerRec(Vector2 position);
static void DrawStar(int x, int y, int frameNumber);
static void DrawPlayer(struct Player *player);
static bool IsStarLighted(int x, int y);

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
    InitWindow(screenWidth, screenHeight, "raylib 9yr gamejam");
    
    // TODO: Load resources / Initialize variables at this point

    // Load spritesheet
    spritesheet = LoadTexture("resources/spritesheet.png");

    // Init player
    ResetPlayer(&player);

    // Init camera
    ResetCamera(&camera, &player);

    // Init stars
    ResetStars();

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

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
    UnloadRenderTexture(target);
    
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
        SetWindowSize(screenWidth*screenScale, screenHeight*screenScale);
        
        // Scale mouse proportionally to keep input logic inside the 256x256 screen limits
        SetMouseScale(1.0f/(float)screenScale, 1.0f/(float)screenScale);
        
        prevScreenScale = screenScale;
    }

    // TODO: Update variables / Implement example logic at this point
    //----------------------------------------------------------------------------------

    if (IsKeyPressed(KEY_D))
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

    UpdatePlayer(&player, deltaTime);

    UpdateCameraCenterSmoothFollow(&camera, &player, deltaTime);

    InteractPlayerAndStars(&player);

    // Draw
    //----------------------------------------------------------------------------------
    // Render all screen to texture (for scaling)
    BeginTextureMode(target);
        ClearBackground(palette[0]);
        
        // TODO: Draw screen at 256x256
        DrawRectangle(10, 10, screenWidth - 20, screenHeight - 20, palette[5]);

        BeginMode2D(camera);

            if (debugMode)
            {
                rlPushMatrix();
                    rlTranslatef(0, 25*STAR_SPACING_PIXELS, 0);
                    rlRotatef(90, 1, 0, 0);
                    DrawGrid(100, STAR_SPACING_PIXELS);
                rlPopMatrix();
            }

            // Draw stars
            for (int y = 0; y < STAR_COUNT_Y; y += 1)
            {
                for (int x = 0; x < STAR_COUNT_X; x += 1)
                {
                    DrawStar(x, y, (stars[y][x].isLighted) ? 1 : 0);
                }
            }

            DrawPlayer(&player);

        EndMode2D();

    EndTextureMode();
    
    BeginDrawing();
        ClearBackground(palette[0]);
        
        // Draw render texture to screen scaled as required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width*screenScale, (float)target.texture.height*screenScale }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // Draw equivalent mouse position on the target render-texture
        DrawCircleLines(GetMouseX(), GetMouseY(), 10, MAROON);

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
    player->position = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    player->direction = (Vector2){ 0.0f, 0.0f };
    player->speed = PLAYER_SPEED;
    player->clockSeconds = 0.0f;
}

void ResetCamera(Camera2D *camera, struct Player *player)
{
    camera->target = player->position;
    camera->offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera->rotation = 0.0f;
    camera->zoom = 1.0f;
}

void ResetStars(void)
{
    for (int y = 0; y < STAR_COUNT_Y; y += 1)
    {
        for (int x = 0; x < STAR_COUNT_X; x += 1)
        {
            stars[y][x].isLighted = false;
        }
    }
}

void UpdatePlayer(struct Player *player, float deltaTime)
{
    player->clockSeconds += deltaTime;

    switch (player->state)
    {
        case PLAYER_STUNNED:
        {
            if (player->clockSeconds >= PLAYER_STUN_COOLDOWN_SECONDS)
            {
                player->clockSeconds = 0.0f;
                player->state = PLAYER_IDLE;
            }
            return;
        } break;
        case PLAYER_IDLE:
        {
            player->direction.x = 0;
            player->direction.y = 0;

            if (IsKeyDown(KEY_LEFT)) player->direction.x -= 1;
            if (IsKeyDown(KEY_RIGHT)) player->direction.x += 1;
            if (IsKeyDown(KEY_UP)) player->direction.y -= 1;
            if (IsKeyDown(KEY_DOWN)) player->direction.y += 1;

            if (IsKeyDown(KEY_LEFT_ALT))
            {
                player->speed += PLAYER_BOOST;
                player->clockSeconds = 0.0f;
                player->state = PLAYER_JUMPING;
            }
        } break;
        case PLAYER_JUMPING:
        {
            if (player->clockSeconds >= PLAYER_JUMP_COOLDOWN_SECONDS)
            {
                player->speed -= PLAYER_BOOST;
                player->clockSeconds = 0.0f;
                player->state = PLAYER_IDLE;
            }
        } break;
    }

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

void UpdateCameraCenterSmoothFollow(Camera2D *camera, struct Player *player, float delta)
{
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed*length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length));
    }
}

void InteractPlayerAndStars(struct Player *player)
{
    if (!IsKeyPressed(KEY_SPACE))
    {
        return;
    }

    int closestStarX = lrintf(player->position.x/(float)STAR_SPACING_PIXELS);
    int closestStarY = lrintf(player->position.y/(float)STAR_SPACING_PIXELS);

    const Vector2 closestStarPos = GetStarPosition(closestStarX, closestStarY);
    const Rectangle closestStarRec = GetStarRec(closestStarPos);

    const Rectangle playerRec = GetPlayerRec(player->position);

    if (CheckCollisionRecs(playerRec, closestStarRec))
    {
        LOG("Collision boy\n");
        stars[closestStarY][closestStarX].isLighted = !stars[closestStarY][closestStarX].isLighted;
    }
}

Vector2 GetStarPosition(int x, int y)
{
    Vector2 position = { x*STAR_SPACING_PIXELS, y*STAR_SPACING_PIXELS };
    return position;
}

Rectangle GetStarRec(Vector2 position)
{
    Rectangle starRec = { 0 };
    starRec.x = position.x - (float)PLAYER_REC_WIDTH_PIXELS/2.0f;
    starRec.y = position.y - (float)PLAYER_REC_HEIGHT_PIXELS/2.0f;
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

void DrawStar(int x, int y, int frameNumber)
{
    const int frameWidthPixels = 32;
    const int frameHeightPixels = 32;

    const Vector2 position = GetStarPosition(x, y);

    Rectangle source = { 0, 0, frameWidthPixels, frameHeightPixels };
    Rectangle dest = { position.x, position.y, frameWidthPixels, frameHeightPixels };

    source.x += frameNumber*frameWidthPixels;

    DrawTexturePro(spritesheet, source, dest, (Vector2){ frameWidthPixels/2.0f, frameHeightPixels/2.0f }, 0.0f, WHITE);

    if (debugMode)
    {
        const Rectangle rec = GetStarRec(position);
        DrawRectangleLinesEx(rec, 1.0f, RED);
    }
}

void DrawPlayer(struct Player *player)
{
    Rectangle playerRec = GetPlayerRec(player->position);
    DrawRectangleRec(playerRec, (player->state == PLAYER_IDLE) ? palette[3] : BLACK);
}

bool IsStarLighted(int x, int y)
{
    const int numberOfConnections = sizeof(constellation1) / sizeof(struct ConstellationElement);
    for (int i = 0; i < numberOfConnections; i += 1)
    {
        const struct ConstellationElement elem = constellation1[i];
        if (elem.isLighted)
        {
            if (((x == elem.x1) && (y == elem.y1)) || ((x == elem.x2) && (y == elem.y2)))
            {
                return true;
            }
        }
    }
    return false;
}
