#pragma once

typedef struct {   
    int type;
    bool enabled;
    Vector3 position;
    Vector3 target;
    Color color;
    float attenuation;
    
    // Shader locations
    int enabledLoc;
    int typeLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int attenuationLoc;
} Light;

typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT
} LightType;

extern Shader lights_shader;

void lights_init();
void lights_update();
void lights_draw();
Light CreateLight(int type, Vector3 position, Vector3 target, Color color, Shader shader);
void UpdateLightValues(Shader shader, Light light);
