# Omega 0.80.2 — RVIP import (2026-09-25)

Case O (curses, no Rogue/Moria lineage). `git log`: upstream, then the port.

- Build: `make -f port/Makefile` (X11) · web: `sh web/build.sh`, `web/deploy.sh`.
- Run: `./play.sh` or `~/Desktop/Games/Roguelikes/Omega.app`. Saves in `save/`.
- Port: `port/` (curses shim, `be_x11.c`, `be_web.c`), `rl.c` (explore `X`,
  stairs `<`/`>`, Enter menu, inventory item menu), small hooks in
  command1.c, inv.c (`getitem` cursor, `inventory_control` keys), scr.c,
  save.c, command2.c, char.c, defs.h. Help: `omegalib/help12.txt`.
- Text only, 16 colours: no tile set exists for Omega.
- Not done: sound (6b), mouse, deploy to ruzzoli.de (waiting for OK).
- Tested: char creation, city, countryside travel, temple explore + doors,
  menu, item menu quaff + reopen, save/restore, ASan run (clean), web
  menu/autosave/restore in the browser.
