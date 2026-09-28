/* rl.c -- added for the ~/Games port (RVIP): auto-explore (X), walking to
   known stairs (< >), the command menu on Enter, and the item action menu
   of the inventory. */

#include "glob.h"
#ifndef MSDOS
# include <curses.h>
#endif

int Msg_count = 0;              /* bumped by buffercycle(): a new message */
int Rl_reopen = 0;              /* reopen the inventory after this command */
int Rl_at_prompt = 0;           /* waiting for a command key (web autosave) */
int Rl_saved = 0;               /* the player saved with S (keep the file) */

static int mode = 0;            /* 0, 'X' exploring, '<' / '>' walking to stairs */
static int msgs;                /* Msg_count when the walk started */
static struct level *lvl;       /* the level visited[] belongs to */
static int lvl_env;
static char visited[MAXWIDTH][MAXLENGTH];
static char vi_key[9] = "nubylhjk"; /* Dirs[] index -> key, see getdir() */

static void check_level()
{
  if (Level != lvl || Current_Environment != lvl_env) {
    memset(visited, 0, sizeof visited);
    lvl = Level;
    lvl_env = Current_Environment;
  }
}

/* harmful or impassable for the explorer */
static int blocked(x, y)
int x, y;
{
  struct location *l = &Level->site[x][y];
  short c = l->locchar;
  if (!loc_statusp(x, y, SEEN) || loc_statusp(x, y, SECRET)) return TRUE;
  if (visited[x][y] == 2) return TRUE;          /* locked door */
  /* a site that runs a function (shop door, altar, trap text ...): once */
  if (visited[x][y] && l->p_locf != L_NO_OP) return TRUE;
  return c == WALL || c == STATUE || c == PORTCULLIS || c == HEDGE ||
    c == LAVA || c == ABYSS || c == VOID_CHAR || c == FIRE || c == WHIRLWIND ||
    c == WATER || c == LIFT || c == TRAP || c == RUBBLE ||
    (l->creature != NULL);
}

static int is_target(x, y)
int x, y;
{
  int d;
  if (mode == '<') return Level->site[x][y].locchar == STAIRS_UP;
  if (mode == '>') return Level->site[x][y].locchar == STAIRS_DOWN;
  if (visited[x][y]) return FALSE;
  if (Level->site[x][y].things != NULL) return TRUE;
  for (d = 0; d < 8; d++) {
    int nx = x + Dirs[0][d], ny = y + Dirs[1][d];
    if (inbounds(nx, ny) && !loc_statusp(nx, ny, SEEN)) return TRUE;
  }
  return FALSE;
}

/* BFS over known ground: the Dirs[] index of the first step to the
   nearest target, or -1 */
static int first_step()
{
  static short qx[MAXWIDTH * MAXLENGTH], qy[MAXWIDTH * MAXLENGTH];
  static signed char from[MAXWIDTH][MAXLENGTH];
  int h = 0, t = 0, d, x, y;
  memset(from, -1, sizeof from);
  from[Player.x][Player.y] = 8;
  qx[t] = Player.x; qy[t++] = Player.y;
  while (h < t) {
    x = qx[h]; y = qy[h++];
    if (h > 1 && is_target(x, y)) {
      /* walk back to the step next to the player */
      while (from[x][y] != 8) {
        d = from[x][y];
        if (x - Dirs[0][d] == Player.x && y - Dirs[1][d] == Player.y) return d;
        x -= Dirs[0][d]; y -= Dirs[1][d];
      }
    }
    for (d = 0; d < 8; d++) {
      int nx = x + Dirs[0][d], ny = y + Dirs[1][d];
      if (!inbounds(nx, ny) || from[nx][ny] >= 0) continue;
      if (blocked(nx, ny) && !(mode != 'X' && is_target(nx, ny) &&
                               Level->site[nx][ny].creature == NULL))
        continue;
      from[nx][ny] = d;
      qx[t] = nx; qy[t++] = ny;
    }
  }
  return -1;
}

/* a monster in view; in town only hostile ones (townsfolk are everywhere) */
static int monster_in_view()
{
  pml ml;
  int town = Current_Environment == E_CITY || Current_Environment == E_VILLAGE;
  for (ml = Level->mlist; ml != NULL; ml = ml->next) {
    struct monster *m = ml->m;
    if (m->hp <= 0 || !view_los_p(Player.x, Player.y, m->x, m->y)) continue;
    if (m_statusp(m, M_INVISIBLE) && !Player.status[TRUESIGHT]) continue;
    if (town && !m_statusp(m, HOSTILE)) continue;
    return TRUE;
  }
  return FALSE;
}

static void stop(s)
char *s;
{
  mode = 0;
  if (s) print3(s);
}

/* next key for an ongoing walk, 0 = none (the player types) */
int rl_auto()
{
  int d, nx, ny;
  if (Current_Environment == E_COUNTRYSIDE) mode = 0;
  if (Rl_reopen == 2) Rl_reopen = 1;   /* the queued command runs first */
  else if (Rl_reopen) {
    Rl_reopen = 0;
    if (!monster_in_view()) return 'i';
  }
  if (!mode) return 0;
  check_level();
  visited[Player.x][Player.y] |= 1;
  if (Msg_count != msgs) { stop(NULL); return 0; }
  if (wc_kbhit()) { stop(NULL); return 0; }
  if (monster_in_view()) { stop("You see a monster."); return 0; }
  if (mode != 'X' && Level->site[Player.x][Player.y].locchar ==
      (mode == '<' ? STAIRS_UP : STAIRS_DOWN)) {
    mode = 0;                   /* arrived: the player presses < / > again */
    return 0;
  }
  if ((d = first_step()) < 0) {
    stop(mode == 'X' ? "Nothing left to explore." : "No known way there.");
    return 0;
  }
  nx = Player.x + Dirs[0][d]; ny = Player.y + Dirs[1][d];
  if (Level->site[nx][ny].locchar == CLOSED_DOOR) {
    if (Level->site[nx][ny].aux == LOCKED) {
      visited[nx][ny] = 2;
      stop("That door seems to be locked.");
      return 0;
    }
    wc_push(vi_key[d]);         /* opendoor() asks for the direction */
    return 'o';
  }
  return vi_key[d];
}

static void start(m)
int m;
{
  mode = m;
  msgs = Msg_count;
}

/* the command menu, parsed from the command list help file */
#define MAXCMD 64
static int cmd_menu()
{
  static char text[MAXCMD][80];
  static int keys[MAXCMD];
  FILE *f;
  char line[200], *a, *b;
  int n = 0, w = 0, i, sel = 0, top = 0, h, c, y0, x0;
  WINDOW *save, *win;

  strcpy(Str1, Omegalib);
  strcat(Str1, Current_Environment == E_COUNTRYSIDE ? "help13.txt" : "help12.txt");
  if (!(f = fopen(Str1, "r"))) return ' ';
  while (n < MAXCMD && fgets(line, sizeof line, f)) {
    /* "key  : description   : time" */
    if (!(a = strchr(line, ':')) || !(b = strrchr(line, ':')) || a == b) continue;
    for (i = 0; line[i] && line[i] != ' ' && line[i] != ':'; i++) ;
    /* one key or ^x; not "vi keys" (movement stays off the menu) */
    if (!(i == 1 || (i == 2 && line[0] == '^'))) continue;
    keys[n] = i == 2 && line[0] == '^' ? line[1] & 0x1f : line[0];
    for (*b = 0; b > a && b[-1] == ' '; ) *--b = 0;
    for (a++; *a == ' '; a++) ;
    sprintf(text[n], "%-3.*s %s", i, line, a);
    text[n][60] = 0;
    if ((int)strlen(text[n]) > w) w = strlen(text[n]);
    n++;
  }
  fclose(f);
  if (!n) return ' ';
  h = min(n, LINES - 2);
  y0 = (LINES - h - 2) / 2; x0 = (COLS - w - 4) / 2;
  save = dupwin(curscr);
  win = newwin(h + 2, w + 4, y0, x0);
  for (;;) {
    if (sel < top) top = sel;
    if (sel >= top + h) top = sel - h + 1;
    werase(win);
    for (i = 0; i < w + 4; i++) {
      mvwaddch(win, 0, i, '-');
      mvwaddch(win, h + 1, i, '-');
    }
    for (i = 0; i < h; i++) {
      mvwaddch(win, i + 1, 0, '|');
      mvwaddch(win, i + 1, w + 3, '|');
      if (top + i == sel) wstandout(win);
      mvwprintw(win, i + 1, 2, "%-*s", w, text[top + i]);
      wstandend(win);
    }
    if (top) mvwaddstr(win, 0, 2, " more ");
    if (top + h < n) mvwaddstr(win, h + 1, 2, " more ");
    wmove(win, sel - top + 1, 2);
    c = wgetch(win);
    if (c == 'j' || c == '2') sel = (sel + 1) % n;
    else if (c == 'k' || c == '8') sel = (sel + n - 1) % n;
    else if (c == '\n' || c == '\r' || c == '5' || c == ' ') { c = keys[sel]; break; }
    else if (c == ESCAPE || c == '0' || c == '.') { c = ' '; break; }
    else {
      for (i = 0; i < n && keys[i] != c; i++) ;
      if (i < n) break;
    }
  }
  delwin(win);
  touchwin(save);
  wrefresh(save);
  delwin(save);
  return c;
}

/* the command key the player (or a walk) gives, after the port's own keys */
int rl_command(c)
int c;
{
  short want;
  if (c == '\n' || c == '\r') c = cmd_menu();
  if (Current_Environment == E_COUNTRYSIDE) return c;
  if (c == 'X') {
    start('X');
    return (c = rl_auto()) ? c : ' ';
  }
  if (c == '<' || c == '>') {
    want = c == '<' ? STAIRS_UP : STAIRS_DOWN;
    if (Level->site[Player.x][Player.y].locchar == want) return c;
    start(c);
    if (first_step() < 0) { mode = 0; return c; }  /* "Not here!" as before */
    return (c = rl_auto()) ? c : ' ';
  }
  return c;
}

/* the item menu of the full-screen inventory (Enter on a slot): every
   action that fits, with its usual key. Returns an inventory key (l, e, p,
   d, t) or ESCAPE, or 0 when it queued a game command (eat, quaff ...)
   with the slot letter: the caller leaves the inventory, the command runs
   with its own prompts, then the inventory reopens. */
static char *it_name[16], *it_cmd[16];
static int it_key[16], it_n;
static void item(name, key, cmd)
char *name, *cmd;
int key;
{
  it_name[it_n] = name; it_key[it_n] = key; it_cmd[it_n++] = cmd;
}

int rl_item_menu(slot)
int slot;
{
  pob o = Player.possessions[slot];
  int i, sel = 0, w = 0, c, oc;
  char *q;
  WINDOW *save, *win;

  if (o == NULL) return 't';                        /* empty: take from pack */
  oc = o->objchar;
  it_n = 0;
  /* game commands only with empty hands ('up in air' would be dropped) */
  if (slot > 0 && Player.possessions[O_UP_IN_AIR] == NULL) {
    if (oc == FOOD || oc == CORPSE) item("eat", 'E', "e");
    if (oc == POTION) item("quaff", 'q', "q");
    if (oc == SCROLL) item("read", 'r', "r");
    if (oc == STICK) item("zap", 'a', "a");
    if (oc == THING) item("activate", 'A', "Ai");
    if (oc == ARTIFACT) item("activate", 'A', "Aa");
    if (oc == WEAPON || oc == MISSILEWEAPON) item("fire/throw", 'f', "f");
    item("call it something", 'C', "C");
  }
  item("look at it", 'l', NULL);
  if (slot > 0) item("exchange with up-in-air", 'e', NULL);
  item("put in pack", 'p', NULL);
  item("drop", 'd', NULL);
  for (i = 0; i < it_n; i++) if ((int)strlen(it_name[i]) + 3 > w) w = strlen(it_name[i]) + 3;
  save = dupwin(curscr);
  win = newwin(it_n + 2, w + 4, 4, 30);
  for (;;) {
    werase(win);
    for (i = 0; i < w + 4; i++) { mvwaddch(win, 0, i, '-'); mvwaddch(win, it_n + 1, i, '-'); }
    for (i = 0; i < it_n; i++) {
      mvwaddch(win, i + 1, 0, '|');
      mvwaddch(win, i + 1, w + 3, '|');
      if (i == sel) wstandout(win);
      mvwprintw(win, i + 1, 2, "%c  %-*s", it_key[i] == 'E' ? 'e' : it_key[i], w - 3, it_name[i]);
      wstandend(win);
    }
    wmove(win, sel + 1, 2);
    c = wgetch(win);
    if (c == 'j' || c == '2') sel = (sel + 1) % it_n;
    else if (c == 'k' || c == '8') sel = (sel + it_n - 1) % it_n;
    else if (c == '\n' || c == '\r' || c == '5' || c == ' ' || c == '6') break;
    else if (c == ESCAPE || c == '0' || c == '.' || c == '4') { sel = -1; break; }
    else {
      for (i = 0; i < it_n && it_key[i] != c; i++) ;
      if (i < it_n) { sel = i; break; }
    }
  }
  delwin(win);
  touchwin(save);
  wrefresh(save);
  delwin(save);
  if (sel < 0) return ESCAPE;
  if (!it_cmd[sel]) return it_key[sel];
  for (q = it_cmd[sel]; *q; q++) wc_push(*q);
  wc_push('a' + slot - 1);   /* getitem()'s letter for this slot */
  Rl_reopen = 2;
  return 0;
}

/* web: save to omega.sav without messages or prompts, and keep playing */
void rl_autosave()
{
  int quiet = gamestatusp(SUPPRESS_PRINTING);
  char tmp[20];
  if (Player.hp <= 0 || gamestatusp(ARENA_MODE) || Current_Environment == E_ABYSS ||
      Current_Environment == E_TACTICAL_MAP) return;
  strcpy(tmp, "omega.tmp");
  unlink(tmp);
  setgamestatus(SUPPRESS_PRINTING);
  if (save_game(FALSE, tmp)) rename(tmp, "omega.sav");
  if (!quiet) resetgamestatus(SUPPRESS_PRINTING);
}

/* a list of choices in a box at the right, under the message lines (the
   prompt's text) and clear of the menu window on the left (what it lists:
   pack, wares, a sequence). Each entry shows its key: keys[i], or when keys
   is NULL the letters a) b) ... (A) B) ... after z). The player presses a
   key, or moves with the arrows / 8 2 (9 3 a page) and takes the entry with
   Enter, space or 5. A key that is also a movement key (digits) wins over
   the movement. title may have several lines. Returns the index, -1 on
   ESCAPE. */
static int must;                /* rl_ask(): no ESCAPE, don't offer it */

static char *keyname(k)
int k;
{
  static char b[2];
  if (k == '\n') return "Enter";
  if (k == '\b') return "Bksp";
  if (k == ESCAPE) return "Esc";
  if (k == ' ') return "Space";
  b[0] = k; b[1] = 0;
  return b;
}

static int lower(c)
int c;
{
  return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

int rl_choose_keys(title, items, keys, n, sel)
char *title, **items;
int *keys, n, sel;
{
  char tl[8][80], *p;
  int nt = 0, w, h, top = 0, i, c, len, y0, x0, ty, lw = 1, fold = 1;
  int k[128];
  WINDOW *save, *win;

  if (n <= 0) return -1;
  n = min(n, 128);
  for (i = 0; i < n; i++) {
    k[i] = keys ? keys[i] : i < 26 ? 'a' + i : i < 52 ? 'A' + i - 26 : 0;
    if (k[i]) lw = max(lw, (int)strlen(keyname(k[i])));
  }
  /* letters in either case, unless the keys tell the cases apart */
  for (i = 0; i < n && fold; i++)
    for (c = i + 1; c < n; c++)
      if (k[i] != k[c] && lower(k[i]) == lower(k[c])) { fold = 0; break; }
  for (p = title; p && *p && nt < 7; nt++) {
    for (len = 0; p[len] && p[len] != '\n'; len++) ;
    sprintf(tl[nt], "%.*s", len > 79 ? 79 : len, p);
    p += len;
    if (*p) p++;
  }
  w = 20;
  for (i = 0; i < nt; i++) w = max(w, (int)strlen(tl[i]));
  for (i = 0; i < n; i++) w = max(w, (int)strlen(items[i]) + lw + 2);
  w = min(w, COLS - 4);
  ty = nt ? nt + 1 : 0;                         /* title lines + a rule */
  h = min(n, LINES - 5 - ty);
  y0 = 3; x0 = COLS - w - 4;
  if (sel < 0 || sel >= n) sel = 0;
  save = dupwin(curscr);
  win = newwin(h + ty + 2, w + 4, y0, x0);
  for (;;) {
    if (sel < top) top = sel;
    if (sel >= top + h) top = sel - h + 1;
    werase(win);
    for (i = 0; i < w + 4; i++) {
      mvwaddch(win, 0, i, '-');
      mvwaddch(win, h + ty + 1, i, '-');
      if (nt) mvwaddch(win, nt + 1, i, '-');
    }
    for (i = 1; i <= h + ty; i++) {
      mvwaddch(win, i, 0, '|');
      mvwaddch(win, i, w + 3, '|');
    }
    for (i = 0; i < nt; i++) mvwprintw(win, i + 1, 2, "%.*s", w, tl[i]);
    for (i = 0; i < h; i++) {
      int e = top + i;
      if (e == sel) wstandout(win);
      if (k[e]) mvwprintw(win, ty + i + 1, 2, "%*s) %-*.*s", lw, keyname(k[e]),
                          w - lw - 2, w - lw - 2, items[e]);
      else mvwprintw(win, ty + i + 1, 2, "%*s  %-*.*s", lw, "",
                     w - lw - 2, w - lw - 2, items[e]);
      wstandend(win);
    }
    if (top) mvwaddstr(win, ty, w - 3, " more ");
    if (top + h < n) mvwaddstr(win, h + ty + 1, w - 3, " more ");
    sprintf(tl[7], must ? " key or arrows + Enter " : " key or arrows + Enter, Esc ");
    if ((int)strlen(tl[7]) <= w - 6) mvwaddstr(win, h + ty + 1, 2, tl[7]);
    wmove(win, sel - top + ty + 1, 2);
    c = wgetch(win);
    /* an entry's key (Enter always takes the highlighted entry) */
    for (i = 0; i < n && c != '\n' && c != '\r'; i++)
      if (k[i] && (c == k[i] || (fold && lower(c) == lower(k[i])))) break;
    if (i < n && c != '\n' && c != '\r') { sel = i; break; }
    if (c == KEY_DOWN || c == '2') sel = (sel + 1) % n;
    else if (c == KEY_UP || c == '8') sel = (sel + n - 1) % n;
    else if (c == '3') sel = min(n - 1, sel + h);
    else if (c == '9') sel = max(0, sel - h);
    else if (c == '\n' || c == '\r' || c == ' ' || c == '5') break;
    else if (c == ESCAPE && !must) { sel = -1; break; }
  }
  delwin(win);
  touchwin(save);
  wrefresh(save);
  delwin(save);
  return sel;
}

/* the same, lettered a) b) c) ... */
int rl_choose(title, items, n, sel)
char *title, **items;
int n, sel;
{
  return rl_choose_keys(title, items, NULL, n, sel);
}

/* rl_choose_keys() over "k:text|k:text|...", each entry with the key the
   old prompt took (shown as "k) text"); returns the key of the entry taken,
   ESCAPE for none. An entry with the key \n (Enter) starts highlighted, so
   Enter alone still does what it did. */
int rl_menu(title, spec)
char *title, *spec;
{
  static char buf[1024];
  char *items[52], *p;
  int keys[52], n = 0, sel = 0;

  strncpy(buf, spec, sizeof buf - 1);
  for (p = strtok(buf, "|"); p && n < 52; p = strtok(NULL, "|")) {
    if (p[0] == '\n') sel = n;
    keys[n] = (unsigned char)p[0];
    items[n++] = p[1] == ':' ? p + 2 : p + 1;
  }
  n = rl_choose_keys(title, items, keys, n, sel);
  return n < 0 ? ESCAPE : keys[n];
}

/* the same, for a question that must be answered (no ESCAPE) */
int rl_ask(title, spec)
char *title, *spec;
{
  int c;
  must = 1;
  c = rl_menu(title, spec);
  must = 0;
  return c;
}
