#include "common.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "sky.h"

static Model skybox_model;

void sky_init()
{
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    skybox_model = LoadModelFromMesh(cube);

    skybox_model.materials[0].shader = LoadShader("shaders/skybox.vs", "shaders/skybox.fs");

    SetShaderValue(skybox_model.materials[0].shader, GetShaderLocation(skybox_model.materials[0].shader, "environmentMap"), (int[1]){ MATERIAL_MAP_CUBEMAP }, SHADER_UNIFORM_INT);
    SetShaderValue(skybox_model.materials[0].shader, GetShaderLocation(skybox_model.materials[0].shader, "doGamma"), (int[1]) { 0 }, SHADER_UNIFORM_INT);
    SetShaderValue(skybox_model.materials[0].shader, GetShaderLocation(skybox_model.materials[0].shader, "vflipped"), (int[1]){ 0 }, SHADER_UNIFORM_INT);

    // Load cubemap shader and setup required shader locations
    Shader shdrCubemap = LoadShader("shaders/cubemap.vs","shaders/cubemap.fs");
    SetShaderValue(shdrCubemap, GetShaderLocation(shdrCubemap, "equirectangularMap"), (int[1]){ 0 }, SHADER_UNIFORM_INT);

    Image img = LoadImage("textures/skybox2.png");
    skybox_model.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(img, CUBEMAP_LAYOUT_AUTO_DETECT);
    UnloadImage(img);
}

void sky_update()
{
}

void sky_draw()
{
    // We are inside the cube, we need to disable backface culling!
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
        DrawModel(skybox_model, (Vector3){0, 0, 0}, 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}
