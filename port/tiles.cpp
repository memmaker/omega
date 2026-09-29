/* Map tiles: Omega cell (char | colour pair | A_BOLD) -> tile in web/tiles.png,
 * the 32x32 sheet of David Kinder's WinOmega (github.com/DavidKinder/Omega).
 * map.inc = its gfxMapData, copied as is (the glyph names are rebirth's defs.h
 * too). Returns row*128+col+1, 0 = none. */
#include "../src/glob.h"

/* ponytail: stage 2 stub. map.inc keys on the old MSDOS COL_ colours, which
 * rebirth replaced by curses pairs; until they are mapped (stage 3), cells
 * match on the character alone, first entry wins. */
#define COL_BG_BLUE 0
#define COL_BG_BROWN 0
#define COL_BG_GREEN 0
#define COL_BG_RED 0
#define COL_BG_WHITE 0
#define COL_BLACK 0
#define COL_BLUE 0
#define COL_BRIGHT_WHITE 0
#define COL_BROWN 0
#define COL_CYAN 0
#define COL_FG_BLINK 0
#define COL_GREEN 0
#define COL_GREY 0
#define COL_LIGHT_BLUE 0
#define COL_LIGHT_GREEN 0
#define COL_LIGHT_PURPLE 0
#define COL_LIGHT_RED 0
#define COL_PURPLE 0
#define COL_RED 0
#define COL_WHITE 0
#define COL_YELLOW 0
static const int map[][3] = {
#include "map.inc"
};
static short tile[256];

static int key(int c) { return c & 0xff; }

extern "C" int wc_tile(int c)
{
    static bool built;
    if (!built) {
        for (const auto &m : map)
            if (!tile[key(m[0])]) tile[key(m[0])] = m[2] * 128 + m[1] + 1;
        built = true;
    }
    /* WinOmega's special cases: glyphs shared with countryside terrain */
    if (Current_Environment != E_COUNTRYSIDE) {
        if (key(c) == key('o' | CLR(WHITE))) return 1 * 128 + 69 + 1;   /* cold blast, not village */
        if (key(c) == key('!' | CLR(RED))) return 21 * 128 + 67 + 1;    /* incubus/satyr, not volcano */
    }
    return tile[key(c)];
}
