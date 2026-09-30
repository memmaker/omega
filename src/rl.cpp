// rl.cpp -- added for the ~/Games port (RVIP): auto-explore (X), walking to
// known stairs (< >), the command menu on Enter, choice menus (rl_choose,
// rl_menu, rl_ask), the item menu of the inventory; for the web build also
// the message log, the autosave, the Inventory/Visible lists and the run report.

#include "glob.h"
#include "scr.h"
#include "scrolling_buffer.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

extern WINDOW *level_window, *message_window;
extern scrolling_buffer message_buffer;
std::string get_username();

int Rl_reopen    = 0; // reopen the inventory after this command
int Rl_at_prompt = 0; // waiting for a command key (web: prompt line, autosave)
int Rl_saved     = 0; // the player saved with S (keep the file)

static int mode = 0;                         // 0, 'X' exploring, '<' / '>' walking to stairs
static size_t msgs;                          // message count when the walk started
static level *lvl;                           // the level visited[] belongs to
static int lvl_env;
static char visited[MAXWIDTH][MAXLENGTH];
static const char vi_key[9] = "nubylhjk"; // Dirs[] index -> key, see getdir()

// every message line so far (the history drops old ones at the front)
static size_t message_total()
{
  return message_buffer.dropped() + message_buffer.get_message_history().size();
}

// queue keys for the game to read next, in this order
static void push_keys(const std::string &keys)
{
  for(size_t i = keys.size(); i-- > 0;)
  {
    ungetch(static_cast<unsigned char>(keys[i]));
  }
}

// a key is waiting (a walk stops on any key); it stays queued
static bool key_waiting()
{
  wtimeout(level_window, 0);
  int c = wgetch(level_window);
  wtimeout(level_window, -1);
  if(c == ERR)
  {
    return false;
  }
  ungetch(c);
  return true;
}

static void check_level()
{
  if(Level != lvl || Current_Environment != lvl_env)
  {
    memset(visited, 0, sizeof visited);
    lvl     = Level;
    lvl_env = Current_Environment;
  }
}

// harmful or impassable for the explorer
static bool blocked(int x, int y)
{
  location &l = Level->site[x][y];
  chtype c    = l.locchar;
  if(!loc_statusp(x, y, SEEN, *Level) || loc_statusp(x, y, SECRET, *Level))
  {
    return true;
  }
  if(visited[x][y] == 2)
  {
    return true; // locked door
  }
  // a site that runs a function (shop door, altar, trap text ...): once
  if(visited[x][y] && l.p_locf != L_NO_OP)
  {
    return true;
  }
  return c == WALL || c == STATUE || c == PORTCULLIS || c == HEDGE || c == LAVA || c == ABYSS ||
         c == VOID_CHAR || c == FIRE || c == WHIRLWIND || c == WATER || c == LIFT || c == TRAP ||
         c == RUBBLE || l.creature;
}

static bool is_target(int x, int y)
{
  if(mode == '<')
  {
    return Level->site[x][y].locchar == STAIRS_UP;
  }
  if(mode == '>')
  {
    return Level->site[x][y].locchar == STAIRS_DOWN;
  }
  if(visited[x][y])
  {
    return false;
  }
  if(!Level->site[x][y].things.empty())
  {
    return true;
  }
  for(int d = 0; d < 8; ++d)
  {
    int nx = x + Dirs[0][d], ny = y + Dirs[1][d];
    if(inbounds(nx, ny) && !loc_statusp(nx, ny, SEEN, *Level))
    {
      return true;
    }
  }
  return false;
}

// BFS over known ground: the Dirs[] index of the first step to the nearest target, or -1
static int first_step()
{
  static short qx[MAXWIDTH * MAXLENGTH], qy[MAXWIDTH * MAXLENGTH];
  static signed char from[MAXWIDTH][MAXLENGTH];
  int h = 0, t = 0;
  memset(from, -1, sizeof from);
  from[Player.x][Player.y] = 8;
  qx[t]                    = Player.x;
  qy[t++]                  = Player.y;
  while(h < t)
  {
    int x = qx[h], y = qy[h++];
    if(h > 1 && is_target(x, y))
    {
      // walk back to the step next to the player
      while(from[x][y] != 8)
      {
        int d = from[x][y];
        if(x - Dirs[0][d] == Player.x && y - Dirs[1][d] == Player.y)
        {
          return d;
        }
        x -= Dirs[0][d];
        y -= Dirs[1][d];
      }
    }
    for(int d = 0; d < 8; ++d)
    {
      int nx = x + Dirs[0][d], ny = y + Dirs[1][d];
      if(!inbounds(nx, ny) || from[nx][ny] >= 0)
      {
        continue;
      }
      if(blocked(nx, ny) && !(mode != 'X' && is_target(nx, ny) && !Level->site[nx][ny].creature))
      {
        continue;
      }
      from[nx][ny] = d;
      qx[t]        = nx;
      qy[t++]      = ny;
    }
  }
  return -1;
}

// a monster in view; in town only hostile ones (townsfolk are everywhere)
static bool monster_in_view()
{
  bool town = Current_Environment == E_CITY || Current_Environment == E_VILLAGE;
  for(auto &m : Level->mlist)
  {
    if(m->hp <= 0 || !view_los_p(Player.x, Player.y, m->x, m->y))
    {
      continue;
    }
    if(m_statusp(*m, M_INVISIBLE) && !Player.status[TRUESIGHT])
    {
      continue;
    }
    if(town && !m_statusp(*m, HOSTILE))
    {
      continue;
    }
    return true;
  }
  return false;
}

static void stop(const char *s)
{
  mode = 0;
  if(s)
  {
    queue_message(s);
  }
}

// next key for an ongoing walk, 0 = none (the player types)
int rl_auto()
{
  if(Current_Environment == E_COUNTRYSIDE)
  {
    mode = 0;
  }
  if(Rl_reopen == 2)
  {
    Rl_reopen = 1; // the queued command runs first
  }
  else if(Rl_reopen)
  {
    Rl_reopen = 0;
    if(!monster_in_view())
    {
      return 'i';
    }
  }
  if(!mode)
  {
    return 0;
  }
  check_level();
  visited[Player.x][Player.y] |= 1;
  if(message_total() != msgs)
  {
    stop(nullptr);
    return 0;
  }
  if(key_waiting())
  {
    stop(nullptr);
    return 0;
  }
  if(monster_in_view())
  {
    stop("You see a monster.");
    return 0;
  }
  if(mode != 'X' && Level->site[Player.x][Player.y].locchar == (mode == '<' ? STAIRS_UP : STAIRS_DOWN))
  {
    mode = 0; // arrived: the player presses < / > again
    return 0;
  }
  int d = first_step();
  if(d < 0)
  {
    stop(mode == 'X' ? "Nothing left to explore." : "No known way there.");
    return 0;
  }
  int nx = Player.x + Dirs[0][d], ny = Player.y + Dirs[1][d];
  if(Level->site[nx][ny].locchar == CLOSED_DOOR)
  {
    if(Level->site[nx][ny].aux == LOCKED)
    {
      visited[nx][ny] = 2;
      stop("That door seems to be locked.");
      return 0;
    }
    push_keys(std::string(1, vi_key[d])); // opendoor() asks for the direction
    return 'o';
  }
  return vi_key[d];
}

static void start(int m)
{
  mode = m;
  msgs = message_total();
}

// ---------- choice menus ----------

// rl_ask(): no ESCAPE, don't offer it
static bool must;

static std::string key_label(int k)
{
  if(k == '\n')
  {
    return "Enter";
  }
  if(k == '\b')
  {
    return "Bksp";
  }
  if(k == ESCAPE)
  {
    return "Esc";
  }
  if(k == ' ')
  {
    return "Space";
  }
  if(k > 0 && k < ' ')
  {
    return std::string{'^', static_cast<char>(k + '@')};
  }
  return std::string(1, static_cast<char>(k));
}

static int lower(int c)
{
  return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

// A list of choices in a box at the right (clear of the menu window on the
// left: what it lists, pack, wares, a sequence). Each entry shows its key:
// keys[i], or with no keys the letters a) b) ... (A) B) ... after z). The
// player presses a key, or moves with the arrows / 8 2 (9 3 a page) and takes
// the entry with Enter, space or 5. A key that is also a movement key
// (digits) wins over the movement. title may have several lines. Returns the
// index, -1 on ESCAPE.
int rl_choose_keys(const std::string &title, const std::vector<std::string> &items, const std::vector<int> &keys, int sel)
{
  int n = std::min(static_cast<int>(items.size()), 128);
  if(n <= 0)
  {
    return -1;
  }
  std::vector<int> k(n);
  int lw = 1;
  for(int i = 0; i < n; ++i)
  {
    k[i] = !keys.empty() ? keys[i] : i < 26 ? 'a' + i : i < 52 ? 'A' + i - 26 : 0;
    if(k[i])
    {
      lw = std::max(lw, static_cast<int>(key_label(k[i]).size()));
    }
  }
  // letters in either case, unless the keys tell the cases apart
  bool fold = true;
  for(int i = 0; i < n && fold; ++i)
  {
    for(int j = i + 1; j < n; ++j)
    {
      if(k[i] != k[j] && lower(k[i]) == lower(k[j]))
      {
        fold = false;
        break;
      }
    }
  }
  std::vector<std::string> tl;
  for(size_t p = 0; p < title.size() && tl.size() < 7;)
  {
    size_t e = title.find('\n', p);
    if(e == std::string::npos)
    {
      e = title.size();
    }
    tl.push_back(title.substr(p, std::min<size_t>(e - p, 79)));
    p = e + 1;
  }
  int w = 20;
  for(auto &l : tl)
  {
    w = std::max(w, static_cast<int>(l.size()));
  }
  for(int i = 0; i < n; ++i)
  {
    w = std::max(w, static_cast<int>(items[i].size()) + lw + 2);
  }
  w      = std::min(w, COLS - 4);
  int nt = static_cast<int>(tl.size());
  int ty = nt ? nt + 1 : 0; // title lines + a rule
  int h  = std::min(n, LINES - 5 - ty);
  int y0 = 1, x0 = COLS - w - 4;
  if(sel < 0 || sel >= n)
  {
    sel = 0;
  }
  print_messages(); // what led to the question
  WINDOW *save = dupwin(curscr);
  WINDOW *win  = newwin(h + ty + 2, w + 4, y0, x0);
  int top      = 0;
  std::string hint = must ? " key or arrows + Enter " : " key or arrows + Enter, Esc ";
  for(;;)
  {
    if(sel < top)
    {
      top = sel;
    }
    if(sel >= top + h)
    {
      top = sel - h + 1;
    }
    werase(win);
    for(int i = 0; i < w + 4; ++i)
    {
      mvwaddch(win, 0, i, '-');
      mvwaddch(win, h + ty + 1, i, '-');
      if(nt)
      {
        mvwaddch(win, nt + 1, i, '-');
      }
    }
    for(int i = 1; i <= h + ty; ++i)
    {
      mvwaddch(win, i, 0, '|');
      mvwaddch(win, i, w + 3, '|');
    }
    for(int i = 0; i < nt; ++i)
    {
      mvwaddstr(win, i + 1, 2, tl[i].substr(0, w).c_str());
    }
    for(int i = 0; i < h; ++i)
    {
      int e = top + i;
      if(e == sel)
      {
        wstandout(win);
      }
      std::string text = items[e].substr(0, w - lw - 2);
      std::string line = k[e] ? std::format("{:>{}}) {:<{}}", key_label(k[e]), lw, text, w - lw - 2)
                              : std::format("{:>{}}  {:<{}}", "", lw, text, w - lw - 2);
      mvwaddstr(win, ty + i + 1, 2, line.c_str());
      wstandend(win);
    }
    if(top)
    {
      mvwaddstr(win, ty, w - 3, " more ");
    }
    if(top + h < n)
    {
      mvwaddstr(win, h + ty + 1, w - 3, " more ");
    }
    if(static_cast<int>(hint.size()) <= w - 6)
    {
      mvwaddstr(win, h + ty + 1, 2, hint.c_str());
    }
    wmove(win, sel - top + ty + 1, 2);
    wnoutrefresh(win);
    doupdate();
    int c = wgetch(win);
    // an entry's key (Enter always takes the highlighted entry)
    int i = 0;
    for(; i < n && c != '\n' && c != '\r' && c != KEY_ENTER; ++i)
    {
      if(k[i] && (c == k[i] || (fold && lower(c) == lower(k[i]))))
      {
        break;
      }
    }
    if(i < n && c != '\n' && c != '\r' && c != KEY_ENTER)
    {
      sel = i;
      break;
    }
    if(c == KEY_DOWN || c == '2')
    {
      sel = (sel + 1) % n;
    }
    else if(c == KEY_UP || c == '8')
    {
      sel = (sel + n - 1) % n;
    }
    else if(c == '3')
    {
      sel = std::min(n - 1, sel + h);
    }
    else if(c == '9')
    {
      sel = std::max(0, sel - h);
    }
    else if(c == '\n' || c == '\r' || c == KEY_ENTER || c == ' ' || c == '5')
    {
      break;
    }
    else if(c == ESCAPE && !must)
    {
      sel = -1;
      break;
    }
  }
  delwin(win);
  touchwin(save);
  wrefresh(save);
  delwin(save);
  return sel;
}

// the same, lettered a) b) c) ...
int rl_choose(const std::string &title, const std::vector<std::string> &items, int sel)
{
  return rl_choose_keys(title, items, {}, sel);
}

// rl_choose_keys() over "k:text|k:text|...", each entry with the key the old
// prompt took (shown as "k) text"); returns the key of the entry taken,
// ESCAPE for none. An entry with the key \n (Enter) starts highlighted, so
// Enter alone still does what it did.
int rl_menu(const std::string &title, const std::string &spec)
{
  std::vector<std::string> items;
  std::vector<int> keys;
  int sel = 0;
  for(size_t p = 0; p < spec.size();)
  {
    size_t e = spec.find('|', p);
    if(e == std::string::npos)
    {
      e = spec.size();
    }
    std::string entry = spec.substr(p, e - p);
    p                 = e + 1;
    if(entry.empty())
    {
      continue;
    }
    if(entry[0] == '\n')
    {
      sel = static_cast<int>(keys.size());
    }
    keys.push_back(static_cast<unsigned char>(entry[0]));
    items.push_back(entry.size() > 1 && entry[1] == ':' ? entry.substr(2) : entry.substr(1));
  }
  int n = rl_choose_keys(title, items, keys, sel);
  return n < 0 ? ESCAPE : keys[n];
}

// the same, for a question that must be answered (no ESCAPE)
int rl_ask(const std::string &title, const std::string &spec)
{
  must  = true;
  int c = rl_menu(title, spec);
  must  = false;
  return c;
}

// ---------- the command menu (Enter), parsed from the command list help file ----------
static int cmd_menu()
{
  std::ifstream f(std::format("{}{}", Omegalib, Current_Environment == E_COUNTRYSIDE ? "help13.txt" : "help12.txt"));
  std::vector<std::string> items;
  std::vector<int> keys;
  std::string line;
  while(items.size() < 64 && std::getline(f, line))
  {
    // "key  : description   : time"
    size_t a = line.find(':'), b = line.rfind(':');
    if(a == std::string::npos || a == b)
    {
      continue;
    }
    size_t i = 0;
    while(i < line.size() && line[i] != ' ' && line[i] != ':')
    {
      ++i;
    }
    // one key or ^x; not "vi keys" or "Enter" (movement stays off the menu)
    if(!(i == 1 || (i == 2 && line[0] == '^')))
    {
      continue;
    }
    std::string text = line.substr(a + 1, b - a - 1);
    text.erase(0, text.find_first_not_of(' '));
    text.erase(text.find_last_not_of(' ') + 1);
    keys.push_back(i == 2 ? line[1] & 0x1f : line[0]);
    items.push_back(text.substr(0, 60));
  }
  if(items.empty())
  {
    return ' ';
  }
  int n = rl_choose_keys("Commands", items, keys, 0);
  xredraw();
  return n < 0 ? ' ' : keys[n];
}

// the command key the player (or a walk) gives, after the port's own keys
int rl_command(int c)
{
#ifdef OMEGA_SHIM
  void rl_mark_command();
  rl_mark_command();
#endif
  if(c == '\n' || c == '\r' || c == KEY_ENTER)
  {
    c = cmd_menu();
  }
  if(Current_Environment == E_COUNTRYSIDE)
  {
    return c;
  }
  if(c == 'X')
  {
    start('X');
    return (c = rl_auto()) ? c : ' ';
  }
  if(c == '<' || c == '>')
  {
    chtype want = c == '<' ? STAIRS_UP : STAIRS_DOWN;
    if(Level->site[Player.x][Player.y].locchar == want)
    {
      return c;
    }
    start(c);
    if(first_step() < 0)
    {
      mode = 0;
      return c; // "Not here!" as before
    }
    return (c = rl_auto()) ? c : ' ';
  }
  return c;
}

// The item menu of the inventory (Enter on a slot): every action that fits,
// with its usual key. Returns the inventory key of the slot (put in pack /
// take from pack), ESCAPE, or 0 when it queued a game command (eat, quaff
// ...) with the slot's key: the caller leaves the inventory, the command runs
// with its own prompts, then the inventory reopens.
int rl_item_menu(int slot)
{
  object *o = Player.possessions[slot].get();
  char key  = index_to_key(slot);
  if(!o)
  {
    return key; // empty: take from the pack
  }
  std::vector<std::string> names, cmds;
  std::vector<int> keys;
  auto item = [&](const char *name, int k, const char *cmd) {
    names.emplace_back(name);
    keys.push_back(k);
    cmds.emplace_back(cmd ? cmd : "");
  };
  chtype oc = o->objchar;
  if(oc == FOOD || oc == CORPSE)
  {
    item("eat", 'e', "e");
  }
  if(oc == POTION)
  {
    item("quaff", 'q', "q");
  }
  if(oc == SCROLL)
  {
    item("read", 'r', "r");
  }
  if(oc == STICK)
  {
    item("zap", 'a', "a");
  }
  if(oc == THING)
  {
    item("activate", 'A', "Ai");
  }
  if(oc == ARTIFACT)
  {
    item("activate", 'A', "Aa");
  }
  if(oc == WEAPON || oc == MISSILEWEAPON)
  {
    item("fire/throw", 'f', "f");
  }
  item("call it something", 'C', "C");
  item("put in pack", 'p', nullptr);
  int sel = rl_choose_keys(itemid(o), names, keys, 0);
  if(sel < 0)
  {
    return ESCAPE;
  }
  if(cmds[sel].empty())
  {
    return key;
  }
  push_keys(cmds[sel] + key); // getitem()'s key for this slot
  Rl_reopen = 2;
  return 0;
}

// ---------- web: message log, prompt line, autosave, lists, run report ----------
#ifdef OMEGA_SHIM
extern "C"
{
  void be_msg(const char *s, int append);
  void be_prompt(const char *s);
  void be_lists(const char *inv, const char *vis);
  void be_beacon(const char *g, const char *ev, const char *name, const char *killer, int depth, int score, int turns, int lvl);
  int wc_tile_of(chtype c);
  int wc_colour_of(chtype c);
}

static size_t cmd_mark; // message count when the last command key came

void rl_mark_command()
{
  cmd_mark = message_total();
}

// a history line without color_waddstr's markup (|x = colour x, || = |)
static std::string plain(const std::string &s)
{
  std::string p;
  for(size_t j = 0; j < s.size(); ++j)
  {
    if(s[j] == '|' && j + 1 < s.size())
    {
      if(s[++j] == '|')
      {
        p += '|';
      }
    }
    else
    {
      p += s[j];
    }
  }
  return p;
}

// a line to the log (append 2 = replace the last one). rebirth folds repeats
// within a batch itself ("text x3"); repeats of the line above are folded
// here with that count as "text (x5)", not "text x3 (x2)" (RvipWM.log's own)
static void log_line(const std::string &raw, int append)
{
  static std::string fbase;
  static int fcount;
  std::string s = plain(raw), base = s;
  int n = 1;
  size_t k = s.rfind(" x");
  if(k != std::string::npos && k + 2 < s.size() && s.find_first_not_of("0123456789", k + 2) == std::string::npos)
  {
    base = s.substr(0, k);
    n    = std::stoi(s.substr(k + 2));
  }
  if(append != 2 && !fbase.empty() && base == fbase)
  {
    fcount += n;
    be_msg(std::format("{} (x{})", base, fcount).c_str(), 2);
    return;
  }
  fbase  = base;
  fcount = n;
  be_msg(s.c_str(), append);
}

// the message history goes to the log window (new lines; the last one again
// when it grew or changed), the lines since the last command to the prompt line
void rl_messages()
{
  static size_t sent;
  static std::string last;
  const std::deque<std::string> &h = message_buffer.get_message_history(false);
  size_t base = message_buffer.dropped(), total = base + h.size();
  bool replace = false;
  if(sent > total)
  {
    sent    = total; // a line was taken back (--MORE--)
    replace = true;
  }
  if(sent > base && sent - 1 < total && h[sent - 1 - base] != last)
  {
    last = h[sent - 1 - base];
    log_line(last, 2);
  }
  for(; sent < total; ++sent)
  {
    last = h[sent - base];
    log_line(last, replace ? 2 : 0);
    replace = false;
  }
  std::string p;
  for(size_t i = std::max({cmd_mark, base, total > 3 ? total - 3 : 0}); i < total; ++i)
  {
    p += plain(h[i - base]);
    p += '\n';
  }
  be_prompt(p.c_str());
}

static std::string save_path()
{
  return std::format("{}saves/{}/{}.sav", Omegalib, get_username(), Player.name);
}

// save without messages or prompts, and keep playing
extern "C" void rl_autosave()
{
  if(Player.hp <= 0 || Player.name.empty() || gamestatusp(ARENA_MODE, GameStatus) ||
     Current_Environment == E_ABYSS || Current_Environment == E_TACTICAL_MAP)
  {
    return;
  }
  std::error_code ec;
  std::string file = save_path(), tmp = file + ".tmp";
  std::filesystem::create_directories(std::filesystem::path(file).parent_path(), ec);
  std::filesystem::remove(tmp, ec);
  bool quiet = gamestatusp(SUPPRESS_PRINTING, GameStatus);
  setgamestatus(SUPPRESS_PRINTING, GameStatus);
  if(save_game(tmp))
  {
    std::filesystem::rename(tmp, file, ec);
  }
  if(!quiet)
  {
    resetgamestatus(SUPPRESS_PRINTING, GameStatus);
  }
}

// exit(): the autosave goes unless the player saved with S (died or quit: the game is over)
extern "C" int rl_game_end()
{
  if(!Rl_saved && !Player.name.empty())
  {
    std::error_code ec;
    std::filesystem::remove(save_path(), ec);
  }
  return Rl_saved;
}

// Inventory and Visible windows (rvip-wm.js): lines
// "<colour>\t<glyph>\t<text>\t<tile>" for the inventory;
// "M<glyph><name>\t<colour>\t<tile>" / "I<glyph><name>\t<colour>\t<tile>" for
// what the player sees (colours: PC palette indexes; tile: slot in
// web/tiles.png, -1 none)
extern "C" void rl_send_lists()
{
  static const char *names[MAXITEMS] = {"up in air", "ready hand", "weapon hand", "left shoulder",
    "right shoulder", "belt", "belt", "belt", "shield", "armor", "boots", "cloak", "finger", "finger",
    "finger", "finger"};
  static std::string last_inv, last_vis;
  if(Player.maxhp <= 0 || !Level)
  {
    return;
  }
  std::string inv, vis;
  auto glyph = [](chtype c) { return std::string(1, static_cast<char>(c & 0xff)); };
  for(int i = 0; i < MAXITEMS; ++i)
  {
    if(object *o = Player.possessions[i].get())
    {
      inv += std::format("{}\t{}\t{} {:<14.14} {:.60}\t{}\n", wc_colour_of(o->objchar), glyph(o->objchar),
                         index_to_key(i), names[i], itemid(o), wc_tile_of(o->objchar) - 1);
    }
  }
  for(size_t i = Player.pack.size(); i-- > 0;)
  {
    object *o = Player.pack[i].get();
    inv += std::format("{}\t{}\tpack {}) {:>9} {:.60}\t{}\n", wc_colour_of(o->objchar), glyph(o->objchar),
                       static_cast<char>('a' + Player.pack.size() - 1 - i), "", itemid(o), wc_tile_of(o->objchar) - 1);
  }
  if(Current_Environment != E_COUNTRYSIDE && !Player.status[BLINDED])
  {
    for(auto &m : Level->mlist)
    {
      if(m->hp > 0 && view_los_p(Player.x, Player.y, m->x, m->y) &&
         (Player.status[TRUESIGHT] || !m_statusp(*m, M_INVISIBLE)))
      {
        vis += std::format("M{}{:.60}\t{}\t{}\n", glyph(m->monchar), m->monstring, wc_colour_of(m->monchar), wc_tile_of(m->monchar) - 1);
      }
    }
    for(int x = 0; x < WIDTH && x < MAXWIDTH; ++x)
    {
      for(int y = 0; y < LENGTH && y < MAXLENGTH && vis.size() < 8000; ++y)
      {
        if(!Level->site[x][y].things.empty() && view_los_p(Player.x, Player.y, x, y))
        {
          for(auto &o : Level->site[x][y].things)
          {
            vis += std::format("I{}{:.80}\t{}\t{}\n", glyph(o->objchar), itemid(o.get()), wc_colour_of(o->objchar), wc_tile_of(o->objchar) - 1);
          }
        }
      }
    }
  }
  if(inv != last_inv || vis != last_vis)
  {
    last_inv = inv;
    last_vis = vis;
    be_lists(inv.c_str(), vis.c_str());
  }
}

// run report: scr.cpp display_death/win/quit/bigwin; score as checkhigh() counts it
void rl_run_end(const char *ev, const std::string &killer)
{
  const char *k = killer.c_str();
  if(!strncmp(k, "a ", 2))
  {
    k += 2;
  }
  else if(!strncmp(k, "an ", 3))
  {
    k += 3;
  }
  else if(!strncasecmp(k, "the ", 4))
  {
    k += 4;
  }
  be_beacon("omega", ev, Player.name.c_str(), k, Level ? Level->depth : -1, FixedPoints > 0 ? FixedPoints : calc_points(),
            static_cast<int>(Time), Player.level);
}
#else
void rl_run_end(const char *, const std::string &) {}
#endif
