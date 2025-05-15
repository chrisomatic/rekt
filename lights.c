#include "raylib.h"
#include "raymath.h"
#include "common.h"
#include "lights.h"

#define MAX_LIGHTS  4

static int lightsCount = 0;    // Current amount of created lights
Shader lights_shader;

Light lights[MAX_LIGHTS] = { 0 };

void lights_init()
{
    lights_shader = LoadShader("shaders/lighting.vs", "shaders/lighting.fs");
    lights_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lights_shader, "viewPos");

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(lights_shader, "ambient");
    SetShaderValue(lights_shader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create lights
    lights[0] = CreateLight(LIGHT_DIRECTIONAL, (Vector3){ -50, 50, -50 }, Vector3Zero(), WHITE, lights_shader);

    for (int i = 0; i < MAX_LIGHTS; i++)
        UpdateLightValues(lights_shader, lights[i]);
}

void lights_update()
{
    for (int i = 0; i < MAX_LIGHTS; i++)
        UpdateLightValues(lights_shader, lights[i]);
}

Light CreateLight(int type, Vector3 position, Vector3 target, Color color, Shader shader)
{
    Light light = { 0 };

    if (lightsCount < MAX_LIGHTS)
    {
        light.enabled = true;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color = color;
        light.attenuation = 0.2;

        light.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightsCount));
        light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightsCount));
        light.positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", lightsCount));
        light.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightsCount));
        light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightsCount));
        
        lightsCount++;
    }

    return light;
}

void lights_draw()
{
    if(g_debug)
    {
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            DrawSphere(lights[i].position, 1.00, lights[i].color);
        }
    }

}

void UpdateLightValues(Shader shader, Light light)
{
    // Send to shader light enabled state and type
    SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

    // Send to shader light position values
    float position[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = { light.target.x, light.target.y, light.target.z };
    SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);

    // Send to shader light color values
    float color[4] = { (float)light.color.r/(float)255, (float)light.color.g/(float)255, 
                       (float)light.color.b/(float)255, (float)light.color.a/(float)255 };
    SetShaderValue(shader, light.colorLoc, color, SHADER_UNIFORM_VEC4);
}
