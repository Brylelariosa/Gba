#include "move.h"
#include "world.h"

const s8 g_dir_dx[4] = { 0, 1, 0, -1 };
const s8 g_dir_dy[4] = { -1, 0, 1, 0 };

bool move_try(int x, int y, int z, int dx, int dy, int max_fall, int *out_z)
{
    int nx = x + dx, ny = y + dy;
    if (nx < 0 || ny < 0 || nx >= WX || ny >= WY) return false;

    u8 b = world_get(nx, ny, z);
    if (b == B_AIR) {
        int zz = z;
        while (zz > 0 && world_get(nx, ny, zz - 1) == B_AIR) zz--;
        u8 floor = zz > 0 ? world_get(nx, ny, zz - 1) : B_BEDROCK;
        if (floor == B_WATER) return false;
        if (z - zz > max_fall) return false;
        *out_z = zz;
        return true;
    }
    if (b == B_WATER) return false;

    /* solid block ahead: climb it if there is room above */
    if (z + 1 < WZ && world_get(nx, ny, z + 1) == B_AIR && world_get(x, y, z + 1) == B_AIR) {
        *out_z = z + 1;
        return true;
    }
    return false;
}
