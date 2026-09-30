/* Browser frontend for the curses shim (RVIP step 7): web/omega.js draws
 * the text screen (Module.om); input waits with Asyncify. Saves: OMEGALIB's
 * saves/ links into the IndexedDB folder (web/omega.js); the game autosaves
 * there at the command prompt when the page asks (src/rl.cpp). */
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
EM_JS(int, js_tileset, (void), { return Module.om.tileset(); });
EM_JS(void, js_end, (int saved), { Module.om.end(saved); });
/* src/rl.cpp: the game's side of the page windows */
extern int Rl_at_prompt;
void rl_autosave(void);
void rl_send_lists(void);
int rl_game_end(void);
EM_JS(void, be_prompt, (const char *s), { Module.om.prompt(UTF8ToString(s)); });
/* Inventory and Visible windows (rvip-wm.js): lines from rl_send_lists() */
EM_JS(void, be_lists, (const char *inv, const char *vis), { Module.om.lists(UTF8ToString(inv), UTF8ToString(vis)); });
/* Run report (roguelikes-index/server/CONTRACT.md): fire-and-forget GET,
   never throws, offline just fails silently. Negative ints are omitted. */
EM_JS(void, be_beacon, (const char *g, const char *ev, const char *name, const char *killer, int depth, int score, int turns, int lvl), {
    try {
        var p = [['g', UTF8ToString(g)], ['ev', UTF8ToString(ev)], ['name', name ? UTF8ToString(name) : ''],
                 ['killer', killer ? UTF8ToString(killer) : ''], ['depth', depth], ['score', score], ['turns', turns], ['lvl', lvl]];
        var q = p.filter(function (a) { return a[1] !== '' && !(a[1] < 0); })
                 .map(function (a) { return a[0] + '=' + encodeURIComponent(a[1]); }).join('&');
        if (window.RvipWM && RvipWM.report) RvipWM.report(q); else fetch('/roguelikes/beacon?' + q, { keepalive: true, mode: 'no-cors' }).catch(function () {});
    } catch (e) {}
});

/* exit() (build.sh: -Dexit=wc_exit): Emscripten runs no atexit handlers, so
 * end the page here and idle; the player reloads (ponytail: the wasm stays up).
 * Died or quit: the autosave goes; saved with S: the overlay says so. */
void wc_exit(int code)
{
    (void)code;
    js_end(rl_game_end());
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
void be_flush(void) { rl_send_lists(); js_flush(); }
void be_sleep(int ms) { emscripten_sleep(ms); }
/* src/defs.h omega_sleep(): show the screen, then let the page run */
void wc_sleep_ms(int ms) { doupdate(); emscripten_sleep(ms); }
void be_end(void) { }

int be_getkey(int wait)
{
    int k;
    for (;;) {
        if (js_tileset() != Wc_tileset) { Wc_tileset = js_tileset(); wc_retile(); }
        if (Rl_at_prompt && js_want_save()) {   /* at most every 2 s, at a command key */
            Rl_at_prompt = 0;
            rl_autosave();
            Rl_at_prompt = 1;
        }
        if ((k = js_key(Rl_at_prompt)) >= 0) return k;
        if (!wait) {                /* polling (explore): paint each step */
            emscripten_sleep(40);
            return -1;
        }
        emscripten_sleep(10);
    }
}
