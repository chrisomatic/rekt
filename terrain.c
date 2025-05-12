#include "common.h"
#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "terrain.h"

static Model model;
static Vector3 position;
static Vector3 scale;

void terrain_init()
{
    Image image = LoadImage("textures/heightmap.png");
    Texture2D texture = LoadTextureFromImage(image);
    Texture2D grass = LoadTexture("textures/grass.png");

    scale = (Vector3){ 256.0, 10.0, 256.0 };
    Mesh mesh = GenMeshHeightmap(image, scale);
    model = LoadModelFromMesh(mesh);

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = grass;
    UnloadImage(image);
}

void terrain_draw()
{
    position = Vector3Scale(scale, -0.5);
    //DrawModel(model, position, 1.0f, WHITE);
    DrawModelWires(model, position, 1.0, GREEN);

    if(g_debug)
    {

        int x = floor(player.pos.x) + 128;
        int z = floor(player.pos.z) + 128;

        int p1 = 18*(256*z + x);
        int p2 = 18*(256*z + x)+3;
        int p3 = 18*(256*z + x)+6;

        Mesh* m = &model.meshes[0];
        Vector3 a = Vector3Add(position, (Vector3){m->vertices[p1],m->vertices[p1+1],m->vertices[p1+2]});
        Vector3 b = Vector3Add(position, (Vector3){m->vertices[p2],m->vertices[p2+1],m->vertices[p2+2]});
        Vector3 c = Vector3Add(position, (Vector3){m->vertices[p3],m->vertices[p3+1],m->vertices[p3+2]});

        printf("abc: [%f %f %f] [%f %f %f] [%f %f %f]\n", a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);

        DrawSphere(a, 0.03, RED);
        DrawSphere(b, 0.03, GREEN);
        DrawSphere(c, 0.03, BLUE);
    }
}

#if 0
void terrain_get_info(float x, float z, GroundInfo* ground)
{

    float terrain_x = terrain.pos.x - x;
    float terrain_z = terrain.pos.z - z;

    float grid_square_size = TERRAIN_PLANAR_SCALE;

    memset(ground,0,sizeof(GroundInfo));

    int grid_x = (int)floor(terrain_x / grid_square_size);
    if(grid_x < 0 || grid_x >= terrain.w-1)
        return;

    int grid_z = (int)floor(terrain_z / grid_square_size);
    if(grid_z < 0 || grid_z >= terrain.l-1)
        return;

    /*
    printf("-----------------------------\n");
    printf("grid_x: %d, grid_z: %d\n",grid_x, grid_z);
    printf("-----------------------------\n");
    */

    int index = grid_z*(terrain.l-1)+grid_x;

    float x_coord = fmod(terrain_x,grid_square_size)/grid_square_size;
    float z_coord = fmod(terrain_z,grid_square_size)/grid_square_size;

    int i0,i1,i2;

    if (x_coord <= (1.0f-z_coord))
    {
        i0 = terrain.indices[6*index+0];
        i1 = terrain.indices[6*index+1];
        i2 = terrain.indices[6*index+2];
    }
    else
    {
        i0 = terrain.indices[6*index+3];
        i1 = terrain.indices[6*index+4];
        i2 = terrain.indices[6*index+5];
    }

    ground->a.x = terrain.vertices[i0].position.x;
    ground->a.y = terrain.vertices[i0].position.y;
    ground->a.z = terrain.vertices[i0].position.z;

    ground->b.x = terrain.vertices[i1].position.x;
    ground->b.y = terrain.vertices[i1].position.y;
    ground->b.z = terrain.vertices[i1].position.z;

    ground->c.x = terrain.vertices[i2].position.x;
    ground->c.y = terrain.vertices[i2].position.y;
    ground->c.z = terrain.vertices[i2].position.z;

    ground->height = get_y_value_on_plane(terrain_x,terrain_z,&ground->a,&ground->b,&ground->c); // @NEG
    ground->height *= -1; // @NEG

    normal(ground->a, ground->b, ground->c,&ground->normal);
    //ground->normal.x = terrain.vertices[index].normal.x;
    //ground->normal.y = terrain.vertices[index].normal.y;
    //ground->normal.z = terrain.vertices[index].normal.z;
}
#endif
