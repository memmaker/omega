/* Terminal frontend for the curses shim (native release builds): draws the
 * panes into one text screen with ANSI/VT escapes, no curses library.
 * Linux and macOS: termios; Windows: console VT mode and _getch().
 * Omega's shim hands over the whole 80x24 text screen (be_put). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#endif
#include "curses.h"
#undef exit
#undef usleep

#define SW 200                  /* screen buffer size */
#define SH 80

static chtype scr[SH][SW], out[SH][SW];
static int curY, curX, started, term_w = 80, term_h = 24, full = 1;

#ifdef _WIN32
static HANDLE hin, hout;
static DWORD in_mode, out_mode;
#else
static struct termios saved;
#endif

static void term_size(void)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO i;
    if (GetConsoleScreenBufferInfo(hout, &i)) {
        term_w = i.srWindow.Right - i.srWindow.Left + 1;
        term_h = i.srWindow.Bottom - i.srWindow.Top + 1;
    }
#else
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col) { term_w = ws.ws_col; term_h = ws.ws_row; }
#endif
    if (term_w > SW) term_w = SW;
    if (term_h > SH) term_h = SH;
}

static void term_start(void)
{
    if (started) return;
    started = 1;
#ifdef _WIN32
    hin = GetStdHandle(STD_INPUT_HANDLE);
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hin, &in_mode);
    GetConsoleMode(hout, &out_mode);
    SetConsoleMode(hout, out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    SetConsoleMode(hin, in_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
    SetConsoleOutputCP(437);
#else
    struct termios t;
    tcgetattr(0, &saved);
    t = saved;
    t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    t.c_iflag &= ~(IXON | ICRNL | INLCR);
    t.c_cc[VMIN] = 1; t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
#endif
    term_size();
    fputs("\033[?1049h\033[H\033[2J", stdout);  /* alternate screen */
    fflush(stdout);
}

static void term_stop(void)
{
    if (!started) return;
    started = 0;
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
#ifdef _WIN32
    SetConsoleMode(hin, in_mode);
    SetConsoleMode(hout, out_mode);
#else
    tcsetattr(0, TCSANOW, &saved);
#endif
}

void be_init(int cols, int rows) { (void)cols; (void)rows; term_start(); }
void be_put(int y, int x, chtype ch) { if (y >= 0 && x >= 0 && y < SH && x < SW) scr[y][x] = ch; }
void be_pane(int p, int y, int x, int r, int c) { (void)p; (void)y; (void)x; (void)r; (void)c; }
void be_pput(int p, int y, int x, chtype ch) { (void)p; (void)y; (void)x; (void)ch; }
void be_hero(int y, int x) { (void)y; (void)x; }
void be_popup(int on) { (void)on; }
void be_msg(const char *s, int append) { (void)s; (void)append; }
void be_cursor(int y, int x) { curY = y; curX = x; }

void be_flush(void)
{
    int y, x;
    static int last_w, last_h;
    char buf[64];

    if (!started) return;
    term_size();
    if (term_w != last_w || term_h != last_h) { last_w = term_w; last_h = term_h; full = 1; }
    if (full) { fputs("\033[0m\033[2J", stdout); memset(out, 0xff, sizeof out); full = 0; }
    fputs("\033[?25l", stdout);
    for (y = 0; y < term_h; y++) {
        int at = -1, rev = -1;
        for (x = 0; x < term_w; x++) {
            chtype c = scr[y][x];
            int ch = c & A_CHARTEXT, r = !!(c & A_STANDOUT);
            if (!c) c = ' ', ch = ' ';
            if (out[y][x] == c) continue;
            out[y][x] = c;
            if (at != x) { snprintf(buf, sizeof buf, "\033[%d;%dH", y + 1, x + 1); fputs(buf, stdout); }
            if (r != rev) { fputs(r ? "\033[7m" : "\033[0m", stdout); rev = r; }
            putchar(ch < ' ' || ch >= 127 ? ' ' : ch);
            at = x + 1;
        }
        if (rev == 1) fputs("\033[0m", stdout);
    }
    snprintf(buf, sizeof buf, "\033[%d;%dH\033[?25h", curY + 1, curX + 1);
    fputs(buf, stdout);
    fflush(stdout);
}

void be_sleep(int ms)
{
    fflush(stdout);
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* raw input */
#ifdef _WIN32
static int key_ready(int ms)
{
    DWORD t = GetTickCount();
    do {
        if (_kbhit()) return 1;
        if (ms) Sleep(5);
    } while ((int)(GetTickCount() - t) < ms);
    return 0;
}
static int readc(void) { return _getch(); }
#else
static int key_ready(int ms)
{
    fd_set s;
    struct timeval tv;
    FD_ZERO(&s); FD_SET(0, &s);
    tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000;
    return select(1, &s, NULL, NULL, &tv) > 0;
}
static int readc(void)
{
    unsigned char c;
    if (read(0, &c, 1) != 1) { term_stop(); exit(0); }
    return c;
}
#endif

static int readkey(void)
{
    int c = readc();
#ifdef _WIN32
    if (c == 0 || c == 224) {
        switch (readc()) {
        case 72: return KEY_UP; case 80: return KEY_DOWN;
        case 75: return KEY_LEFT; case 77: return KEY_RIGHT;
        case 71: return KEY_A1; case 73: return KEY_A3;
        case 79: return KEY_C1; case 81: return KEY_C3;
        case 76: return KEY_B2; case 83: return '\b';
        }
        return -1;
    }
    if (c == 3) return 3;
#else
    if (c == 27 && key_ready(30)) {     /* escape sequence */
        int a = readc(), b, n = 0;
        if (a != '[' && a != 'O') return a == 27 ? 27 : a;
        b = readc();
        while (b >= '0' && b <= '9') { n = n * 10 + b - '0'; b = readc(); }
        switch (b) {
        case 'A': return KEY_UP; case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT; case 'D': return KEY_LEFT;
        case 'H': return KEY_A1; case 'F': return KEY_C1;
        case 'E': return KEY_B2;
        case '~':
            switch (n) {
            case 1: case 7: return KEY_A1; case 4: case 8: return KEY_C1;
            case 5: return KEY_A3; case 6: return KEY_C3; case 3: return '\b';
            }
        }
        return -1;
    }
    if (c == 127) return '\b';
#endif
    if (c == '\r') return '\n';     /* Enter, as be_x11.c and the web give it */
    return c;
}

int be_getkey(int wait)
{
    for (;;) {
        int k;
        if (wait <= 0 && !key_ready(0)) {
            /* polling (auto-explore): paint each step, a short pause;
             * wait -1 only drains the queue */
            if (!wait) be_flush();
            return -1;
        }
        be_flush();
        if ((k = readkey()) >= 0) return k;
    }
}

void be_end(void) { term_stop(); }

/* exit() (build-term.sh: -Dexit=wc_exit): give the terminal back first */
void wc_exit(int code) { term_stop(); exit(code); }
