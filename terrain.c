#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "terrain.h"

float terrain_scale_planar = 2.0;
float terrain_scale_height = 10.0;

typedef struct
{
    Model model;
    Vector3 pos;
    Vector3 scale;
    Vector2 size; // w,h
} Terrain;

static Terrain terrain;
static Texture2D grass;
static Image heightmap_image;

void terrain_init()
{
    heightmap_image = LoadImage("textures/heightmap.png");
    LoadTextureFromImage(heightmap_image);

    grass = LoadTexture("textures/grass.png");

    terrain.size = (Vector2) {heightmap_image.width - 1.0, heightmap_image.height - 1.0};
    terrain.scale = (Vector3){ terrain_scale_planar*(terrain.size.x), terrain_scale_height, terrain_scale_planar*(terrain.size.y) };
    terrain.pos = (Vector3) {-0.5*terrain.scale.x, 0.0, -0.5*terrain.scale.z}; // offset terrain mesh so center is at (0,0,0)

    Mesh mesh = GenMeshHeightmap(heightmap_image, terrain.scale);
    terrain.model = LoadModelFromMesh(mesh);

    terrain.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grass;
}

void terrain_update()
{
    Vector3 scale_prior = terrain.scale;

    terrain.scale = (Vector3){ terrain_scale_planar*(terrain.size.x), terrain_scale_height, terrain_scale_planar*(terrain.size.y) };
    terrain.pos = (Vector3) {-0.5*terrain.scale.x, 0.0, -0.5*terrain.scale.z};

    if(Vector3Equals(terrain.scale, scale_prior) == 0)
    {
        Mesh mesh = GenMeshHeightmap(heightmap_image, terrain.scale);
        UnloadModel(terrain.model);
        terrain.model = LoadModelFromMesh(mesh);
        terrain.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grass;
    }
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
    // check if point is outside terrain mesh, and set height to 0 if so
    if((ABS(x) >= 0.5 * terrain.scale.x) || (ABS(z) >= 0.5 * terrain.scale.z))
    {
        ground->height = 0.0;
        return 0.0;
    }

    // find starting index in terrain mesh vertices list based on (x,z)
    float _x = floor(x/terrain_scale_planar) + (terrain.size.x / 2.0);
    float _z = floor(z/terrain_scale_planar) + (terrain.size.y / 2.0);
    int p = (int)(18*(terrain.size.y*_z + _x)); // 18 floats per quad (6 vertices * 3 axis)

    Mesh* m = &terrain.model.meshes[0];

    float dx = (x/terrain_scale_planar) - floor(x/terrain_scale_planar);
    float dz = (z/terrain_scale_planar) - floor(z/terrain_scale_planar);

    // find the specific terrain triangle (points a,b,c) where (x,z) is within
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

    // calculate the y value based on the point in the triangle

    // plane equ: rx+sy+tz=k
    Vector3 v1 = (Vector3){ground->a.x - ground->b.x, ground->a.y - ground->b.y, ground->a.z - ground->b.z};
    Vector3 v2 = (Vector3){ground->a.x - ground->c.x, ground->a.y - ground->c.y, ground->a.z - ground->c.z};
    Vector3 n = Vector3CrossProduct(v1,v2);

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
