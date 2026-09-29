/* Browser frontend for the curses shim (RVIP step 7): web/omega.js draws
 * the text screen (Module.om); input waits with Asyncify. Saves: OMEGALIB's
 * saves/ links into the IndexedDB folder (web/omega.js), synced while playing. */
#include <emscripten.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "curses.h"

EM_JS(void, js_init, (int c, int r), { Module.om.init(c, r); });
EM_JS(void, js_put, (int y, int x, int ch), { Module.om.put(y, x, ch); });
EM_JS(void, js_cursor, (int y, int x), { Module.om.cursor(y, x); });
EM_JS(void, js_flush, (void), { Module.om.flush(); });
EM_JS(int, js_key, (int at_cmd), { return Module.om.key(at_cmd); });
EM_JS(int, js_want_save, (void), { return Module.om.wantSave(); });
EM_JS(void, js_end, (int saved), { Module.om.end(saved); });
/* ponytail: stage 2 of the rebirth port. The run report (js_beacon), the
 * Inventory/Visible lists and the autosave come back with rl.c in stage 3. */

/* exit() (build.sh: -Dexit=wc_exit): Emscripten runs no atexit handlers, so
 * end the page here and idle; the player reloads (ponytail: the wasm stays up) */
void wc_exit(int code)
{
    (void)code;
    js_end(0);
    for (;;) emscripten_sleep(1000);
}

void be_init(int c, int r) { js_init(c, r); }
void be_put(int y, int x, chtype ch) { js_put(y, x, ch); }
void be_cursor(int y, int x) { js_cursor(y, x); }
EM_JS(void, be_pane, (int p, int y, int x, int r, int c), { Module.om.pane(p, y, x, r, c); });
EM_JS(void, be_pput, (int p, int y, int x, chtype ch), { Module.om.pput(p, y, x, ch); });
EM_JS(void, be_hero, (int y, int x), { Module.om.hero(y, x); });
EM_JS(void, be_popup, (int on), { Module.om.popup(on); });
EM_JS(void, be_msg, (const char *s, int append), { Module.om.msg(UTF8ToString(s), append); });
void be_flush(void) { js_flush(); }
void be_sleep(int ms) { emscripten_sleep(ms); }
void be_end(void) { }

int be_getkey(int wait)
{
    int k;
    for (;;) {
        js_want_save();             /* persists IDBFS (saves/) at most every 2 s */
        if ((k = js_key(1)) >= 0) return k;
        if (!wait) {                /* polling (explore): paint each step */
            emscripten_sleep(40);
            return -1;
        }
        emscripten_sleep(10);
    }
}
