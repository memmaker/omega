/* In-memory curses: windows are drawn onto curscr in wrefresh() order (like
 * real curses), and changed curscr cells go to the frontend. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "curses.h"

WINDOW *stdscr, *curscr;
int LINES = 36, COLS = 106, COLORS = 16, COLOR_PAIRS = 256;
static short pairs[256][2];      /* curses colours, set by init_pair */
static int curvis = 1, cury, curx;
static chtype *shown;
static unsigned char *owner;       /* pane of the window that drew each curscr cell */
static struct { int y, x, r, c; chtype *shown; } P[WC_PANES];

static void touch(WINDOW *w, int y, int x0, int x1)
{
    if (x0 < w->lo[y]) w->lo[y] = x0;
    if (x1 > w->hi[y]) w->hi[y] = x1;
    w->dirty = 1;
}

static void touchall(WINDOW *w)
{
    int y;
    for (y = 0; y < w->maxy; y++) { w->lo[y] = 0; w->hi[y] = w->maxx; }
    w->dirty = 1;
}

WINDOW *newwin(int rows, int cols, int by, int bx)
{
    WINDOW *w = calloc(1, sizeof *w);
    if (!rows) rows = LINES - by;
    if (!cols) cols = COLS - bx;
    w->maxy = rows; w->maxx = cols; w->begy = by; w->begx = bx;
    w->delay = -1;
    w->c = malloc(sizeof(chtype) * rows * cols);
    w->lo = malloc(rows * sizeof(int)); w->hi = malloc(rows * sizeof(int));
    werase(w);
    return w;
}

WINDOW *initscr(void)
{
    char *e;
    if (curscr) return stdscr;
    int i;
    /* fixed grid: 106 columns = rebirth's full 64-wide map + its 42-column side */
    if ((e = getenv("OMEGA_LINES")) && atoi(e) >= 24) LINES = atoi(e);
    if ((e = getenv("OMEGA_COLS")) && atoi(e) >= 80) COLS = atoi(e);
    for (i = 0; i < 256; i++) { pairs[i][0] = COLOR_WHITE; pairs[i][1] = COLOR_BLACK; }
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
    ch |= w->attr & (A_STANDOUT | A_BOLD | A_UNDERLINE);
    w->c[y * w->maxx + x] = ch;
    touch(w, y, x, x + 1);
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
    if (s == curscr) {      /* a copy of the screen: put back as is (menus restore it) */
        w->own = malloc(LINES * COLS);
        memcpy(w->own, owner, LINES * COLS);
    }
    return w;
}

int delwin(WINDOW *w)
{
    if (!w) return ERR;
    free(w->c); free(w->lo); free(w->hi); free(w->own); free(w);
    return OK;
}

chtype winch(WINDOW *w) { return w->c[w->cury * w->maxx + w->curx]; }

int wclrtoeol(WINDOW *w)
{
    int x;
    for (x = w->curx; x < w->maxx; x++) w->c[w->cury * w->maxx + x] = ' ';
    touch(w, w->cury, w->curx, w->maxx);
    return OK;
}

int werase(WINDOW *w)
{
    int i;
    for (i = 0; i < w->maxy * w->maxx; i++) w->c[i] = ' ';
    w->cury = w->curx = 0;
    touchall(w);
    return OK;
}

int wclear(WINDOW *w) { werase(w); w->clear = 1; return OK; }
int touchwin(WINDOW *w) { touchall(w); return OK; }
int wattrset(WINDOW *w, int a) { w->attr = (chtype)a & ~A_CHARTEXT; return OK; }
int wattr_on(WINDOW *w, attr_t a, void *o) { (void)o; if (a & A_COLOR) w->attr &= ~A_COLOR; w->attr |= a & ~A_CHARTEXT; return OK; }
int wattr_off(WINDOW *w, attr_t a, void *o) { (void)o; w->attr &= ~(a & ~A_CHARTEXT); return OK; }
int wcolor_set(WINDOW *w, short p, void *o) { (void)o; w->attr = (w->attr & ~A_COLOR) | COLOR_PAIR(p); return OK; }
int start_color(void) { return OK; }
int init_pair(short p, short f, short b)
{
    if (p <= 0 || p > 255) return ERR;      /* pair 0 stays white on black */
    pairs[p][0] = f; pairs[p][1] = b;
    return OK;
}
int init_color(short c, short r, short g, short b) { (void)c; (void)r; (void)g; (void)b; return ERR; }
int curs_set(int v) { int o = curvis; curvis = v; return o; }

int mvwin(WINDOW *w, int y, int x)
{
    if (!w || y < 0 || x < 0 || y + w->maxy > LINES || x + w->maxx > COLS) return ERR;
    w->begy = y; w->begx = x; touchall(w);
    return OK;
}

int wresize(WINDOW *w, int rows, int cols)
{
    int y, x;
    chtype *c;
    if (!w || rows <= 0 || cols <= 0) return ERR;
    c = malloc(sizeof(chtype) * rows * cols);
    for (y = 0; y < rows; y++)
        for (x = 0; x < cols; x++)
            c[y * cols + x] = y < w->maxy && x < w->maxx ? w->c[y * w->maxx + x] : ' ';
    free(w->c);
    w->c = c; w->maxy = rows; w->maxx = cols;
    if (w->cury >= rows) w->cury = rows - 1;
    if (w->curx >= cols) w->curx = cols - 1;
    w->lo = realloc(w->lo, rows * sizeof(int)); w->hi = realloc(w->hi, rows * sizeof(int));
    touchall(w);
    return OK;
}

/* window cell -> screen cell: curses pair + A_BOLD -> PC fg/bg */
chtype wc_fold(chtype v)
{
    static const int pc[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };   /* curses COLOR_x -> PC colour */
    int p = PAIR_NUMBER(v), f = pairs[p][0], b = pairs[p][1], fg, bg;
    fg = pc[f & 7] | (f & 8) | (v & A_BOLD ? 8 : 0);
    bg = pc[b & 7];
    if (!p && !(v & A_BOLD)) return (v & (A_CHARTEXT | A_STANDOUT));   /* plain text */
    return (v & (A_CHARTEXT | A_STANDOUT)) | (chtype)fg << 8 | (chtype)bg << 12;
}

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

/* a map cell with its tile; the tile looks at the 8 neighbours the window
 * holds (gromega joins walls, hedges, water) */
static chtype tilecell(WINDOW *w, int y, int x)
{
    chtype v = wc_fold(w->c[y * w->maxx + x]);
    int nb[8], k = 0, dy, dx;
    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++)
            if (dy || dx) {
                int ny = y + dy, nx = x + dx;
                nb[k++] = ny < 0 || nx < 0 || ny >= w->maxy || nx >= w->maxx ? -1 : (int)(wc_fold(w->c[ny * w->maxx + nx]) & 0x7fff);
            }
    return v | A_TILE | (chtype)wc_tile(v, nb, y, x) << 18;
}

/* one window cell onto curscr and its pane */
static void putcell(WINDOW *w, int y, int x, chtype v)
{
    int sy = y + w->begy, sx = x + w->begx;
    int pane = w->own ? w->own[sy * COLS + sx] : w->pane;
    if (sy >= 0 && sx >= 0 && sy < LINES && sx < COLS) { curscr->c[sy * COLS + sx] = v; owner[sy * COLS + sx] = pane; }
    if (pane) {
        int py = sy - P[pane].y, px = sx - P[pane].x, i = py * P[pane].c + px;
        if (P[pane].shown[i] != v) { P[pane].shown[i] = v; be_pput(pane, py, px, v); }
    }
}

/* a map cell not refreshed itself but next to one that was (its joins may
 * change), or all of them for a new tile set: only where the map is on top */
static void retilecell(WINDOW *w, int y, int x)
{
    int sy = y + w->begy, sx = x + w->begx;
    if (sy >= 0 && sx >= 0 && sy < LINES && sx < COLS && owner[sy * COLS + sx] == w->pane && (curscr->c[sy * COLS + sx] & A_TILE))
        putcell(w, y, x, tilecell(w, y, x));
}

static WINDOW *tilewin;     /* the map window (tiles), for wc_retile() */

int wnoutrefresh(WINDOW *w)
{
    int y, x, *lo = 0, *hi = 0;
    if (!curscr || !w) return ERR;
    if (w->clear) memset(shown, 0xff, sizeof(chtype) * LINES * COLS);
    w->clear = 0;
    if (w->tiles) {             /* the refreshed ranges, to retile their neighbours */
        tilewin = w;
        lo = malloc(w->maxy * sizeof *lo); hi = malloc(w->maxy * sizeof *hi);
        memcpy(lo, w->lo, w->maxy * sizeof *lo); memcpy(hi, w->hi, w->maxy * sizeof *hi);
    }
    /* like curses, only the cells changed (or touchwin()ed) since the last
     * refresh reach the screen: windows overlap (menus over the map) */
    for (y = 0; y < w->maxy; y++, w->lo[y - 1] = w->maxx, w->hi[y - 1] = 0)
        for (x = w->lo[y]; x < w->hi[y]; x++)
            putcell(w, y, x, w->tiles ? tilecell(w, y, x) : w->own ? w->c[y * w->maxx + x] : wc_fold(w->c[y * w->maxx + x]));
    if (lo) {
        for (y = 0; y < w->maxy; y++) {
            int dy, a = w->maxx, b = 0;
            for (dy = y - 1; dy <= y + 1; dy++)
                if (dy >= 0 && dy < w->maxy && lo[dy] < hi[dy]) { if (lo[dy] - 1 < a) a = lo[dy] - 1; if (hi[dy] + 1 > b) b = hi[dy] + 1; }
            for (x = a < 0 ? 0 : a; x < b && x < w->maxx; x++)
                if (x < lo[y] || x >= hi[y]) retilecell(w, y, x);
        }
        free(lo); free(hi);
    }
    w->dirty = 0;
    cury = w->cury + w->begy; curx = w->curx + w->begx;
    return OK;
}

/* the page switched tile sets: new tiles for the shown map */
void wc_retile(void)
{
    int y, x;
    if (tilewin)
        for (y = 0; y < tilewin->maxy; y++)
            for (x = 0; x < tilewin->maxx; x++) retilecell(tilewin, y, x);
    doupdate();
}

int doupdate(void)
{
    int y, x, p, pop = 0, y0 = LINES, x0 = COLS, y1 = 0, x1 = 0;
    if (!curscr) return ERR;
    /* a window that isn't a pane covers a pane (menus over the map, choice
     * boxes over the side panel): show the whole screen */
    for (p = WC_MAP; p < WC_PANES; p++)
        for (y = P[p].y; y < P[p].y + P[p].r; y++)
            for (x = P[p].x; x < P[p].x + P[p].c; x++)
                /* side panes have gaps no game window draws: there only text counts */
                if (owner[y * COLS + x] == WC_FULL && (p == WC_MAP || (curscr->c[y * COLS + x] & A_CHARTEXT) > ' ')) {
                    pop = 1;    /* the pop-up's box (its text): the page shows only it, over the panes */
                    if ((curscr->c[y * COLS + x] & A_CHARTEXT) > ' ') {
                        if (y < y0) y0 = y; if (x < x0) x0 = x; if (y >= y1) y1 = y + 1; if (x >= x1) x1 = x + 1;
                    }
                }
    if (!P[WC_MAP].r || y1 <= y0) { y0 = x0 = 0; y1 = LINES; x1 = COLS; pop |= !P[WC_MAP].r; }
    be_popup(pop, y0, x0, y1 - y0, x1 - x0);
    for (y = 0; y < LINES * COLS; y++)
        if (shown[y] != curscr->c[y]) {
            shown[y] = curscr->c[y];
            be_put(y / COLS, y % COLS, shown[y]);
        }
    be_cursor(cury, curx);
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

int wrefresh(WINDOW *w) { return wnoutrefresh(w) == ERR ? ERR : doupdate(); }

static int queue[64], qn, pushback = -1;

/* keys the game queues (item menus run commands this way) come first */
void wc_push(int k) { if (qn < 64) queue[qn++] = k; }

/* ungetch: next key read */
int ungetch(int k)
{
    if (qn >= 64) return ERR;
    memmove(queue + 1, queue, qn++ * sizeof *queue);
    queue[0] = k;
    return OK;
}

void wtimeout(WINDOW *w, int d) { w->delay = d; }

int wgetch(WINDOW *w)
{
    int k = pushback, t;
    if (w->dirty || w->clear) wrefresh(w); else doupdate();
    if (qn) {
        k = queue[0];
        memmove(queue, queue + 1, --qn * sizeof *queue);
        return k;
    }
    pushback = -1;
    if (k >= 0) return k;
    if (w->delay < 0) return be_getkey(1);
    for (t = 0;; t += 10) {     /* wtimeout(): ERR after delay ms */
        if ((k = be_getkey(0)) >= 0) return k;
        if (t >= w->delay) return ERR;
        be_sleep(10);
    }
}

/* a key is waiting (auto-explore stops on any key) */
int wc_kbhit(void)
{
    if (pushback < 0) pushback = be_getkey(0);
    return pushback >= 0;
}

int wc_usleep(unsigned int us) { be_flush(); be_sleep(us / 1000); return 0; }
