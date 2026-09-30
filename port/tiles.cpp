/* Map tiles: Omega screen cell -> tile in web/tiles.png, the 32x32 sheet of
 * David Kinder's WinOmega (github.com/DavidKinder/Omega). map.inc = its
 * gfxMapData, copied as is. Cells are keyed as WinOmega keys them: char |
 * PC fg << 8 | PC bg << 12 (the MSDOS COL_ values). The symbolic entries
 * (WALL, FLOOR ...) are rebirth's curses pairs: wc_fold() turns them into
 * the same PC colours the screen shows; the raw COL_ entries are PC values
 * already (marked with RAW). Returns row*128+col+1, 0 = none (text).
 * Wc_tileset 2: the gromega sheet, row*32+col+1. */
#include "../src/glob.h"

#define RAW 0x40000000
#define COL_BLACK (RAW | 0x0000)
#define COL_BLUE (RAW | 0x0100)
#define COL_GREEN (RAW | 0x0200)
#define COL_CYAN (RAW | 0x0300)
#define COL_RED (RAW | 0x0400)
#define COL_PURPLE (RAW | 0x0500)
#define COL_BROWN (RAW | 0x0600)
#define COL_WHITE (RAW | 0x0700)
#define COL_GREY (RAW | 0x0800)
#define COL_LIGHT_BLUE (RAW | 0x0900)
#define COL_LIGHT_GREEN (RAW | 0x0a00)
#define COL_LIGHT_RED (RAW | 0x0c00)
#define COL_LIGHT_PURPLE (RAW | 0x0d00)
#define COL_YELLOW (RAW | 0x0e00)
#define COL_BRIGHT_WHITE (RAW | 0x0f00)
#define COL_BG_BLUE (RAW | 0x1000)
#define COL_BG_GREEN (RAW | 0x2000)
#define COL_BG_RED (RAW | 0x4000)
#define COL_BG_BROWN (RAW | 0x6000)
#define COL_BG_WHITE (RAW | 0x7000)
#define COL_FG_BLINK RAW
static const unsigned map[][3] = {
#include "map.inc"
};
static short tile[0x8000];

extern "C" chtype wc_fold(chtype);

/* the second set: gromega 0.80.2a's omegalib/omegatiles.xpm (8x12 cells, 32
 * a row, code = row*32+col; web/gromega.png is the sheet without its grid).
 * gromega gives each monster and terrain piece its own code (its ochars.h);
 * here, as for WinOmega, only the screen cell is known: monsters by name ->
 * their char/colour (port/gromega.inc, made from its minit.h + ochars.h by
 * a regex over ",C_xxx,\"name\"" lines), terrain and item classes by hand,
 * no wall/river joining. Set from the page (be_web.c): 0 text, 1 WinOmega,
 * 2 gromega. */
extern "C" int Wc_tileset;
int Wc_tileset = 1;
static const struct { const char *name; int code; } gmon[] = {
#include "gromega.inc"
};
static const unsigned gloc[][2] = {
  {WALL, 0xd0}, {PORTCULLIS, 0x142}, {OPEN_DOOR, 0x145}, {CLOSED_DOOR, 0x144},
  {WHIRLWIND, 0x13e}, {ABYSS, 0xff}, {LAVA, 0x137}, {HEDGE, 0x100}, {WATER, 0xa0},
  {FIRE, 0x13d}, {TRAP, 0x1f5}, {LIFT, 0x13c}, {STAIRS_UP, 0x139}, {STAIRS_DOWN, 0x138},
  {FLOOR, 0x146}, {PLAYER, 0x200}, {CORPSE, 0x208}, {STATUE, 0x13b}, {RUBBLE, 0xcf},
  {ALTAR, 0x136}, {CASH, 0x1f9}, {PILE, 0x208}, {FOOD, 0x221}, {WEAPON, 0x1e1},
  {MISSILEWEAPON, 0x1ef}, {SCROLL, 0x1dc}, {POTION, 0x1de}, {ARMOR, 0x1fe},
  {SHIELD, 0x1df}, {CLOAK, 0x1e0}, {BOOTS, 0x1fd}, {STICK, 0x1f6}, {RING, 0x1f3},
  {THING, 0x1ff}, {ARTIFACT, 0x20f}, {PLAINS, 0x3b0}, {TUNDRA, 0x3b0},
  {MOUNTAINS, 0x240}, {PASS, 0x450}, {CITY, 0x4c8}, {VILLAGE, 0x4c9}, {FOREST, 0x370},
  {JUNGLE, 0x4e0}, {SWAMP, 0x340}, {VOLCANO, 0x4d9}, {CASTLE, 0x4d6}, {TEMPLE, 0x4d2},
  {CAVES, 0x4d8}, {DESERT, 0x368}, {CHAOS_SEA, 0x4a0}, {STARPEAK, 0x4d7},
  {DRAGONLAIR, 0x4da}, {MAGIC_ISLE, 0x4db}, {CHAIR, 0x148}, {SAFE, 0x13a},
  {FURNITURE, 0x148}, {BED, 0x148},
};
static short gtile[0x8000];
static int gro_tile(int c)
{
  static bool built;
  if(!built)
  {
    for(const auto &m : Monsters)
    {
      for(const auto &g : gmon)
      {
        int k = wc_fold(m.monchar) & 0x7fff;
        if(m.monstring == g.name && !gtile[k])
        {
          gtile[k] = g.code + 1; // the first monster of a glyph wins
        }
      }
    }
    for(const auto &l : gloc)
    {
      gtile[wc_fold(l[0]) & 0x7fff] = l[1] + 1;
    }
    built = true;
  }
  return gtile[c];
}

/* c: a screen cell (already folded) */
extern "C" int wc_tile(int c)
{
  static bool built;
  c &= 0x7fff;
  if(Wc_tileset == 2)
  {
    return gro_tile(c);
  }
  if(!built)
  {
    for(const auto &m : map)
    {
      int k = (m[0] & RAW ? m[0] : wc_fold(m[0])) & 0x7fff;
      tile[k] = m[2] * 128 + m[1] + 1; // a later entry wins, as in WinOmega
    }
    built = true;
  }
  // WinOmega's special cases: glyphs shared with countryside terrain
  if(Current_Environment != E_COUNTRYSIDE)
  {
    if(c == ('o' | 0x0700))
    {
      return 1 * 128 + 69 + 1; // cold blast, not village
    }
    if(c == ('!' | 0x0400))
    {
      return 21 * 128 + 67 + 1; // incubus/satyr, not volcano
    }
  }
  return tile[c];
}

/* a game glyph (char | pair | A_BOLD): its tile, and its PC colour */
extern "C" int wc_tile_of(chtype c)
{
  return wc_tile(wc_fold(c));
}
extern "C" int wc_colour_of(chtype c)
{
  chtype v = wc_fold(c);
  return v & 0xff00 ? (v >> 8) & 15 : 7;
}
