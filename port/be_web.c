/* Browser frontend for the curses shim (RVIP step 7): web/omega.js draws
 * the text screen (Module.om); input waits with Asyncify. omega.sav is an
 * autosave while playing (written at the command prompt when the page asks),
 * and removed when the game ends unless the player saved with S. */
#include <emscripten.h>
#include <stdlib.h>
#include <unistd.h>
#include "curses.h"
#include "../glob.h"

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
EM_JS(void, be_pane, (int p, int y, int x, int r, int c), { Module.om.pane(p, y, x, r, c); });
EM_JS(void, be_pput, (int p, int y, int x, chtype ch), { Module.om.pput(p, y, x, ch); });
EM_JS(void, be_hero, (int y, int x), { Module.om.hero(y, x); });
EM_JS(void, be_popup, (int on), { Module.om.popup(on); });
EM_JS(void, be_msg, (const char *s, int append), { Module.om.msg(UTF8ToString(s), append); });
/* Inventory and Visible windows (rvip-wm.js): lines "<colour>\t<text>" for
 * the inventory; "M<glyph><name>\t<colour>" / "I<glyph><name>\t<colour>"
 * for what the player sees (colours: PC palette indexes, omega.js maps them) */
EM_JS(void, js_lists, (const char *inv, const char *vis), { Module.om.lists(UTF8ToString(inv), UTF8ToString(vis)); });
static void send_lists(void)
{
    static char inv[8192], vis[8192];
    char *p = inv, *e;
    int i, x, y;
    pml ml;
    pol ol;

    *p = 0;
    for (i = 0; i < MAXITEMS; i++)
        if (Player.possessions[i])
            p += sprintf(p, "%d\t%-14.14s %.60s\n", Player.possessions[i]->objchar >> 8 & 15, slotstr(i), itemid(Player.possessions[i]));
    for (i = 0; i < Player.packptr && i < MAXPACK; i++)
        if (Player.pack[i])
            p += sprintf(p, "%d\tpack %c)        %.60s\n", Player.pack[i]->objchar >> 8 & 15, 'a' + i, itemid(Player.pack[i]));
    p = vis; e = vis + sizeof vis - 200; *p = 0;
    if (Level && Current_Environment != E_COUNTRYSIDE && !Player.status[BLINDED]) {
        for (ml = Level->mlist; ml && p < e; ml = ml->next)
            if (ml->m->hp > 0 && view_los_p(Player.x, Player.y, ml->m->x, ml->m->y)
                && (Player.status[TRUESIGHT] || !m_statusp(ml->m, M_INVISIBLE)))
                p += sprintf(p, "M%c%.60s\t%d\n", ml->m->monchar & 0xff, ml->m->monstring, ml->m->monchar >> 8 & 15);
        for (x = 0; x < WIDTH && x < MAXWIDTH; x++)
            for (y = 0; y < LENGTH && y < MAXLENGTH && p < e; y++)
                if (Level->site[x][y].things && view_los_p(Player.x, Player.y, x, y))
                    for (ol = Level->site[x][y].things; ol && p < e; ol = ol->next)
                        p += sprintf(p, "I%c%.80s\t%d\n", ol->thing->objchar & 0xff, itemid(ol->thing), ol->thing->objchar >> 8 & 15);
    }
    js_lists(inv, vis);
}
void be_flush(void) { if (Player.maxhp > 0) send_lists(); js_flush(); }
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
