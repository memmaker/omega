/* In-memory curses: windows are drawn onto curscr in wrefresh() order (like
 * real curses), and changed curscr cells go to the frontend. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "curses.h"

WINDOW *stdscr, *curscr;
int LINES = 24, COLS = 80;
static chtype *shown;
static unsigned char *owner;       /* pane of the window that drew each curscr cell */
static struct { int y, x, r, c; chtype *shown; } P[WC_PANES];

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
    owner = calloc(LINES * COLS, 1);
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

int mvwprintw(WINDOW *w, int y, int x, const char *f, ...)
{
    va_list ap; int r;
    if (wmove(w, y, x) == ERR) return ERR;
    va_start(ap, f); r = vw(w, f, ap); va_end(ap);
    return r;
}

WINDOW *dupwin(WINDOW *s)
{
    WINDOW *w = newwin(s->maxy, s->maxx, s->begy, s->begx);
    memcpy(w->c, s->c, sizeof(chtype) * s->maxy * s->maxx);
    return w;
}

int delwin(WINDOW *w)
{
    if (!w) return ERR;
    free(w->c); free(w);
    return OK;
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

void wc_pane(WINDOW *w, int p)
{
    int y = P[p].r ? P[p].y : w->begy, x = P[p].r ? P[p].x : w->begx;
    int y1 = P[p].r ? P[p].y + P[p].r : 0, x1 = P[p].r ? P[p].x + P[p].c : 0;
    w->pane = p;
    if (w->begy < y) y = w->begy;
    if (w->begx < x) x = w->begx;
    if (w->begy + w->maxy > y1) y1 = w->begy + w->maxy;
    if (w->begx + w->maxx > x1) x1 = w->begx + w->maxx;
    P[p].y = y; P[p].x = x; P[p].r = y1 - y; P[p].c = x1 - x;
    free(P[p].shown);
    P[p].shown = malloc(sizeof(chtype) * P[p].r * P[p].c);
    memset(P[p].shown, 0xff, sizeof(chtype) * P[p].r * P[p].c);
    be_pane(p, y, x, P[p].r, P[p].c);
}

/* append: 1 = run-on text for the last line. A repeat of the last message
   becomes "message (xN)", replacing the page's last line (append = 2). */
void wc_msg(const char *s, int append)
{
    static char prev[512];
    static int reps;
    char fold[560];
    if (append) { prev[0] = 0; be_msg(s, 1); return; }
    if (*prev && !strcmp(s, prev)) {
        snprintf(fold, sizeof fold, "%s (x%d)", s, ++reps);
        be_msg(fold, 2);
        return;
    }
    snprintf(prev, sizeof prev, "%s", s);
    reps = 1;
    be_msg(s, 0);
}

int wrefresh(WINDOW *w)
{
    int y, x, pop = 0;
    if (!curscr) return ERR;
    if (w->clear) memset(shown, 0xff, sizeof(chtype) * LINES * COLS);
    w->clear = 0;
    /* ponytail: copies the whole window every refresh (no per-line change
     * ranges); 80x50 cells is nothing, add ranges if a profiler complains */
    for (y = 0; y < w->maxy; y++)
        for (x = 0; x < w->maxx; x++) {
            int sy = y + w->begy, sx = x + w->begx;
            chtype v = w->c[y * w->maxx + x] | (w->tiles ? A_TILE | (chtype)wc_tile(w->c[y * w->maxx + x]) << 18 : 0);
            if (sy < LINES && sx < COLS) { curscr->c[sy * COLS + sx] = v; owner[sy * COLS + sx] = w->pane; }
            if (w->pane) {
                int py = sy - P[w->pane].y, px = sx - P[w->pane].x, i = py * P[w->pane].c + px;
                if (P[w->pane].shown[i] != v) { P[w->pane].shown[i] = v; be_pput(w->pane, py, px, v); }
            }
        }
    w->dirty = 0;
    /* a window that isn't a pane covers the map: show the whole screen */
    for (y = P[WC_MAP].y; y < P[WC_MAP].y + P[WC_MAP].r; y++)
        for (x = P[WC_MAP].x; x < P[WC_MAP].x + P[WC_MAP].c; x++)
            if (owner[y * COLS + x] == WC_FULL) pop = 1;
    be_popup(pop || !P[WC_MAP].r);
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

static int queue[64], qn, pushback = -1;

/* keys the game queues (item menus run commands this way) come first */
void wc_push(int k) { if (qn < 64) queue[qn++] = k; }

int wgetch(WINDOW *w)
{
    int k = pushback;
    wrefresh(w);
    if (qn) {
        k = queue[0];
        memmove(queue, queue + 1, --qn * sizeof *queue);
        return k;
    }
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
