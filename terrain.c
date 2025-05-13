#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "terrain.h"

#define TERRAIN_SCALE_PLANAR 1.0
#define TERRAIN_SCALE_HEIGHT 12.0

typedef struct
{
    Model model;
    Vector3 pos;
    Vector3 scale;
    Vector2 size; // w,h
} Terrain;

static Terrain terrain;

void terrain_init()
{
    Image image = LoadImage("textures/heightmap.png");
    LoadTextureFromImage(image);
    Texture2D grass = LoadTexture("textures/grass.png");

    terrain.size = (Vector2) {image.width - 1.0, image.height - 1.0};
    terrain.scale = (Vector3){ TERRAIN_SCALE_PLANAR*(terrain.size.x), TERRAIN_SCALE_HEIGHT, TERRAIN_SCALE_PLANAR*(terrain.size.y) };
    //terrain.scale = (Vector3){ 1.0, 0.2, 1.0 };
    Mesh mesh = GenMeshHeightmap(image, terrain.scale);
    terrain.model = LoadModelFromMesh(mesh);
    terrain.pos = (Vector3) {-0.5*terrain.scale.x, 0.0, -0.5*terrain.scale.z}; // offset terrain mesh so center is at (0,0,0)

    terrain.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grass;
    UnloadImage(image);
}

void terrain_draw()
{
    if(g_debug)
    {
        DrawModelWires(terrain.model, terrain.pos, 1.0, GREEN);
    }
    else
    {
        DrawModel(terrain.model, terrain.pos, 1.0f, WHITE);
    }
}

float terrain_get_ground(float x, float z, Ground* ground)
{
    float grid_square_size = 1.0; //TERRAIN_SCALE_PLANAR;
    float dx = x - floor(x)*grid_square_size;
    float dz = z - floor(z)*grid_square_size;

    int _x = floor(x) + (terrain.size.x / 2.0);
    int _z = floor(z) + (terrain.size.y / 2.0);
    int p = (int)(18*(terrain.size.y*_z + _x));

    Mesh* m = &terrain.model.meshes[0];
    if(ABS(x) >= terrain.scale.x / 2.0 || ABS(z) >= terrain.scale.z / 2.0)
    {
        ground->height = 0.0;
        return 0.0;
    }

    //printf("x: %d, z: %d\n", _x, _z);

    if (dx <= (1.0-dz))
    {
        ground->a = Vector3Add(terrain.pos, (Vector3){m->vertices[p],m->vertices[p+1],m->vertices[p+2]});
        ground->b = Vector3Add(terrain.pos, (Vector3){m->vertices[p+3],m->vertices[p+4],m->vertices[p+5]});
        ground->c = Vector3Add(terrain.pos, (Vector3){m->vertices[p+6],m->vertices[p+7],m->vertices[p+8]});
    }
    else
    {
        ground->a = Vector3Add(terrain.pos, (Vector3){m->vertices[p+9],m->vertices[p+10],m->vertices[p+11]});
        ground->b = Vector3Add(terrain.pos, (Vector3){m->vertices[p+12],m->vertices[p+13],m->vertices[p+14]});
        ground->c = Vector3Add(terrain.pos, (Vector3){m->vertices[p+15],m->vertices[p+16],m->vertices[p+17]});
    }

    // plane equ: rx+sy+tz=k
    Vector3 v1 = (Vector3){ground->a.x - ground->b.x, ground->a.y - ground->b.y, ground->a.z - ground->b.z};
    Vector3 v2 = (Vector3){ground->a.x - ground->c.x, ground->a.y - ground->c.y, ground->a.z - ground->c.z};
    Vector3 n = Vector3CrossProduct(v1,v2);

    // compute k
    float k = Vector3DotProduct(n,ground->a);

    float rx = n.x * x;
    float tz = n.z * z;
    float s  = n.y;

    float y = (s == 0.0) ? 0.0 : (k - rx - tz) / s;
    ground->height = y;

#if 0
    normal(ground->a, ground->b, ground->c,&ground->normal);
    //ground->normal.x = terrain.vertices[index].normal.x;
    //ground->normal.y = terrain.vertices[index].normal.y;
    //ground->normal.z = terrain.vertices[index].normal.z;
#endif

    return y;
}
