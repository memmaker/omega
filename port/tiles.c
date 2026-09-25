/* Map tiles (web): Omega cell (char|colour) -> tile in web/tiles.png, the
 * 32x32 sheet of David Kinder's WinOmega (github.com/DavidKinder/Omega).
 * map.inc = its gfxMapData, copied as is. Returns row*128+col+1, 0 = none. */
#include "../glob.h"

static int map[][3] = {
#include "map.inc"
};
static short tile[0x8000];

int wc_tile(int c)
{
    int i;
    c &= 0x7fff;
    if (!tile[0])       /* SPACE is entry 0, so this is "not built yet" */
        for (i = 0; i < sizeof map / sizeof map[0]; i++)
            tile[map[i][0] & 0x7fff] = map[i][2] * 128 + map[i][1] + 1;
    /* WinOmega's special cases: glyphs shared with countryside terrain */
    if (Current_Environment != E_COUNTRYSIDE) {
        if (c == ('o' | COL_WHITE)) return 1 * 128 + 69 + 1;   /* cold blast, not village */
        if (c == ('!' | COL_RED)) return 21 * 128 + 67 + 1;    /* incubus/satyr, not volcano */
    }
    return tile[c];
}
