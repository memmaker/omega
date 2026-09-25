/* Browser frontend for the curses shim (RVIP step 7): web/omega.js draws
 * the text screen (Module.om); input waits with Asyncify. omega.sav is an
 * autosave while playing (written at the command prompt when the page asks),
 * and removed when the game ends unless the player saved with S. */
#include <emscripten.h>
#include <stdlib.h>
#include <unistd.h>
#include "curses.h"

extern int Rl_at_prompt, Rl_saved;
void rl_autosave(void);

EM_JS(void, js_init, (int c, int r), { Module.om.init(c, r); });
EM_JS(void, js_put, (int y, int x, int ch), { Module.om.put(y, x, ch); });
EM_JS(void, js_cursor, (int y, int x), { Module.om.cursor(y, x); });
EM_JS(void, js_flush, (void), { Module.om.flush(); });
EM_JS(int, js_key, (void), { return Module.om.key(); });
EM_JS(int, js_want_save, (void), { return Module.om.wantSave(); });
EM_JS(void, js_end, (int saved), { Module.om.end(saved); });

static void at_exit(void)
{
    if (!Rl_saved) unlink("omega.sav");     /* died or quit: the game is over */
    js_end(Rl_saved);
}

void be_init(int c, int r) { atexit(at_exit); js_init(c, r); }
void be_put(int y, int x, chtype ch) { js_put(y, x, ch); }
void be_cursor(int y, int x) { js_cursor(y, x); }
void be_flush(void) { js_flush(); }
void be_sleep(int ms) { emscripten_sleep(ms); }
void be_end(void) { }

int be_getkey(int wait)
{
    static double last;
    int k;
    for (;;) {
        if (Rl_at_prompt && js_want_save()) {
            Rl_at_prompt = 0;               /* the save redraws nothing, but be safe */
            rl_autosave();
            Rl_at_prompt = 1;
        }
        if ((k = js_key()) >= 0) return k;
        if (!wait) {                /* polling (explore): let the page paint */
            if (emscripten_get_now() - last > 50) {
                last = emscripten_get_now();
                emscripten_sleep(0);
            }
            return -1;
        }
        emscripten_sleep(10);
    }
}
