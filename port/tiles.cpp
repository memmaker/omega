/* Map tiles: Omega screen cell -> tile in web/tiles.png, the 32x32 sheet of
 * David Kinder's WinOmega (github.com/DavidKinder/Omega). map.inc = its
 * gfxMapData, copied as is. Cells are keyed as WinOmega keys them: char |
 * PC fg << 8 | PC bg << 12 (the MSDOS COL_ values). The symbolic entries
 * (WALL, FLOOR ...) are rebirth's curses pairs: wc_fold() turns them into
 * the same PC colours the screen shows; the raw COL_ entries are PC values
 * already (marked with RAW). Returns row*128+col+1, 0 = none. */
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

/* c: a screen cell (already folded) */
extern "C" int wc_tile(int c)
{
  static bool built;
  c &= 0x7fff;
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
