/* X11 frontend for the curses shim: one text window, Xft font, the 16 PC
 * colours Omega's MSDOS build uses. The map (A_TILE cells) is drawn with
 * Kinder's WinOmega tiles (port/tiles.bmp, tile from port/tiles.c), square
 * cells of row height scrolled round the player, as web/omega.js does.
 * Env: OMEGA_XFT (font, default Menlo), OMEGA_TEXT (px, 18), OMEGA_TILES=0 text, OMEGA_BMP (sheet path),
 * OMEGA_POS "x,y", OMEGA_LINES (rows, >= 24). */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "curses.h"
#include <time.h>

static Display *dpy;
static Window win;
static Pixmap pix;
static GC gc;
static XftDraw *xd;
static XftFont *fnt;
static XftColor col[16];
static int cols, rows, tw, th, cy, cx;
static chtype *cell;
static int tiles, mapdirty, tox;
static unsigned char *bmp;          /* 8-bit sheet, 4096x960, top row first */
static unsigned bpal[256];
static XImage *timg[4096];          /* scaled tiles, made when first drawn */
#define MAPW 64

/* Kinder's 32x32.bmp: 8-bit, uncompressed, bottom-up */
static int load_bmp(const char *path)
{
    unsigned char h[54], q[4];
    FILE *f = fopen(path, "rb");
    int i, off;
    if (!f || fread(h, 1, 54, f) != 54 || h[0] != 'B' || h[28] != 8
        || (h[18] | h[19] << 8 | h[20] << 16 | h[21] << 24) != 4096
        || (h[22] | h[23] << 8) != 960) { if (f) fclose(f); return 0; }
    off = h[10] | h[11] << 8 | h[12] << 16;
    fseek(f, 14 + (h[14] | h[15] << 8), SEEK_SET);
    for (i = 0; i < 256 && fread(q, 1, 4, f) == 4; i++) bpal[i] = q[2] << 16 | q[1] << 8 | q[0];
    bmp = malloc(4096 * 960);
    fseek(f, off, SEEK_SET);
    for (i = 959; i >= 0; i--) fread(bmp + i * 4096, 1, 4096, f);
    fclose(f);
    return 1;
}

static XImage *tile_img(int t)
{
    int x, y, sx = t % 128 * 32, sy = t / 128 * 32;
    XImage *im;
    if (timg[t]) return timg[t];
    im = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), DefaultDepth(dpy, DefaultScreen(dpy)),
                      ZPixmap, 0, malloc(th * th * 4), th, th, 32, 0);
    for (y = 0; y < th; y++)        /* nearest neighbour */
        for (x = 0; x < th; x++)
            XPutPixel(im, x, y, bpal[bmp[(sy + y * 32 / th) * 4096 + sx + x * 32 / th]]);
    return timg[t] = im;
}

static void draw_map(void)
{
    int y, i, nx = MAPW * tw / th, onmap = cx < MAPW && cell[cy * cols] & A_TILE;
    if (nx > MAPW) nx = MAPW;
    if (onmap) tox = cx - nx / 2 < 0 ? 0 : cx - nx / 2 > MAPW - nx ? MAPW - nx : cx - nx / 2;
    for (y = 0; y < rows; y++) {
        if (!(cell[y * cols] & A_TILE)) continue;
        XftDrawRect(xd, &col[0], 0, y * th, MAPW * tw, th);
        for (i = 0; i < nx; i++) {
            chtype v = cell[y * cols + tox + i];
            FcChar8 c = v & A_CHARTEXT;
            if (v >> 18) XPutImage(dpy, pix, gc, tile_img((v >> 18) - 1), 0, 0, i * th, y * th, th, th);
            else if (c != ' ')
                XftDrawString8(xd, &col[v & A_COLOR ? v >> 8 & 15 : 7], fnt, i * th + (th - tw) / 2, y * th + fnt->ascent, &c, 1);
        }
    }
}

static const unsigned char pal[16][3] = {
    {0,0,0}, {0,0,170}, {0,170,0}, {0,170,170}, {170,0,0}, {170,0,170}, {170,85,0}, {170,170,170},
    {85,85,85}, {85,85,255}, {85,255,85}, {85,255,255}, {255,85,85}, {255,85,255}, {255,255,85}, {255,255,255}};

void be_init(int c, int r)
{
    XSizeHints h;
    XGlyphInfo gi;
    const char *e;
    int scr, i, x = 0, y = 0;
    Visual *vis;
    Colormap cm;

    if (!(dpy = XOpenDisplay(NULL))) { fprintf(stderr, "omega: no X display\n"); exit(1); }
    scr = DefaultScreen(dpy); vis = DefaultVisual(dpy, scr); cm = DefaultColormap(dpy, scr);
    fnt = XftFontOpen(dpy, scr, XFT_FAMILY, XftTypeString, (e = getenv("OMEGA_XFT")) ? e : "Menlo",
                      XFT_PIXEL_SIZE, XftTypeDouble, (e = getenv("OMEGA_TEXT")) ? atof(e) : 18.0, NULL);
    XftTextExtents8(dpy, fnt, (FcChar8 *)"M", 1, &gi);
    tw = gi.xOff; th = fnt->ascent + fnt->descent;
    for (i = 0; i < 16; i++) {
        XRenderColor rc = { pal[i][0] * 257, pal[i][1] * 257, pal[i][2] * 257, 0xffff };
        XftColorAllocValue(dpy, vis, cm, &rc, &col[i]);
    }
    cols = c; rows = r;
    cell = calloc(c * r, sizeof *cell);
    tiles = !((e = getenv("OMEGA_TILES")) && *e == '0') && load_bmp((e = getenv("OMEGA_BMP")) ? e : "port/tiles.bmp");
    if ((e = getenv("OMEGA_POS"))) sscanf(e, "%d,%d", &x, &y);
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), x, y, c * tw, r * th, 0, 0, 0);
    h.flags = PPosition | USPosition | PMinSize | PMaxSize;
    h.x = x; h.y = y;
    h.min_width = h.max_width = c * tw;
    h.min_height = h.max_height = r * th;
    XSetWMNormalHints(dpy, win, &h);
    XStoreName(dpy, win, "Omega");
    XSelectInput(dpy, win, KeyPressMask | ExposureMask);
    pix = XCreatePixmap(dpy, win, c * tw, r * th, DefaultDepth(dpy, scr));
    xd = XftDrawCreate(dpy, pix, vis, cm);
    gc = XCreateGC(dpy, win, 0, NULL);
    XftDrawRect(xd, &col[0], 0, 0, c * tw, r * th);
    XMapWindow(dpy, win);
    XFlush(dpy);
}

void be_put(int y, int x, chtype ch)
{
    FcChar8 c = ch & A_CHARTEXT;
    int fg = ch >> 8 & 15, bg = ch >> 12 & 7, t;
    if (!(ch & A_COLOR)) fg = 7;    /* plain text: light grey */
    if (ch & A_STANDOUT) { t = fg; fg = bg; bg = t; }
    cell[y * cols + x] = ch;
    if (tiles && ch & A_TILE && x < MAPW) { mapdirty = 1; return; }
    XftDrawRect(xd, &col[bg], x * tw, y * th, tw, th);
    if (c != ' ') XftDrawString8(xd, &col[fg], fnt, x * tw, y * th + fnt->ascent, &c, 1);
}

/* one text window: panes, pop-ups and the message history are the screen itself */
void be_pane(int p, int y, int x, int r, int c) { }
void be_pput(int p, int y, int x, chtype ch) { }
void be_popup(int on) { }
void be_msg(const char *s, int append) { }

void be_cursor(int y, int x) { if (y != cy || x != cx) mapdirty = 1; cy = y; cx = x; }

void be_flush(void)
{
    if (tiles && mapdirty) { draw_map(); mapdirty = 0; }
    XCopyArea(dpy, pix, win, gc, 0, 0, cols * tw, rows * th, 0, 0);
    if (tiles && cx < MAPW && cell[cy * cols] & A_TILE) {
        XSetForeground(dpy, gc, col[14].pixel);
        XDrawRectangle(dpy, win, gc, (cx - tox) * th, cy * th, th - 1, th - 1);
    } else {
        XSetForeground(dpy, gc, col[7].pixel);
        XFillRectangle(dpy, win, gc, cx * tw, cy * th + th - 2, tw, 2);
    }
    XFlush(dpy);
}

static int keycode(XKeyEvent *ev)
{
    char buf[8];
    KeySym ks;
    int n = XLookupString(ev, buf, sizeof buf, &ks, NULL);
    switch (ks) {       
    /* arrows = keypad digits: Omega moves with them, and lists use 8/2 */
    case XK_Left: case XK_KP_Left: return '4';
    case XK_Right: case XK_KP_Right: return '6';
    case XK_Up: case XK_KP_Up: return '8';
    case XK_Down: case XK_KP_Down: return '2';
    case XK_Home: case XK_KP_Home: return '7';
    case XK_Prior: case XK_KP_Prior: return '9';
    case XK_End: case XK_KP_End: return '1';
    case XK_Next: case XK_KP_Next: return '3';
    case XK_KP_Begin: return '5';
    case XK_KP_Enter: case XK_Return: return '\n';   /* curses nl() mode */
    case XK_BackSpace: case XK_Delete: return '\b';
    case XK_KP_Add: return '+';
    case XK_KP_Subtract: return '-';
    case XK_KP_Multiply: return '*';
    case XK_KP_Divide: return '/';
    case XK_KP_Decimal: case XK_KP_Delete: return '.';
    case XK_KP_Insert: return '0';
    }
    if (ks >= XK_KP_0 && ks <= XK_KP_9) return '0' + (int)(ks - XK_KP_0);
    return n == 1 ? (unsigned char)buf[0] : -1;
}

int be_getkey(int wait)
{
    XEvent ev;
    for (;;) {
        if (!wait && !XPending(dpy)) return -1;
        XNextEvent(dpy, &ev);
        if (ev.type == Expose) be_flush();
        else if (ev.type == KeyPress) {
            int k = keycode(&ev.xkey);
            if (k >= 0) return k;
        }
    }
}

void be_sleep(int ms)
{
    struct timespec t = { ms / 1000, ms % 1000 * 1000000L };
    nanosleep(&t, NULL);
}
void be_end(void) { if (dpy) XCloseDisplay(dpy); dpy = NULL; }
