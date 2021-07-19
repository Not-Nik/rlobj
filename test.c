// rlobj (c) Nikolas Wipper 2021

#include "rlobj.h"
#include "stdio.h"
#include "raymath.h"

typedef struct TwoTimes {
    double t1, t2;
} TwoTimes;

TwoTimes CompareLoading(const char *filename) {
    double start1 = GetTime();
    Model model = LoadModel(filename);
    double total1 = GetTime() - start1;

    double start2 = GetTime();
    Model model2 = LoadObj(filename);
    double total2 = GetTime() - start2;

    UnloadModel(model);
    UnloadModel(model2);

    return (TwoTimes) {.t1 = total1 * 1000000, .t2 = total2 * 1000000};
}

void Bench() {
    // From "models" examples
    TwoTimes bridge = CompareLoading("raylib/examples/models/resources/models/bridge.obj");
    TwoTimes castle = CompareLoading("raylib/examples/models/resources/models/castle.obj");
    TwoTimes cube = CompareLoading("raylib/examples/models/resources/models/cube.obj");
    TwoTimes house = CompareLoading("raylib/examples/models/resources/models/house.obj");
    TwoTimes market = CompareLoading("raylib/examples/models/resources/models/market.obj");
    TwoTimes turret = CompareLoading("raylib/examples/models/resources/models/turret.obj");
    TwoTimes well = CompareLoading("raylib/examples/models/resources/models/well.obj");

    // From "shaders" examples
    TwoTimes barracks = CompareLoading("raylib/examples/shaders/resources/models/barracks.obj");
    TwoTimes church = CompareLoading("raylib/examples/shaders/resources/models/church.obj");
    TwoTimes watermill = CompareLoading("raylib/examples/shaders/resources/models/barracks.obj");

    // μ is counted as two characters by printf so the format args have to be one char longer than below
    printf("| %15s | %22s | %11s | %13s | %8s |\n", "model name", "tinyobj-loader-c (μs)", "rlobj (μs)", "diff (μs)", "speedup");

    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "bridge.obj", bridge.t1, bridge.t2, bridge.t2 - bridge.t1, bridge.t1 / bridge.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "castle.obj", castle.t1, castle.t2, castle.t2 - castle.t1, castle.t1 / castle.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "cube.obj", cube.t1, cube.t2, cube.t2 - cube.t1, cube.t1 / cube.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "house.obj", house.t1, house.t2, house.t2 - house.t1, house.t1 / house.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "market.obj", market.t1, market.t2, market.t2 - market.t1, market.t1 / market.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "turret.obj", turret.t1, turret.t2, turret.t2 - turret.t1, turret.t1 / turret.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "well.obj", well.t1, well.t2, well.t2 - well.t1, well.t1 / well.t2);

    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "barracks.obj", barracks.t1, barracks.t2, barracks.t2 - barracks.t1, barracks.t1 / barracks.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "church.obj", church.t1, church.t2, church.t2 - church.t1, church.t1 / church.t2);
    printf("| %15s | %21.2f | %10.2f | %12.2f | %8.2f |\n", "watermill.obj", watermill.t1, watermill.t2, watermill.t2 - watermill.t1, watermill.t1 / watermill.t2);

    putchar('\n');

    double t1 = bridge.t1 + castle.t1 + cube.t1 + house.t1 + market.t1 + turret.t1 + well.t1 + barracks.t1 + church.t1 + watermill.t1;
    double t2 = bridge.t2 + castle.t2 + cube.t2 + house.t2 + market.t2 + turret.t2 + well.t2 + barracks.t2 + church.t2 + watermill.t2;
    printf("time to load all models with tinyobj-loader-c: %.2f\n", t1);
    printf("time to load all models with rlobj: %.2f\n", t2);
    printf("total difference in microseconds: %.2f\n", t2 - t1);
    printf("total speedup: %.2f\n", t1 / t2);
}

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 840;

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_ERROR);

    InitWindow(screenWidth, screenHeight, "raylib [models] test - models loading");

    SetTargetFPS(60);

    Camera camera = {0};
    camera.position = (Vector3) {50.0f, 50.0f, 50.0f};
    camera.target = (Vector3) {0.0f, 10.0f, 0.0f};
    camera.up = (Vector3) {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Bench();

    Model model = LoadObj("raylib/examples/models/resources/models/castle.obj");

    Vector3 position = {0.0f, 0.0f, 0.0f};

    SetCameraMode(camera, CAMERA_ORBITAL);

    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(model, position, 1.f, WHITE);

        DrawGrid(20, 10.0f);

        EndMode3D();

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadModel(model);         // Unload model

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
