/* In-memory curses: windows are drawn onto curscr in wrefresh() order (like
 * real curses), and changed curscr cells go to the frontend. */
#include <stdlib.h>
#include <string.h>
#include "curses.h"

WINDOW *stdscr, *curscr;
int LINES = 24, COLS = 80;
static chtype *shown;

WINDOW *newwin(int rows, int cols, int by, int bx)
{
    WINDOW *w = calloc(1, sizeof *w);
    if (!rows) rows = LINES - by;
    if (!cols) cols = COLS - bx;
    w->maxy = rows; w->maxx = cols; w->begy = by; w->begx = bx;
    w->c = malloc(sizeof(chtype) * rows * cols);
    werase(w);
    return w;
}

WINDOW *initscr(void)
{
    char *e;
    if (curscr) return stdscr;
    if ((e = getenv("OMEGA_LINES")) && atoi(e) >= 24) LINES = atoi(e);
    curscr = newwin(LINES, COLS, 0, 0);
    stdscr = newwin(LINES, COLS, 0, 0);
    shown = malloc(sizeof(chtype) * LINES * COLS);
    memset(shown, 0xff, sizeof(chtype) * LINES * COLS);
    be_init(COLS, LINES);
    return stdscr;
}

int endwin(void) { return OK; }

int wmove(WINDOW *w, int y, int x)
{
    if (!w || y < 0 || x < 0 || y >= w->maxy || x >= w->maxx) return ERR;
    w->cury = y; w->curx = x;
    return OK;
}

int waddch(WINDOW *w, chtype ch)
{
    int y = w->cury, x = w->curx, c = ch & A_CHARTEXT;
    if (c == '\n') {
        wclrtoeol(w);
        if (y + 1 < w->maxy) { w->cury++; w->curx = 0; }
        return OK;
    }
    if (c == '\r') { w->curx = 0; return OK; }
    if (c == '\t') { do waddch(w, ' '); while (w->curx % 8 && w->curx); return OK; }
    if (c == '\b') { if (x) w->curx--; return OK; }
    if (c < ' ' && c) { waddch(w, '^'); return waddch(w, c + '@'); }
    if (!(ch & A_COLOR)) ch |= w->attr & A_COLOR;
    ch |= w->attr & A_STANDOUT;
    w->c[y * w->maxx + x] = ch;
    w->dirty = 1;
    if (++w->curx >= w->maxx) {
        if (y + 1 < w->maxy) { w->cury++; w->curx = 0; }
        else { w->curx = w->maxx - 1; return ERR; }
    }
    return OK;
}

int waddstr(WINDOW *w, const char *s)
{
    while (*s) waddch(w, (unsigned char)*s++);
    return OK;
}

static int vw(WINDOW *w, const char *f, va_list ap)
{
    char buf[2048];
    vsnprintf(buf, sizeof buf, f, ap);
    return waddstr(w, buf);
}

int wprintw(WINDOW *w, const char *f, ...)
{
    va_list ap; int r;
    va_start(ap, f); r = vw(w, f, ap); va_end(ap);
    return r;
}

int printw(const char *f, ...)
{
    va_list ap; int r;
    va_start(ap, f); r = vw(stdscr, f, ap); va_end(ap);
    return r;
}

chtype winch(WINDOW *w) { return w->c[w->cury * w->maxx + w->curx]; }

int wclrtoeol(WINDOW *w)
{
    int x;
    for (x = w->curx; x < w->maxx; x++) w->c[w->cury * w->maxx + x] = ' ';
    w->dirty = 1;
    return OK;
}

int werase(WINDOW *w)
{
    int i;
    for (i = 0; i < w->maxy * w->maxx; i++) w->c[i] = ' ';
    w->cury = w->curx = 0;
    w->dirty = 1;
    return OK;
}

int wclear(WINDOW *w) { werase(w); w->clear = 1; return OK; }
int touchwin(WINDOW *w) { w->dirty = 1; return OK; }
int wstandout(WINDOW *w) { w->attr |= A_STANDOUT; return OK; }
int wstandend(WINDOW *w) { w->attr &= ~A_STANDOUT; return OK; }
/* Omega passes COL_x >> 8 */
int wattrset(WINDOW *w, int a) { w->attr = (w->attr & A_STANDOUT) | (a << 8 & A_COLOR); return OK; }

int wrefresh(WINDOW *w)
{
    int y, x;
    if (!curscr) return ERR;
    if (w->clear) memset(shown, 0xff, sizeof(chtype) * LINES * COLS);
    w->clear = 0;
    /* ponytail: copies the whole window every refresh (no per-line change
     * ranges); 80x50 cells is nothing, add ranges if a profiler complains */
    for (y = 0; y < w->maxy; y++)
        for (x = 0; x < w->maxx; x++) {
            int sy = y + w->begy, sx = x + w->begx;
            if (sy < LINES && sx < COLS) curscr->c[sy * COLS + sx] = w->c[y * w->maxx + x];
        }
    w->dirty = 0;
    for (y = 0; y < LINES * COLS; y++)
        if (shown[y] != curscr->c[y]) {
            shown[y] = curscr->c[y];
            be_put(y / COLS, y % COLS, shown[y]);
        }
    be_cursor(w->cury + w->begy, w->curx + w->begx);
    be_flush();
    if (getenv("OMEGA_DUMP")) {     /* testing: the screen as text */
        FILE *f = fopen(getenv("OMEGA_DUMP"), "w");
        for (y = 0; f && y < LINES * COLS; y++) {
            fputc(curscr->c[y] & A_CHARTEXT, f);
            if (y % COLS == COLS - 1) fputc('\n', f);
        }
        if (f) fclose(f);
    }
    return OK;
}

static int pushback = -1;

int wgetch(WINDOW *w)
{
    int k = pushback;
    wrefresh(w);
    pushback = -1;
    return k >= 0 ? k : be_getkey(1);
}

/* a key is waiting (auto-explore stops on any key) */
int wc_kbhit(void)
{
    if (pushback < 0) pushback = be_getkey(0);
    return pushback >= 0;
}

int wc_usleep(unsigned int us) { be_flush(); be_sleep(us / 1000); return 0; }
