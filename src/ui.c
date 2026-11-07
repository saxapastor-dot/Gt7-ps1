/* GT7 PS1 - menus, tienda, garaje, tuning, coches usados, ajustes y resultados */
#include <string.h>
#include "ui.h"

/* ---------------------------------------------------------------- texto */
typedef struct { char b[64]; int n; } Line;
static void Lc(Line *l, char c) { if (l->n < 62) { l->b[l->n++] = c; l->b[l->n] = 0; } }
static void Ls(Line *l, const char *s, int w) {            /* texto con ancho fijo (w=0: sin relleno) */
    int k = 0;
    while (s[k] && (w == 0 || k < w)) { Lc(l, s[k]); k++; }
    while (w && k < w) { Lc(l, ' '); k++; }
}
static void Lt(Line *l, const char *s, int max) { for (int k = 0; s[k] && k < max; k++) Lc(l, s[k]); }
static void Ln(Line *l, int v, int w) {                    /* entero alineado a la derecha */
    char t[12]; int n = 0, neg = v < 0; unsigned u = (unsigned)(neg ? -v : v);
    do { t[n++] = (char)('0' + u % 10); u /= 10; } while (u && n < 11);
    if (neg) t[n++] = '-';
    for (int k = n; k < w; k++) Lc(l, ' ');
    while (n) Lc(l, t[--n]);
}
static void Lreset(Line *l) { l->n = 0; l->b[0] = 0; }
static void T(int x, int y, const Line *l) { gfx_text(x, y, l->b); }
static void Tt(int x, int y, const char *s) { gfx_text(x, y, s); }

static int wrap(int v, int n) { return ((v % n) + n) % n; }
static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void say(App *a, const char *m) {
    int k = 0; while (m[k] && k < 38) { a->msg[k] = m[k]; k++; } a->msg[k] = 0; a->msg_timer = 150;
}

/* ------------------------------------------------------ piezas comunes */
static void backdrop(App *a, int theme) {
    Rect full = {0, 0, SCREEN_W, SCREEN_H};
    render_backdrop(theme, WE_SUNNY, a->state == S_TITLE ? TOD_DUSK : TOD_DAY, full);
}

static void header(App *a, const char *title) {
    Line l; ui_panel(4, 4, 312, 22, 10);
    Tt(12, 11, title);
    Lreset(&l); Ls(&l, "CR ", 0); Ln(&l, a->save.credits, 0);
    gfx_text(308 - l.n * 8, 11, l.b);
}

static void footer(App *a, const char *s) {
    ui_panel(4, 214, 312, 22, 10);
    if (a->msg_timer > 0) { Tt(12, 221, a->msg); a->msg_timer--; }
    else Tt(12, 221, s);
}

static void hilite(int x, int y, int w, int h) { gfx_rect(x, y, w, h, 70, 56, 10, 9); gfx_rect(x, y, 2, h, 255, 200, 40, 8); }

static void stat_bars(int x, int y, int w, const Stats *st) {
    Line l;
    Tt(x, y, "VEL"); ui_bar(x + 28, y + 1, w - 28, 6, st->bar_speed, 90, 190, 255);
    Tt(x, y + 11, "ACE"); ui_bar(x + 28, y + 12, w - 28, 6, st->bar_accel, 255, 170, 50);
    Tt(x, y + 22, "AGA"); ui_bar(x + 28, y + 23, w - 28, 6, st->bar_grip, 110, 220, 110);
    Lreset(&l); Ln(&l, st->vmax_kmh, 0); Ls(&l, "km/h PP", 0); Ln(&l, st->pp, 0);
    T(x, y + 33, &l);
    Lreset(&l); Ls(&l, "N2O: ", 0); Lt(&l, st->nitro >= 0 ? NITRO[st->nitro].name : "---", w / 8 - 5);
    T(x, y + 43, &l);
}

static void preview_box(const Owned *o, int x, int y, int w, int h, int angle) {
    Rect r = {x + 1, y + 1, x + w - 1, y + h - 1};
    ui_frame(x, y, w, h, 10);
    render_car_preview(o, r, angle);
}

static const Owned *cur_car(App *a) { return &a->save.own[a->save.cur]; }

/* ------------------------------------------------------------- titulo */
static const uint8_t GLYPH[3][7] = {
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},        /* G */
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},        /* T */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},        /* 7 */
};
static void big_letter(int g, int x, int y, int s, int r, int gg, int b) {
    for (int row = 0; row < 7; row++) for (int col = 0; col < 5; col++)
        if (GLYPH[g][row] & (0x10 >> col)) gfx_rect(x + col * s, y + row * s, s, s, r, gg, b, 4);
}

static void draw_title(App *a) {
    int s = 9, x = (SCREEN_W - (3 * 5 * s + 2 * 10)) / 2;
    backdrop(a, 3);
    gfx_rect(0, 62, SCREEN_W, 80, 8, 10, 20, 14);
    gfx_rect(0, 62, SCREEN_W, 2, 255, 190, 40, 13);
    gfx_rect(0, 140, SCREEN_W, 2, 255, 190, 40, 13);
    for (int i = 0; i < 3; i++) {
        big_letter(i, x + i * (5 * s + 10) + 3, 75 + 3, s, 20, 20, 30);
        big_letter(i, x + i * (5 * s + 10), 75, s, 245, 245, 250);
    }
    for (int k = 0; k < 6; k++) gfx_rect(40 + k * 4, 66, 3, 70, 220, 40, 40, 12);
    Tt(96, 150, "GRAN TURISMO PS1 EDITION");
    if ((a->frame / 30) & 1) Tt(116, 190, "PULSA START");
    Tt(52, 222, "PlayStation 1 - estilo 2020");
}

/* --------------------------------------------------------- menu principal */
static const char *MENU_ITEMS[5] = {"CARRERA RAPIDA", "GARAJE", "TUNING", "CONCESIONARIO", "COCHES USADOS"};

static void draw_menu(App *a) {
    Line l; Stats st; car_stats(cur_car(a), &st);
    backdrop(a, 0);
    header(a, "MENU PRINCIPAL");
    ui_panel(4, 30, 150, 182, 10);
    for (int i = 0; i < 5; i++) {
        int y = 44 + i * 24;
        if (i == a->sel) hilite(8, y - 5, 142, 18);
        Tt(18, y, MENU_ITEMS[i]);
    }
    preview_box(cur_car(a), 158, 30, 158, 88, a->angle);
    ui_panel(158, 120, 158, 92, 10);
    Lreset(&l); Ls(&l, MODELS[cur_car(a)->model].name, 0);
    Tt(166, 126, l.b);
    stat_bars(166, 140, 142, &st);
    Lreset(&l); Ls(&l, "Coches: ", 0); Ln(&l, a->save.nown, 0); Ls(&l, "/", 0); Ln(&l, MAXOWN, 0);
    T(166, 196, &l);
    footer(a, "Arriba/Abajo  X: entrar");
}

/* ------------------------------------------------------------- ajustes */
enum { R_TRACK, R_LAPS, R_WEATHER, R_TOD, R_TYRE, R_NITRO, R_PLAYERS, R_CAR1, R_CAR2, R_GO, NROWS };

static int setup_rows(App *a, int *rows) {
    int n = 0;
    for (int i = 0; i < NROWS; i++) { if (i == R_CAR2 && a->cfg.players < 2) continue; rows[n++] = i; }
    return n;
}

static void draw_setup(App *a) {
    Line l; int rows[NROWS], n = setup_rows(a, rows);
    backdrop(a, a->cfg.track);
    header(a, "CARRERA RAPIDA");
    ui_panel(4, 30, 216, 182, 10);
    for (int k = 0; k < n; k++) {
        int y = 38 + k * 16, r = rows[k];
        if (k == a->sel) hilite(8, y - 4, 208, 15);
        Lreset(&l);
        switch (r) {
            case R_TRACK: Ls(&l, "Pista  ", 0); Ls(&l, TRACK_DEFS[a->cfg.track].name, 0); break;
            case R_LAPS: Ls(&l, "Vueltas", 0); Lc(&l, ' '); Ln(&l, a->cfg.laps, 0); break;
            case R_WEATHER: Ls(&l, "Clima  ", 0); Ls(&l, WEATHER_NAMES[a->cfg.weather], 0); break;
            case R_TOD: Ls(&l, "Hora   ", 0); Ls(&l, TOD_NAMES[a->cfg.tod], 0); break;
            case R_TYRE: Ls(&l, "Neumat.", 0); Lc(&l, ' '); Ls(&l, TYRES[a->cfg.tyre].name, 0); break;
            case R_NITRO: Ls(&l, "Nitro  ", 0); Ls(&l, a->cfg.nitro ? "SI" : "NO", 0); break;
            case R_PLAYERS: Ls(&l, "Jugad.  ", 0); Ln(&l, a->cfg.players, 0); Ls(&l, a->cfg.players == 2 ? " pant. dividida" : "", 0); break;
            case R_CAR1: Ls(&l, "Coche 1", 0); Lc(&l, ' '); Ls(&l, MODELS[a->save.own[a->save.cur].model].name, 0); break;
            case R_CAR2: Ls(&l, "Coche 2", 0); Lc(&l, ' '); Ls(&l, MODELS[a->save.own[a->car2].model].name, 0); break;
            default: Ls(&l, "*** EMPEZAR CARRERA ***", 0); break;
        }
        T(14, y, &l);
        if (r != R_GO && r != R_PLAYERS && r != R_CAR1 && r != R_CAR2) { }
    }
    ui_panel(224, 30, 92, 182, 10);
    ui_trackmap(a->cfg.track, 227, 36, 86, 60);
    Lreset(&l); Ls(&l, "Largo ", 0); Ln(&l, TRACK_DEFS[a->cfg.track].length_m, 0); Ls(&l, "m", 0); T(228, 104, &l);
    Lreset(&l); Ls(&l, "Premio ", 0); Ln(&l, TRACK_DEFS[a->cfg.track].reward * a->cfg.laps / TRACK_DEFS[a->cfg.track].laps, 0); T(228, 116, &l);
    Tt(228, 138, "Tu coche:");
    Stats st; car_stats(&a->save.own[a->save.cur], &st);
    Lreset(&l); Ln(&l, st.vmax_kmh, 0); Ls(&l, "km/h", 0); T(228, 150, &l);
    Lreset(&l); Ls(&l, "Rivales: ", 0); Ln(&l, a->cfg.players == 2 ? 4 : 5, 0); T(228, 170, &l);
    Tt(228, 182, "igualados a");
    Tt(228, 192, "tu coche");
    footer(a, "Izq/Der: cambiar  O: volver");
}

static void setup_change(App *a, int row, int d) {
    switch (row) {
        case R_TRACK: a->cfg.track = wrap(a->cfg.track + d, NTRACKS); a->cfg.laps = TRACK_DEFS[a->cfg.track].laps; break;
        case R_LAPS: a->cfg.laps = clampi(a->cfg.laps + d, 1, 20); break;
        case R_WEATHER: a->cfg.weather = wrap(a->cfg.weather + d, NWEATHER); break;
        case R_TOD: a->cfg.tod = wrap(a->cfg.tod + d, NTOD); break;
        case R_TYRE: a->cfg.tyre = wrap(a->cfg.tyre + d, NTYRES); break;
        case R_NITRO: a->cfg.nitro ^= 1; break;
        case R_PLAYERS: a->cfg.players = a->cfg.players == 1 ? 2 : 1; break;
        case R_CAR1: a->save.cur = wrap(a->save.cur + d, a->save.nown); break;
        case R_CAR2: a->car2 = wrap(a->car2 + d, a->save.nown); break;
    }
}

static void start_race(App *a) {
    const Owned *p1 = &a->save.own[a->save.cur], *p2 = &a->save.own[a->car2];
    rnd_seed((uint32_t)(a->frame * 2654435761u) ^ (uint32_t)a->save.races * 7919u ^ 0xA5A5u);
    a->cfg.nitro = a->cfg.nitro ? 1 : 0;
    race_init(&a->race, &a->cfg, p1, a->cfg.players == 2 ? p2 : 0);
    a->cam[0].init = a->cam[1].init = 0;
    a->cam[0].mode = a->cam[1].mode = 0;
    a->paused = 0; a->pause_sel = 0; a->result_wait = 0;
    a->state = S_RACE;
}

/* -------------------------------------------------------------- garaje */
static void draw_garage(App *a) {
    Line l; Stats st; const Save *s = &a->save;
    backdrop(a, 0);
    header(a, "GARAJE");
    ui_panel(4, 30, 170, 182, 10);
    for (int i = 0; i < s->nown; i++) {
        int y = 36 + i * 21; const Owned *o = &s->own[i];
        if (i == a->sel) hilite(8, y - 3, 162, 20);
        Lreset(&l); Lc(&l, i == s->cur ? '*' : ' '); Ls(&l, MODELS[o->model].name, 15); T(14, y, &l);
        Lreset(&l); Ln(&l, o->cond, 3); Ls(&l, "% ", 0); Ln(&l, o->km, 0); Ls(&l, "km", 0); T(22, y + 9, &l);
    }
    const Owned *o = &s->own[a->sel]; car_stats(o, &st);
    preview_box(o, 178, 30, 138, 84, a->angle);
    ui_panel(178, 116, 138, 96, 10);
    uint8_t rgb[3]; owned_rgb(o, rgb);
    gfx_rect(184, 122, 14, 8, rgb[0], rgb[1], rgb[2], 6);
    Lreset(&l); Ls(&l, "Color ", 0); Ln(&l, o->color >= 12 ? 13 : o->color + 1, 0); Ls(&l, "/13", 0);
    T(202, 122, &l);
    Lreset(&l); Ls(&l, "Aleron: ", 0); Ls(&l, o->spoiler ? "SI" : "NO", 0); T(184, 133, &l);
    stat_bars(184, 144, 126, &st);
    Lreset(&l); Ls(&l, "Valor ", 0); Ln(&l, owned_value(o), 0);
    T(184, 198, &l);
    if (a->confirm) { gfx_rect(8, 190, 164, 18, 120, 20, 20, 5); Tt(12, 195, "/\\ otra vez: VENDER"); }
    footer(a, "X:usar <>:color []:aleron /\\:vender");
}

/* -------------------------------------------------------------- tuning */
static int cat_first(int cat) { for (int i = 0; i < NPARTS; i++) if (PARTS[i].cat == cat) return i; return 0; }
static int cat_count(int cat) { int n = 0; for (int i = 0; i < NPARTS; i++) if (PARTS[i].cat == cat) n++; return n; }

static void draw_tune(App *a) {
    Line l; Stats st; const Owned *o = cur_car(a); car_stats(o, &st);
    backdrop(a, 2);
    Lreset(&l); Ls(&l, "TUNING <L1 ", 0); Ls(&l, CAT_NAMES[a->cat], 0); Ls(&l, " R1>", 0);
    header(a, l.b);
    ui_panel(4, 30, 224, 182, 10);
    int first = cat_first(a->cat), cnt = cat_count(a->cat);
    for (int k = 0; k < cnt; k++) {
        int p = first + k, y = 36 + k * 14;
        if (k == a->sel) hilite(8, y - 3, 216, 13);
        Lreset(&l); Ls(&l, o->parts[p] ? "[X]" : "[ ]", 0); Lc(&l, ' '); Ls(&l, PARTS[p].name, 16); Ln(&l, PARTS[p].price, 6);
        T(12, y, &l);
    }
    ui_panel(232, 78, 84, 134, 10);
    preview_box(o, 232, 30, 84, 46, a->angle);
    Tt(236, 82, "VEL"); ui_bar(262, 83, 48, 5, st.bar_speed, 90, 190, 255);
    Tt(236, 92, "ACE"); ui_bar(262, 93, 48, 5, st.bar_accel, 255, 170, 50);
    Tt(236, 102, "AGA"); ui_bar(262, 103, 48, 5, st.bar_grip, 110, 220, 110);
    Lreset(&l); Ln(&l, st.vmax_kmh, 0); Ls(&l, "km/h", 0); T(236, 114, &l);
    const Part *pp = &PARTS[first + a->sel];
    Tt(236, 134, "Pieza:");
    Lreset(&l); Ls(&l, "Vel +", 0); Ln(&l, pp->gs, 0); T(236, 146, &l);
    Lreset(&l); Ls(&l, "Ace +", 0); Ln(&l, pp->ga, 0); T(236, 156, &l);
    Lreset(&l); Ls(&l, "Man +", 0); Ln(&l, pp->gh, 0); T(236, 166, &l);
    if (pp->gn) { Lreset(&l); Ls(&l, "N2O +", 0); Ln(&l, pp->gn, 0); T(236, 176, &l); }
    Lreset(&l); Ls(&l, "Venta ", 0); Ln(&l, pp->price * 40 / 100, 0); T(236, 196, &l);
    footer(a, "X:comprar []:vender L1/R1:categoria");
}

/* ------------------------------------------------------ concesionario */
static void draw_dealer(App *a) {
    Line l; Stats st; Owned tmp;
    backdrop(a, 1);
    header(a, "CONCESIONARIO");
    ui_panel(4, 30, 190, 182, 10);
    for (int i = 0; i < NMODELS; i++) {
        int y = 38 + i * 21;
        if (i == a->sel) hilite(8, y - 4, 182, 20);
        Lreset(&l); Ls(&l, MODELS[i].name, 15); Lc(&l, ' '); Ln(&l, MODELS[i].price, 5); T(14, y, &l);
        Lreset(&l); Ls(&l, MODELS[i].shape == SH_F1 ? "Monoplaza" : (MODELS[i].shape == SH_KART ? "Kart" : "Deportivo"), 0);
        T(22, y + 9, &l);
    }
    memset(&tmp, 0, sizeof tmp); tmp.model = (uint8_t)a->sel; tmp.color = 12; tmp.cond = 100; car_stats(&tmp, &st);
    preview_box(&tmp, 198, 30, 118, 78, a->angle);
    ui_panel(198, 110, 118, 102, 10);
    stat_bars(204, 118, 106, &st);
    Lreset(&l); Ls(&l, "Precio ", 0); Ln(&l, MODELS[a->sel].price, 0); T(204, 172, &l);
    Lreset(&l); Ls(&l, "Tienes ", 0); Ln(&l, a->save.credits, 0); T(204, 184, &l);
    Lreset(&l); Ls(&l, "Garaje ", 0); Ln(&l, a->save.nown, 0); Ls(&l, "/", 0); Ln(&l, MAXOWN, 0); T(204, 196, &l);
    footer(a, "X: comprar coche nuevo");
}

/* ------------------------------------------------------------- usados */
static void draw_used(App *a) {
    Line l; Stats st; Owned tmp; const Listing *li = &a->save.used[a->sel];
    backdrop(a, 3);
    header(a, "COCHES DE SEGUNDA MANO");
    ui_panel(4, 30, 190, 182, 10);
    for (int i = 0; i < NLIST; i++) {
        const Listing *q = &a->save.used[i]; int y = 36 + i * 29;
        if (i == a->sel) hilite(8, y - 3, 182, 27);
        Lreset(&l);
        if (q->price <= 0) { Ls(&l, "-- VENDIDO --", 0); T(14, y + 6, &l); continue; }
        Ls(&l, MODELS[q->model].name, 15); Lc(&l, ' '); Ln(&l, q->price, 5); T(14, y, &l);
        Lreset(&l); Ln(&l, q->cond, 3); Ls(&l, "% ", 0); Ln(&l, q->km, 0); Ls(&l, "km +", 0); Ln(&l, q->nparts, 0); Ls(&l, "pz", 0);
        T(14, y + 10, &l);
        Lreset(&l); Ls(&l, "Nuevo: ", 0); Ln(&l, MODELS[q->model].price, 0); T(14, y + 19 - 1, &l);
    }
    memset(&tmp, 0, sizeof tmp); tmp.model = li->model; tmp.color = li->color; tmp.cond = li->cond; tmp.km = li->km;
    for (int k = 0; k < li->nparts; k++) tmp.parts[li->parts[k]] = 1;
    car_stats(&tmp, &st);
    preview_box(&tmp, 198, 30, 118, 70, a->angle);
    ui_panel(198, 102, 118, 110, 10);
    stat_bars(204, 108, 106, &st);
    for (int k = 0; k < li->nparts && k < 4; k++) { Lreset(&l); Ls(&l, PARTS[li->parts[k]].name, 14); T(204, 160 + k * 10, &l); }
    Lreset(&l); Ls(&l, "Tienes ", 0); Ln(&l, a->save.credits, 0); T(204, 200, &l);
    footer(a, "X: comprar (cambia cada carrera)");
}

/* ----------------------------------------------------------- resultados */
static void draw_result(App *a) {
    Line l; const Race *r = &a->race; char tb[12];
    backdrop(a, r->track);
    header(a, "RESULTADOS");
    ui_panel(4, 30, 312, 182, 10);
    Tt(14, 38, "POS  PILOTO / COCHE         TIEMPO");
    for (int p = 0; p < r->ncars; p++) for (int i = 0; i < r->ncars; i++) if (r->car[i].rank == p) {
        const Car *c = &r->car[i]; int y = 54 + p * 16;
        if (!c->is_ai) hilite(8, y - 3, 304, 14);
        Lreset(&l); Ln(&l, p + 1, 2); Ls(&l, "   ", 0);
        if (c->is_ai) { Ls(&l, "CPU ", 0); } else { Ls(&l, "J", 0); Ln(&l, i + 1, 0); Ls(&l, "  ", 0); }
        Ls(&l, MODELS[c->model].name, 16);
        if (c->finished) { format_time(tb, c->finish_time); Lc(&l, ' '); Ls(&l, tb, 0); } else Ls(&l, " --", 0);
        T(14, y, &l);
    }
    Lreset(&l); Ls(&l, "Premio: ", 0); Ln(&l, r->reward, 0); Ls(&l, " cr", 0); T(14, 158, &l);
    if (r->car[0].best_lap) { format_time(tb, r->car[0].best_lap); Lreset(&l); Ls(&l, "Mejor vuelta J1: ", 0); Ls(&l, tb, 0); T(14, 172, &l); }
    Lreset(&l); Ls(&l, "Combustible J1: ", 0); Ln(&l, r->car[0].fuel / 6000, 0); Ls(&l, "%  Neumaticos: ", 0); Ln(&l, 100 - r->car[0].wear / 1000, 0); Ls(&l, "%", 0);
    T(14, 186, &l);
    footer(a, "X o START: volver al menu");
}

/* ---------------------------------------------------------- pad -> coche */
static int steer_from(const Pad *p) {
    int s = 0;
    if (p->stick > 14 || p->stick < -14) { s = p->stick * 2; if (s > 256) s = 256; if (s < -256) s = -256; }
    if (p->held & BT_LEFT) s = -256;
    if (p->held & BT_RIGHT) s = 256;
    return s;
}

static void draw_pause(App *a) {
    static const char *it[3] = {"Reanudar", "Reiniciar carrera", "Abandonar"};
    ui_panel(80, 70, 160, 84, 2);
    Tt(116, 78, "PAUSA");
    for (int i = 0; i < 3; i++) {
        if (i == a->pause_sel) hilite(86, 94 + i * 18 - 3, 148, 14);
        Tt(96, 94 + i * 18, it[i]);
    }
}

/* ============================================================ bucle */
void app_init(App *a) {
    memset(a, 0, sizeof *a);
    math_init();
    save_default(&a->save);
    a->cfg.track = 0; a->cfg.laps = TRACK_DEFS[0].laps; a->cfg.weather = WE_SUNNY; a->cfg.tod = TOD_DAY;
    a->cfg.nitro = 1; a->cfg.tyre = TY_MEDIUM; a->cfg.players = 1;
    a->state = S_TITLE;
}

static void go(App *a, int st) { a->state = st; a->sel = 0; a->confirm = 0; a->msg_timer = 0; }

void app_frame(App *a, const Pad pads[2], int nticks) {
    const Pad *p = &pads[0]; Save *s = &a->save;
    a->frame++; a->angle = (a->angle + 14) & 4095;
    int d = (p->down & BT_RIGHT) ? 1 : ((p->down & BT_LEFT) ? -1 : 0);
    int ud = (p->down & BT_DOWN) ? 1 : ((p->down & BT_UP) ? -1 : 0);

    switch (a->state) {
    case S_TITLE:
        draw_title(a);
        if (p->down & (BT_START | BT_A)) go(a, S_MENU);
        break;
    case S_MENU:
        a->sel = wrap(a->sel + ud, 5);
        draw_menu(a);
        if (p->down & BT_A) {
            switch (a->sel) {
                case 0: go(a, S_SETUP); a->car2 = s->nown > 1 ? (s->cur + 1) % s->nown : s->cur; break;
                case 1: go(a, S_GARAGE); a->sel = s->cur; break;
                case 2: go(a, S_TUNE); a->cat = 0; break;
                case 3: go(a, S_DEALER); break;
                default: go(a, S_USED); break;
            }
        }
        if (p->down & BT_B) go(a, S_TITLE);
        break;
    case S_SETUP: {
        int rows[NROWS], n = setup_rows(a, rows);
        a->sel = wrap(a->sel + ud, n);
        if (a->sel >= n) a->sel = n - 1;
        if (d) setup_change(a, rows[a->sel], d);
        if (a->cfg.players == 2 && a->car2 >= s->nown) a->car2 = 0;
        draw_setup(a);
        if ((p->down & BT_A) && rows[a->sel] == R_GO) start_race(a);
        else if ((p->down & BT_A) && rows[a->sel] == R_NITRO) setup_change(a, R_NITRO, 1);
        if (p->down & BT_B) go(a, S_MENU);
        break; }
    case S_GARAGE: {
        Owned *o;
        a->sel = wrap(a->sel + ud, s->nown);
        o = &s->own[a->sel];
        if (d) { o->color = (uint8_t)wrap((o->color >= 12 ? 12 : o->color) + d, 13); }
        if (p->down & BT_X) o->spoiler ^= 1;
        if (p->down & BT_A) { s->cur = a->sel; say(a, "Coche seleccionado"); }
        if (ud) a->confirm = 0;
        if (p->down & BT_Y) {
            if (s->nown <= 1) say(a, "No puedes vender tu ultimo coche");
            else if (!a->confirm) a->confirm = 1;
            else { int v = owned_value(o); sell_car(s, a->sel); a->sel = clampi(a->sel, 0, s->nown - 1); a->confirm = 0; say(a, "Coche vendido"); (void)v; }
        }
        draw_garage(a);
        if (p->down & BT_B) go(a, S_MENU);
        break; }
    case S_TUNE: {
        int cnt;
        if (p->down & BT_R1) { a->cat = wrap(a->cat + 1, NCAT); a->sel = 0; }
        if (p->down & BT_L1) { a->cat = wrap(a->cat - 1, NCAT); a->sel = 0; }
        cnt = cat_count(a->cat);
        a->sel = wrap(a->sel + ud, cnt);
        if (p->down & BT_A) {
            int part = cat_first(a->cat) + a->sel;
            if (s->own[s->cur].parts[part]) say(a, "Ya instalada ([] para vender)");
            else if (buy_part(s, s->cur, part)) say(a, "Pieza instalada");
            else say(a, "No tienes creditos suficientes");
        }
        if (p->down & BT_X) {
            int part = cat_first(a->cat) + a->sel;
            if (sell_part(s, s->cur, part)) say(a, "Pieza vendida");
        }
        draw_tune(a);
        if (p->down & BT_B) go(a, S_MENU);
        break; }
    case S_DEALER:
        a->sel = wrap(a->sel + ud, NMODELS);
        if (p->down & BT_A) {
            if (s->nown >= MAXOWN) say(a, "Garaje lleno: vende un coche");
            else if (dealer_buy(s, a->sel)) say(a, "Coche comprado y equipado");
            else say(a, "No tienes creditos suficientes");
        }
        draw_dealer(a);
        if (p->down & BT_B) go(a, S_MENU);
        break;
    case S_USED:
        a->sel = wrap(a->sel + ud, NLIST);
        if (p->down & BT_A) {
            if (s->used[a->sel].price <= 0) say(a, "Ese coche ya esta vendido");
            else if (s->nown >= MAXOWN) say(a, "Garaje lleno: vende un coche");
            else if (used_buy(s, a->sel)) say(a, "Coche usado comprado");
            else say(a, "No tienes creditos suficientes");
        }
        draw_used(a);
        if (p->down & BT_B) go(a, S_MENU);
        break;
    case S_RACE: {
        Race *r = &a->race;
        if (p->down & BT_START || (pads[1].down & BT_START && r->nhumans == 2)) { a->paused = !a->paused; a->pause_sel = 0; }
        if (a->paused) {
            a->pause_sel = wrap(a->pause_sel + ud, 3);
            if (p->down & BT_A) {
                if (a->pause_sel == 0) a->paused = 0;
                else if (a->pause_sel == 1) start_race(a);
                else { r->state = 2; a->result_wait = 999; a->paused = 0; }
            }
        } else {
            for (int h = 0; h < r->nhumans; h++) if (pads[h].down & BT_SELECT) a->cam[h].mode = (a->cam[h].mode + 1) % 3;
            for (int t = 0; t < nticks; t++) {
                PadIn in[2] = {{0}};
                for (int h = 0; h < r->nhumans; h++) {
                    const Pad *q = &pads[h];
                    in[h].thr = (q->held & (BT_A | BT_R2)) ? 256 : 0;
                    in[h].brk = (q->held & (BT_X | BT_L2)) ? 256 : 0;
                    in[h].steer = steer_from(q);
                    in[h].nitro = (q->held & (BT_R1 | BT_B)) ? 1 : 0;
                    if (t == 0) { in[h].pit = (q->down & BT_Y) ? 1 : 0; in[h].tyre = (q->down & BT_L1) ? 1 : 0; }
                }
                race_tick(r, in);
            }
            if (r->state == 2) a->result_wait += nticks;
        }
        for (int h = 0; h < r->nhumans; h++) cam_update(&a->cam[h], &r->car[h]);
        if (r->nhumans == 1) {
            Rect vp = {0, 0, SCREEN_W, SCREEN_H};
            render_world(r, 0, &a->cam[0], vp, a->frame); render_hud(r, 0, vp, 0, a->frame);
        } else {
            Rect v0 = {0, 0, SCREEN_W, 119}, v1 = {0, 121, SCREEN_W, SCREEN_H};
            render_world(r, 0, &a->cam[0], v0, a->frame); render_hud(r, 0, v0, 1, a->frame);
            render_world(r, 1, &a->cam[1], v1, a->frame); render_hud(r, 1, v1, 1, a->frame);
            gfx_rect(0, 119, SCREEN_W, 2, 0, 0, 0, 1);
        }
        if (a->paused) draw_pause(a);
        if (r->state == 2 && (a->result_wait > 180 || (p->down & BT_A && r->car[0].finished))) {
            race_finish_save(r, s); a->state = S_RESULT; a->sel = 0;
        }
        break; }
    case S_RESULT:
        draw_result(a);
        if (p->down & (BT_A | BT_START)) go(a, S_MENU);
        break;
    }
}
