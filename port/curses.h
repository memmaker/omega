/* Minimal in-memory curses for Omega (port of ~/Games/xrogue/port): every
 * window is composited onto one screen that a frontend draws as text
 * (be_x11.c on the Mac, be_web.c in the browser). Only what Omega uses.
 * Cell = char | colour (bits 8-14, Omega's MSDOS COL_ values) | A_STANDOUT. */
#ifndef WCURSES_H
#define WCURSES_H
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

typedef unsigned int chtype;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define ERR (-1)
#define OK 0
#define A_CHARTEXT 0xff
#define A_COLOR 0x7f00
#define A_STANDOUT 0x10000
#define A_REVERSE A_STANDOUT
#define A_TILE 0x20000        /* map window cell; bits 18+ = wc_tile() */
int wc_tile(int c);

typedef struct _win {
    int maxy, maxx, begy, begx, cury, curx, attr;
    int clear, dirty, tiles, pane;
    chtype *c;
} WINDOW;

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS;

#define KEY_DOWN  0402
#define KEY_UP    0403
#define KEY_LEFT  0404
#define KEY_RIGHT 0405
#define KEY_HOME  0406
#define KEY_A1    0534
#define KEY_A3    0535
#define KEY_B2    0536
#define KEY_C1    0537
#define KEY_C3    0540
#define KEY_LL    0533

WINDOW *initscr(void);
int endwin(void);
WINDOW *newwin(int, int, int, int);
int wmove(WINDOW *, int, int);
int waddch(WINDOW *, chtype);
int waddstr(WINDOW *, const char *);
int wprintw(WINDOW *, const char *, ...);
int printw(const char *, ...);
chtype winch(WINDOW *);
int wclear(WINDOW *);
int werase(WINDOW *);
int wclrtoeol(WINDOW *);
int touchwin(WINDOW *);
int wrefresh(WINDOW *);
int wgetch(WINDOW *);
int wstandout(WINDOW *);
int wstandend(WINDOW *);
int wattrset(WINDOW *, int);
int wc_kbhit(void);
void wc_push(int key);         /* queue a key for wgetch */
WINDOW *dupwin(WINDOW *);
int delwin(WINDOW *);
int mvwprintw(WINDOW *, int, int, const char *, ...);
int wc_usleep(unsigned int us);   /* -Dusleep=wc_usleep */

#define getyx(w, y, x) ((y) = (w)->cury, (x) = (w)->curx)
#define mvwaddch(w, y, x, ch) (wmove(w, y, x) == ERR ? ERR : waddch(w, ch))
#define mvwaddstr(w, y, x, s) (wmove(w, y, x) == ERR ? ERR : waddstr(w, s))
#define mvwinch(w, y, x) (wmove(w, y, x) == ERR ? (chtype)ERR : winch(w))
#define move(y, x) wmove(stdscr, y, x)
#define addch(ch) waddch(stdscr, ch)
#define addstr(s) waddstr(stdscr, s)
#define clear() wclear(stdscr)
#define erase() werase(stdscr)
#define clrtoeol() wclrtoeol(stdscr)
#define refresh() wrefresh(stdscr)
#define getch() wgetch(stdscr)
#define standout() wstandout(stdscr)
#define standend() wstandend(stdscr)
#define scrollok(w, b) OK
#define clearok(w, b) ((w)->clear = (b), OK)
#define leaveok(w, b) OK
#define keypad(w, b) OK
#define noecho() OK
#define echo() OK
#define crmode() OK
#define cbreak() OK
#define nocrmode() OK
#define nocbreak() OK
#define nl() OK
#define nonl() OK
#define flushinp() OK
#define beep() OK

/* panes (page windows): the game names its windows with wc_pane(); a pane's
 * rect is the union of its windows. Other windows only reach the whole
 * screen (curscr); when one of them covers the map, the screen is a pop-up. */
enum { WC_FULL, WC_MAP, WC_SIDE, WC_STAT, WC_MSG, WC_PANES };
void wc_pane(WINDOW *w, int pane);
void wc_msg(const char *s, int append);   /* message history line */

/* frontend: the whole screen, plus each pane */
void be_init(int cols, int rows);
void be_put(int y, int x, chtype ch);
void be_pane(int pane, int y, int x, int rows, int cols);  /* pane rect on the screen */
void be_pput(int pane, int y, int x, chtype ch);          /* cell inside the pane */
void be_popup(int on);
void be_msg(const char *s, int append);
void be_cursor(int y, int x);
void be_hero(int y, int x);   /* player's screen cell: the map camera centres on it (RVIP.md W4) */
void be_flush(void);
int  be_getkey(int wait);   /* -1 when !wait and nothing queued */
void be_sleep(int ms);
void be_end(void);
#endif
