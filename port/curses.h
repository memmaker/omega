/* Minimal in-memory curses for Omega (port of ~/Games/xrogue/port): every
 * window is composited onto one screen that a frontend draws as text
 * (be_web.c in the browser). Only what Omega rebirth
 * (C++23) uses; wcurses.c is C, this header is C and C++.
 * Window cell = char | colour pair (bits 8-15) | A_STANDOUT/A_BOLD/A_UNDERLINE.
 * Screen cell (curscr, be_put/be_pput) = char | PC fg (bits 8-11) | PC bg
 * (bits 12-14) | A_STANDOUT | A_TILE | tile << 18: wrefresh() folds the pair
 * and A_BOLD into the 16 PC colours the frontends draw. */
#ifndef WCURSES_H
#define WCURSES_H
#define OMEGA_SHIM 1   /* the game's web hooks (#ifdef OMEGA_SHIM) */
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int chtype;
typedef unsigned int attr_t;
typedef unsigned long mmask_t;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define ERR (-1)
#define OK 0
#define A_NORMAL 0u
#define A_CHARTEXT 0xffu
#define A_COLOR 0xff00u       /* window cells: pair; screen cells: PC fg | bg << 4 */
#define A_STANDOUT 0x10000u
#define A_REVERSE A_STANDOUT
#define A_TILE 0x20000u       /* screen cells only: map window cell; bits 18+ = wc_tile() */
#define A_BOLD 0x40000u       /* window cells only (folded into the colour) */
#define A_UNDERLINE 0x80000u  /* window cells only (not drawn) */
#define WA_NORMAL A_NORMAL
#define WA_REVERSE A_REVERSE
#define WA_STANDOUT A_STANDOUT
#define WA_BOLD A_BOLD
#define WA_UNDERLINE A_UNDERLINE
#define COLOR_PAIR(n) ((chtype)((n) & 0xff) << 8)
#define PAIR_NUMBER(a) ((int)(((a) & A_COLOR) >> 8))
int wc_tile(int c, const int *nb, int y, int x); /* map cell (its 8 neighbours or NULL, window y/x) -> tile (port/tiles.cpp) */
extern int Wc_tileset;         /* 0 text, 1 WinOmega, 2 gromega (port/tiles.cpp) */
void wc_retile(void);          /* the page switched sets: new tiles for the shown map */
chtype wc_fold(chtype v);     /* window cell -> screen cell */

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

typedef struct _win {
    int maxy, maxx, begy, begx, cury, curx;
    chtype attr;
    int clear, dirty, tiles, pane, delay;
    chtype *c;
    unsigned char *own;   /* dupwin(curscr): screen cells as they are, with their panes */
    int *lo, *hi;         /* per row: changed columns [lo, hi) since the last refresh */
} WINDOW;

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS, COLORS, COLOR_PAIRS;

#define KEY_DOWN      0402
#define KEY_UP        0403
#define KEY_LEFT      0404
#define KEY_RIGHT     0405
#define KEY_HOME      0406
#define KEY_BACKSPACE 0407
#define KEY_DC        0512
#define KEY_ENTER     0527
#define KEY_LL        0533
#define KEY_A1        0534
#define KEY_A3        0535
#define KEY_B2        0536
#define KEY_C1        0537
#define KEY_C3        0540
#define KEY_MOUSE     0631
#define KEY_RESIZE    0632   /* never sent: the shim's screen has a fixed size */

/* mouse: no-op (getmouse always fails), the constants only have to exist */
typedef struct { short id; int x, y, z; mmask_t bstate; } MEVENT;
#define WC_MB(b, e) ((mmask_t)1 << (((b) - 1) * 5 + (e)))
#define BUTTON1_RELEASED WC_MB(1, 0)
#define BUTTON1_PRESSED WC_MB(1, 1)
#define BUTTON1_CLICKED WC_MB(1, 2)
#define BUTTON1_DOUBLE_CLICKED WC_MB(1, 3)
#define BUTTON1_TRIPLE_CLICKED WC_MB(1, 4)
#define BUTTON2_RELEASED WC_MB(2, 0)
#define BUTTON2_PRESSED WC_MB(2, 1)
#define BUTTON2_CLICKED WC_MB(2, 2)
#define BUTTON2_DOUBLE_CLICKED WC_MB(2, 3)
#define BUTTON2_TRIPLE_CLICKED WC_MB(2, 4)
#define BUTTON3_RELEASED WC_MB(3, 0)
#define BUTTON3_PRESSED WC_MB(3, 1)
#define BUTTON3_CLICKED WC_MB(3, 2)
#define BUTTON3_DOUBLE_CLICKED WC_MB(3, 3)
#define BUTTON3_TRIPLE_CLICKED WC_MB(3, 4)
#define BUTTON4_RELEASED WC_MB(4, 0)
#define BUTTON4_PRESSED WC_MB(4, 1)
#define BUTTON4_CLICKED WC_MB(4, 2)
#define BUTTON4_DOUBLE_CLICKED WC_MB(4, 3)
#define BUTTON4_TRIPLE_CLICKED WC_MB(4, 4)
#define BUTTON5_RELEASED WC_MB(5, 0)
#define BUTTON5_PRESSED WC_MB(5, 1)
#define BUTTON5_CLICKED WC_MB(5, 2)
#define BUTTON5_DOUBLE_CLICKED WC_MB(5, 3)
#define BUTTON5_TRIPLE_CLICKED WC_MB(5, 4)
#define ALL_MOUSE_EVENTS (WC_MB(6, 0) - 1)

WINDOW *initscr(void);
int endwin(void);
WINDOW *newwin(int, int, int, int);
int delwin(WINDOW *);
WINDOW *dupwin(WINDOW *);
int mvwin(WINDOW *, int, int);
int wresize(WINDOW *, int, int);
int wmove(WINDOW *, int, int);
int waddch(WINDOW *, chtype);
int waddstr(WINDOW *, const char *);
int wprintw(WINDOW *, const char *, ...);
int printw(const char *, ...);
int mvwprintw(WINDOW *, int, int, const char *, ...);
chtype winch(WINDOW *);
int wclear(WINDOW *);
int werase(WINDOW *);
int wclrtoeol(WINDOW *);
int touchwin(WINDOW *);
int wnoutrefresh(WINDOW *);
int doupdate(void);
int wrefresh(WINDOW *);
int wgetch(WINDOW *);
int ungetch(int);
void wtimeout(WINDOW *, int);
int wattrset(WINDOW *, int);
int wattr_on(WINDOW *, attr_t, void *);
int wattr_off(WINDOW *, attr_t, void *);
int wcolor_set(WINDOW *, short, void *);
int start_color(void);
int init_pair(short, short, short);
int init_color(short, short, short, short);
int curs_set(int);
int wc_kbhit(void);
void wc_push(int key);         /* queue a key for wgetch */
int wc_usleep(unsigned int us);   /* -Dusleep=wc_usleep */

#ifdef __cplusplus
}
#define WC_INLINE inline
#else
#define WC_INLINE static inline
#endif

/* functions, not macros: C++ code has v.clear(), std::move, s.erase() */
#define getyx(w, y, x) ((y) = (w)->cury, (x) = (w)->curx)
#define getmaxyx(w, y, x) ((y) = (w)->maxy, (x) = (w)->maxx)
#define getbegyx(w, y, x) ((y) = (w)->begy, (x) = (w)->begx)
WC_INLINE int getcury(const WINDOW *w) { return w->cury; }
WC_INLINE int getcurx(const WINDOW *w) { return w->curx; }
WC_INLINE int getmaxy(const WINDOW *w) { return w->maxy; }
WC_INLINE int getmaxx(const WINDOW *w) { return w->maxx; }
WC_INLINE int mvwaddch(WINDOW *w, int y, int x, chtype ch) { return wmove(w, y, x) == ERR ? ERR : waddch(w, ch); }
WC_INLINE int mvwaddstr(WINDOW *w, int y, int x, const char *s) { return wmove(w, y, x) == ERR ? ERR : waddstr(w, s); }
WC_INLINE chtype mvwinch(WINDOW *w, int y, int x) { return wmove(w, y, x) == ERR ? (chtype)ERR : winch(w); }
WC_INLINE int mvwgetch(WINDOW *w, int y, int x) { return wmove(w, y, x) == ERR ? ERR : wgetch(w); }
WC_INLINE int move(int y, int x) { return wmove(stdscr, y, x); }
WC_INLINE int addch(chtype ch) { return waddch(stdscr, ch); }
WC_INLINE int addstr(const char *s) { return waddstr(stdscr, s); }
WC_INLINE int mvaddch(int y, int x, chtype ch) { return mvwaddch(stdscr, y, x, ch); }
WC_INLINE int mvaddstr(int y, int x, const char *s) { return mvwaddstr(stdscr, y, x, s); }
WC_INLINE int clear(void) { return wclear(stdscr); }
WC_INLINE int erase(void) { return werase(stdscr); }
WC_INLINE int clrtoeol(void) { return wclrtoeol(stdscr); }
WC_INLINE int refresh(void) { return wrefresh(stdscr); }
WC_INLINE int getch(void) { return wgetch(stdscr); }
WC_INLINE void timeout(int d) { wtimeout(stdscr, d); }
WC_INLINE int wstandout(WINDOW *w) { return wattr_on(w, A_STANDOUT, 0); }
WC_INLINE int wstandend(WINDOW *w) { return wattrset(w, A_NORMAL); }
WC_INLINE int standout(void) { return wstandout(stdscr); }
WC_INLINE int standend(void) { return wstandend(stdscr); }
WC_INLINE int attron(int a) { return wattr_on(stdscr, (attr_t)a, 0); }
WC_INLINE int attroff(int a) { return wattr_off(stdscr, (attr_t)a, 0); }
WC_INLINE int attrset(int a) { return wattrset(stdscr, a); }
WC_INLINE int wattron(WINDOW *w, int a) { return wattr_on(w, (attr_t)a, 0); }
WC_INLINE int wattroff(WINDOW *w, int a) { return wattr_off(w, (attr_t)a, 0); }
WC_INLINE int nodelay(WINDOW *w, int b) { wtimeout(w, b ? 0 : -1); return OK; }
WC_INLINE int scrollok(WINDOW *w, int b) { (void)w; (void)b; return OK; }
WC_INLINE int clearok(WINDOW *w, int b) { w->clear = b; return OK; }
WC_INLINE int leaveok(WINDOW *w, int b) { (void)w; (void)b; return OK; }
WC_INLINE int keypad(WINDOW *w, int b) { (void)w; (void)b; return OK; }
WC_INLINE int noecho(void) { return OK; }
WC_INLINE int echo(void) { return OK; }
WC_INLINE int crmode(void) { return OK; }
WC_INLINE int cbreak(void) { return OK; }
WC_INLINE int nocrmode(void) { return OK; }
WC_INLINE int nocbreak(void) { return OK; }
WC_INLINE int raw(void) { return OK; }
WC_INLINE int noraw(void) { return OK; }
WC_INLINE int nl(void) { return OK; }
WC_INLINE int nonl(void) { return OK; }
WC_INLINE int flushinp(void) { return OK; }
WC_INLINE int beep(void) { return OK; }
WC_INLINE int flash(void) { return OK; }
WC_INLINE int has_colors(void) { return TRUE; }
WC_INLINE int can_change_color(void) { return FALSE; }   /* frontends own the palette */
WC_INLINE int use_default_colors(void) { return OK; }
WC_INLINE int set_escdelay(int d) { (void)d; return OK; }
WC_INLINE mmask_t mousemask(mmask_t m, mmask_t *old) { (void)m; if (old) *old = 0; return 0; }
WC_INLINE int mouseinterval(int i) { (void)i; return 0; }
WC_INLINE int getmouse(MEVENT *e) { (void)e; return ERR; }
WC_INLINE int ungetmouse(MEVENT *e) { (void)e; return ERR; }

#ifdef __cplusplus
extern "C" {
#endif
/* panes (page windows): the game names its windows with wc_pane(); a pane's
 * rect is the union of its windows. Other windows only reach the whole
 * screen (curscr); when one of them covers the map, the screen is a pop-up. */
enum { WC_FULL, WC_MAP, WC_SIDE, WC_STAT, WC_MSG, WC_PANES };
void wc_pane(WINDOW *w, int pane);

/* frontend: the whole screen, plus each pane */
void be_init(int cols, int rows);
void be_put(int y, int x, chtype ch);
void be_pane(int pane, int y, int x, int rows, int cols);  /* pane rect on the screen */
void be_pput(int pane, int y, int x, chtype ch);          /* cell inside the pane */
void be_popup(int on, int y, int x, int rows, int cols);  /* a window over the panes: its box */
void be_msg(const char *s, int append);
void be_cursor(int y, int x);
void be_hero(int y, int x);   /* player's screen cell: the map camera centres on it (RVIP.md W4) */
void be_flush(void);
int  be_getkey(int wait);   /* -1 when !wait and nothing queued */
void be_sleep(int ms);
void be_end(void);
#ifdef __cplusplus
}
#endif
#endif
