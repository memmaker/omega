# Omega 0.80.2 — RVIP import (2026-09-25)

Case O (curses, no Rogue/Moria lineage). `git log`: upstream, then the port.

- Build: `make -f port/Makefile` (X11) · web: `sh web/build.sh`, `web/deploy.sh`.
- Run: `./play.sh` or `~/Desktop/Games/Roguelikes/Omega.app`. Saves in `save/`.
- Port: `port/` (curses shim, `be_x11.c`, `be_web.c`), `rl.c` (explore `X`,
  stairs `<`/`>`, Enter menu, inventory item menu), small hooks in
  command1.c, inv.c (`getitem` cursor, `inventory_control` keys), scr.c,
  save.c, command2.c, char.c, defs.h. Help: `omegalib/help12.txt`.
- Web tiles: David Kinder's WinOmega 32x32 sheet (github.com/DavidKinder/Omega
  fae6f21, `32x32.bmp` -> `web/tiles.png`). C picks the tile: `port/tiles.c`
  (`wc_tile`, table = Kinder's `gfxMapData` in `port/map.inc`, plus his
  non-countryside cases: cold blast, incubus/satyr). Levelw cells carry
  `A_TILE` + tile in bits 18+; omega.js only blits, scrolled round the player;
  *Tiles* button, `localStorage`. X11 (`be_x11.c`) draws the same from
  `port/tiles.bmp` (Kinder's BMP as is); `OMEGA_TILES=0 ./play.sh` = text.
- Live: https://ruzzoli.de/roguelikes/omega/ (deployed 2026-09-25).
- Not done: sound (6b), mouse.
- Tested: char creation, city, countryside travel, temple explore + doors,
  menu, item menu quaff + reopen, save/restore, ASan run (clean), web
  menu/autosave/restore in the browser.
