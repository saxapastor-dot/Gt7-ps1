/* GT7 PS1 - logica del juego. Solo enteros (la PS1 no tiene FPU). */
#include "game.h"

/* ====================================================================== */
/* Tablas de datos (valores tomados del HTML original)                      */
/* ====================================================================== */
const CarModel MODELS[NMODELS] = {
    {"Porsche 911 GT3",  9000, 265, 100, 100, 255, 215,   0, SH_GT},
    {"Ferrari 488 GTB", 15000, 300, 110,  95, 211,  47,  47, SH_SUPER},
    {"McLaren 720S",    20000, 310, 115,  90, 244, 143,   0, SH_SUPER},
    {"Mercedes AMG GT", 17000, 290, 105, 105,  17,  17,  17, SH_GT},
    {"Aston Martin",    24000, 295, 108, 102,  21, 198,  31, SH_GT},
    {"Rayo McQueen",    28000, 320, 125,  95, 255,   0,   0, SH_TOON},
    {"Formula 1",       45000, 360, 140,  80,   0, 102,  51, SH_F1},
    {"Kart",            38000, 380, 150,  82, 255,   0,   0, SH_KART},
};

const char *CAT_NAMES[NCAT] = {"MOTOR", "TRANSMISION", "SUSPENSION", "FRENOS",
                               "NEUMATICOS", "CARROCERIA", "ESPECIALES"};

/* nombre, precio, ganancias (vel, acel, manejo, nitro), categoria, nitro */
const Part PARTS[NPARTS] = {
    /* MOTOR */
    {"Filtro de Aire",          500,  2,  3,  0,  0, 0, -1},
    {"Escape Deportivo",        900,  3,  2,  0,  0, 0, -1},
    {"Reprogramacion ECU",     1200,  3,  4,  0,  0, 0, -1},
    {"Turbo Nivel I",          2500,  4,  8,  0,  0, 0, -1},
    {"Turbo Nivel II",         4500,  8, 15,  0,  0, 0, -1},
    {"Turbo Nivel III",        8500, 14, 24,  0,  0, 0, -1},
    {"Compresor Volumetrico",  7500, 10, 18,  0,  0, 0, -1},
    {"Motor Fase I",           5000, 10,  6,  0,  0, 0, -1},
    {"Motor Fase II",         10000, 18, 10,  0,  0, 0, -1},
    {"Motor Fase III",        18000, 28, 16,  0,  0, 0, -1},
    {"Pistones de Titanio",   12000,  8, 12,  0,  0, 0, -1},
    {"Turbo Ultra-RPM",       22000, 18, 30,  0,  0, 0, -1},
    /* TRANSMISION */
    {"Embrague Deportivo",      600,  0,  3,  0,  0, 1, -1},
    {"Caja Deportiva",         1800,  3,  4,  0,  0, 1, -1},
    {"Diferencial LSD 1 Via",  2500,  0,  0,  8,  0, 1, -1},
    {"Diferencial LSD 2 Vias", 5000,  0,  4, 14,  0, 1, -1},
    {"Arbol de Carbono",       7500,  4,  6,  0,  0, 1, -1},
    /* SUSPENSION */
    {"Susp. Deportiva",        1000,  0,  0,  6,  0, 2, -1},
    {"Susp. Semi-Racing",      2800,  0,  0, 12,  0, 2, -1},
    {"Susp. Racing",           5500,  3,  0, 20,  0, 2, -1},
    {"Susp. Ajustable",       10000,  5,  0, 30,  0, 2, -1},
    /* FRENOS */
    {"Frenos Deportivos",       500,  0,  0,  4,  0, 3, -1},
    {"Frenos Semi-Racing",     2200,  0,  0,  9,  0, 3, -1},
    {"Frenos Racing Brembo",   5500,  2,  0, 16,  0, 3, -1},
    /* NEUMATICOS */
    {"Neumaticos Deportivos",   800,  2,  0,  8,  0, 4, -1},
    {"Neumaticos Semi-Racing", 2000,  0,  4, 16,  0, 4, -1},
    {"Neumaticos Slick",       4000,  4,  8, 26,  0, 4, -1},
    /* CARROCERIA */
    {"Reduccion Peso I",       1200,  4,  3,  0,  0, 5, -1},
    {"Reduccion Peso II",      2800,  8,  5,  0,  0, 5, -1},
    {"Reduccion Peso III",     6000, 14,  8,  0,  0, 5, -1},
    {"Reduccion Peso IV",     12000, 20, 12,  0,  0, 5, -1},
    {"Reduccion Peso V",      22000, 28, 18,  0,  0, 5, -1},
    {"Aleron Deportivo",       1200,  2,  0,  6,  0, 5, -1},
    {"Aleron Racing GT",       3500,  4,  0, 12,  0, 5, -1},
    {"Splitter Delantero",     1800,  0,  0,  8,  0, 5, -1},
    /* ESPECIALES (nitros) */
    {"Nitro Chispa Rapida",     800,  2,  0,  0, 10, 6, 0},
    {"Nitro N2O Estandar",     1500,  5,  0,  0, 25, 6, 1},
    {"Nitro Recarga Relamp.",  2200,  4,  0,  0, 20, 6, 2},
    {"Nitro Maratoniano",      3000,  4,  0,  0, 60, 6, 3},
    {"Nitro Inyeccion Dual",   4000,  8,  8,  0, 35, 6, 4},
    {"Nitro GT Racing",        7000, 12, 10,  0, 50, 6, 5},
    {"Nitro Kaos Total",      12000, 18, 16,  0, 80, 6, 6},
    {"Jaula Antivuelco",       3000,  3,  0,  6,  0, 6, -1},
};

const NitroDef NITRO[7] = {
    {"Chispa Rapida",   50,  90, 40, 10,  7},
    {"N2O Estandar",   100,  50, 12, 25, 15},
    {"Recarga Relamp.", 75,  60, 38, 20, 12},
    {"Maratoniano",    160,  22,  5, 22, 13},
    {"Inyeccion Dual", 120,  42, 14, 35, 22},
    {"GT Racing",      100,  32, 10, 50, 32},
    {"Kaos Total",      80,  28,  7, 70, 50},
};

const TyreDef TYRES[NTYRES] = {
    {"Blandos",    103, 130, 255,  40,  40},
    {"Medios",     100, 100, 255, 220,  40},
    {"Duros",       85,  60, 120, 160, 255},
    {"Intermedios", 90, 110,  60, 220,  80},
    {"Lluvia",      80, 130,  40,  80, 200},
};

const uint8_t PALETTE[12][3] = {
    {220, 30, 30}, {255,130, 20}, {250,215, 40}, { 40,170, 60}, { 30,190,200}, { 40, 90,220},
    {130, 60,200}, {240,110,170}, {240,240,240}, {160,165,175}, { 25, 25, 28}, {212,175, 55}};

const char *WEATHER_NAMES[NWEATHER] = {"Soleado", "Lluvia", "Tormenta", "Niebla"};
const char *TOD_NAMES[NTOD] = {"Dia", "Atardecer", "Noche"};

/* ====================================================================== */
/* Matematicas                                                              */
/* ====================================================================== */
static int16_t sintab[4096];
static uint32_t rng_state = 0x1234567u;

void math_init(void) {
    /* seno por Taylor en punto fijo Q30 (solo se calcula al arrancar) */
    const int64_t PI_Q30 = 3373259426LL;                 /* pi * 2^30 */
    for (int a = 0; a <= 1024; a++) {
        int64_t x  = (PI_Q30 * a) >> 11;                 /* a * pi/2048 */
        int64_t x2 = (x * x) >> 30;
        int64_t t = (1LL << 30);
        t = (1LL << 30) - ((x2 * t / 110) >> 30);
        t = (1LL << 30) - ((x2 * t / 72) >> 30);
        t = (1LL << 30) - ((x2 * t / 42) >> 30);
        t = (1LL << 30) - ((x2 * t / 20) >> 30);
        t = (1LL << 30) - ((x2 * t / 6) >> 30);
        int64_t sn = (x * t) >> 30;
        int v = (int)((sn * 16384 + (1LL << 29)) >> 30);
        if (v > 16384) v = 16384;
        sintab[a] = (int16_t)v;
        sintab[2048 - a] = (int16_t)v;
        sintab[2048 + a] = (int16_t)(-v);
        if (a > 0) sintab[4096 - a] = (int16_t)(-v);
    }
}
int isin(int a) { return (sintab[a & 4095] + (sintab[a & 4095] >= 0 ? 2 : -2)) / 4; }
int icos(int a) { return isin(a + 1024); }
int isin14(int a) { return sintab[a & 4095]; }
int icos14(int a) { return sintab[(a + 1024) & 4095]; }

int isqrt32(uint32_t v) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return (int)r;
}

int iatan2(int y, int x) {
    if (!x && !y) return 0;
    int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a, r12;
    {   /* escalado para evitar divisiones de 64 bits (lentas en PS1) */
        int mx = ax >= ay ? ax : ay, mn = ax >= ay ? ay : ax;
        while (mx >= (1 << 19)) { mx >>= 1; mn >>= 1; }
        r12 = (int)(((uint32_t)mn << 12) / (uint32_t)mx);
    }
    a = ((512 * r12) >> 12) + (int)(((uint32_t)178 * (uint32_t)r12 * (uint32_t)(4096 - r12)) >> 24);
    if (ax < ay) a = 1024 - a;            /* 0..1024 = primer cuadrante */
    if (x < 0) a = 2048 - a;
    if (y < 0) a = 4096 - a;
    return a & 4095;
}

void rnd_seed(uint32_t s) { rng_state = s ? s : 0x9E3779B9u; }
uint32_t rnd(void) {
    uint32_t x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rng_state = x;
}
int rnd_range(int n) { return (int)((rnd() >> 8) % (uint32_t)n); }

/* ====================================================================== */
/* Coches, tuning y economia                                                */
/* ====================================================================== */
void owned_rgb(const Owned *o, uint8_t rgb[3]) {
    if (o->color >= 12) { rgb[0] = MODELS[o->model].r; rgb[1] = MODELS[o->model].g; rgb[2] = MODELS[o->model].b; }
    else { rgb[0] = PALETTE[o->color][0]; rgb[1] = PALETTE[o->color][1]; rgb[2] = PALETTE[o->color][2]; }
}

static void owned_new(Owned *o, int model) {
    for (unsigned i = 0; i < sizeof(*o); i++) ((uint8_t *)o)[i] = 0;
    o->model = (uint8_t)model; o->color = 12; o->cond = 100; o->km = 0;
}

void save_default(Save *s) {
    for (unsigned i = 0; i < sizeof(*s); i++) ((uint8_t *)s)[i] = 0;
    s->credits = 10000;
    s->nown = 1; s->cur = 0;
    owned_new(&s->own[0], CAR_PORSCHE);
    s->mseed = 0xC0FFEEu;
    market_refresh(s);
}

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int iabs(int a) { return a < 0 ? -a : a; }
static int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void car_stats(const Owned *o, Stats *st) {
    const CarModel *m = &MODELS[o->model];
    int sS = 0, sA = 0, sH = 0, nitro = -1, nprio = -1;
    for (int i = 0; i < NPARTS; i++) {
        if (!o->parts[i]) continue;
        sS += PARTS[i].gs; sA += PARTS[i].ga; sH += PARTS[i].gh;
        if (PARTS[i].nitro > nprio) { nprio = PARTS[i].nitro; nitro = PARTS[i].nitro; }
    }
    int perf = 80 + o->cond / 5;                         /* 80..100 % */
    st->vmax_kmh = m->vmax_kmh * (100 + imin(sS, 150) * 22 / 100) / 100 * perf / 100;
    st->accel    = m->accel   * (100 + imin(sA, 160) * 45 / 100) / 100 * perf / 100;
    st->grip     = m->handling * (100 + imin(sH, 160) * 25 / 100) / 100 * perf / 100;
    st->bar_speed = iclamp(st->vmax_kmh * 100 / 480, 0, 100);
    st->bar_accel = iclamp(st->accel * 100 / 260, 0, 100);
    st->bar_grip  = iclamp(st->grip * 100 / 150, 0, 100);
    st->nitro = nitro;
    st->pp = st->vmax_kmh + st->accel + st->grip;
}

int parts_total(const Owned *o) {
    int t = 0;
    for (int i = 0; i < NPARTS; i++) if (o->parts[i]) t += PARTS[i].price;
    return t;
}

int owned_value(const Owned *o) {
    int base = MODELS[o->model].price * 60 / 100 * (50 + o->cond / 2) / 100;
    return base + parts_total(o) * 40 / 100;
}

int buy_part(Save *s, int slot, int part) {
    Owned *o = &s->own[slot];
    if (o->parts[part] || s->credits < PARTS[part].price) return 0;
    s->credits -= PARTS[part].price; o->parts[part] = 1;
    return 1;
}
int sell_part(Save *s, int slot, int part) {
    Owned *o = &s->own[slot];
    if (!o->parts[part]) return 0;
    o->parts[part] = 0; s->credits += PARTS[part].price * 40 / 100;
    return 1;
}
int dealer_buy(Save *s, int model) {
    if (s->nown >= MAXOWN || s->credits < MODELS[model].price) return 0;
    s->credits -= MODELS[model].price;
    owned_new(&s->own[s->nown], model);
    s->cur = s->nown++;
    return 1;
}
int sell_car(Save *s, int slot) {
    if (s->nown <= 1 || slot < 0 || slot >= s->nown) return 0;
    s->credits += owned_value(&s->own[slot]);
    for (int i = slot; i < s->nown - 1; i++) s->own[i] = s->own[i + 1];
    s->nown--;
    if (s->cur >= s->nown) s->cur = s->nown - 1;
    else if (s->cur > slot) s->cur--;
    return 1;
}

void market_refresh(Save *s) {
    rnd_seed(s->mseed);
    for (int i = 0; i < NLIST; i++) {
        Listing *l = &s->used[i];
        l->model = (uint8_t)rnd_range(NMODELS);
        l->cond = (uint8_t)(55 + rnd_range(42));
        l->km = (uint16_t)((100 - l->cond) * 1300 + rnd_range(5000));
        l->color = (uint8_t)(rnd_range(3) == 0 ? rnd_range(12) : 12);
        l->nparts = (uint8_t)rnd_range(5);
        int ptotal = 0;
        for (int k = 0; k < l->nparts; k++) {
            int p, tries = 0, dup;
            do {
                p = rnd_range(NPARTS - 3); dup = 0; tries++;           /* sin nitros caros */
                for (int q = 0; q < k; q++) if (l->parts[q] == p) dup = 1;
            } while ((PARTS[p].price > 5000 || dup) && tries < 40);
            if (PARTS[p].price > 5000 || dup) { l->nparts = (uint8_t)k; break; }
            l->parts[k] = (uint8_t)p; ptotal += PARTS[p].price;
        }
        l->price = MODELS[l->model].price * 75 / 100 * (40 + l->cond * 60 / 100) / 100 + ptotal * 60 / 100;
        l->price = (l->price + 49) / 50 * 50;
    }
}

int used_buy(Save *s, int li) {
    Listing *l = &s->used[li];
    if (l->price <= 0 || s->nown >= MAXOWN || s->credits < l->price) return 0;
    s->credits -= l->price;
    Owned *o = &s->own[s->nown];
    owned_new(o, l->model);
    o->color = l->color; o->cond = l->cond; o->km = l->km;
    for (int k = 0; k < l->nparts; k++) o->parts[l->parts[k]] = 1;
    s->cur = s->nown++;
    l->price = 0;                       /* vendido */
    return 1;
}

/* ====================================================================== */
/* Pista                                                                    */
/* ====================================================================== */
Trk TRK;

void trk_load(int id) {
    const TrackDef *d = &TRACK_DEFS[id];
    Trk *t = &TRK;
    t->def = d; t->n = d->npts; t->hw = d->hw; t->length_m = d->length_m;
    for (int i = 0; i < t->n; i++) {
        t->px[i] = (int32_t)d->pts[i * 2] * (POS_ONE / TRK_UNIT);
        t->pz[i] = (int32_t)d->pts[i * 2 + 1] * (POS_ONE / TRK_UNIT);
        t->vm[i] = (int)d->vmax[i] * 1092;
    }
    for (int i = 0; i < t->n; i++) {
        int a = (i + t->n - 1) % t->n, b = (i + 1) % t->n;
        int dx = (t->px[b] - t->px[a]) >> 2, dz = (t->pz[b] - t->pz[a]) >> 2;
        int l = isqrt32((uint32_t)(dx * dx + dz * dz)); if (l < 1) l = 1;
        t->tx[i] = (int16_t)(dx * 4096 / l); t->tz[i] = (int16_t)(dz * 4096 / l);
        t->heading[i] = iatan2(dx, dz);
    }
}

/* ====================================================================== */
/* Carrera                                                                  */
/* ====================================================================== */
int weather_grip(int w, int ty) {
    static const uint8_t tab[NWEATHER][NTYRES] = {
        {100, 100, 100,  88,  78},     /* soleado  */
        { 58,  60,  56,  92, 100},     /* lluvia   */
        { 50,  52,  48,  84,  96},     /* tormenta */
        {100, 100, 100,  88,  78},     /* niebla   */
    };
    return tab[w][ty];
}

void format_time(char *o, int ticks) {
    int cs = (int)((int64_t)ticks * 100 / 60);
    int m = cs / 6000, sec = (cs / 100) % 60, c = cs % 100;
    o[0] = (char)('0' + (m % 10)); o[1] = ':';
    o[2] = (char)('0' + sec / 10); o[3] = (char)('0' + sec % 10); o[4] = '.';
    o[5] = (char)('0' + c / 10);   o[6] = (char)('0' + c % 10); o[7] = 0;
}

static void car_setup(Car *c, const Stats *st, const Owned *o, int tyre) {
    for (unsigned i = 0; i < sizeof(*c); i++) ((uint8_t *)c)[i] = 0;
    c->vmax = st->vmax_kmh * 273 / 2;
    c->accel = st->accel * 1365 / 1000;
    c->grip = st->grip;
    c->nitro_idx = st->nitro;
    c->nitro_gauge = st->nitro >= 0 ? NITRO[st->nitro].cap * 100 : 0;
    c->tyre = tyre; c->pit_tyre = tyre;
    c->fuel = 600000; c->wear = 0;
    c->lap = -1; c->best_lap = 0;
    c->model = o->model; c->color = o->color; c->spoiler = o->spoiler;
    c->pp_vmax = c->vmax; c->vmax_eff = c->vmax;
}

static void place_on_grid(Car *c, int slot) {
    int row = slot >> 1, col = slot & 1, n = TRK.n;
    int i = ((n - 2 - row) % n + n) % n;
    int off = (col ? -3 : 3) * 256;                 /* derecha / izquierda */
    c->x = TRK.px[i] + (TRK.tz[i] * off >> 12);
    c->z = TRK.pz[i] - (TRK.tx[i] * off >> 12);
    c->heading = TRK.heading[i] << 8;
    c->idx = i; c->vx = c->vz = 0;
}

static void update_progress(Car *c, int full);

void race_init(Race *r, const RaceCfg *cfg, const Owned *p1, const Owned *p2) {
    for (unsigned i = 0; i < sizeof(*r); i++) ((uint8_t *)r)[i] = 0;
    trk_load(cfg->track);
    r->track = cfg->track; r->nlaps = cfg->laps; r->weather = cfg->weather; r->tod = cfg->tod;
    r->nitro_on = cfg->nitro; r->ncars = MAXCARS; r->nhumans = cfg->players;
    r->start_tyre = cfg->tyre;
    Stats st1, st2; car_stats(p1, &st1);
    if (p2) car_stats(p2, &st2);
    int hslot[2] = {4, 5};
    int used[MAXCARS] = {0};
    for (int h = 0; h < cfg->players; h++) {
        Car *c = &r->car[hslot[h]];
        car_setup(c, h == 0 ? &st1 : &st2, h == 0 ? p1 : p2, cfg->tyre);
        used[hslot[h]] = 1;
    }
    /* los humanos ocupan los primeros indices del array de coches */
    Car tmp[MAXCARS]; int map[MAXCARS], k = 0;
    for (int h = 0; h < cfg->players; h++) { tmp[k] = r->car[hslot[h]]; map[k] = hslot[h]; k++; }
    for (int s = 0; s < MAXCARS; s++) if (!used[s]) {
        Car *c = &tmp[k]; Owned ai;
        int model = rnd_range(NMODELS);
        for (unsigned q = 0; q < sizeof(ai); q++) ((uint8_t *)&ai)[q] = 0;
        ai.model = (uint8_t)model; ai.color = (uint8_t)rnd_range(13); ai.cond = 100;
        car_setup(c, &st1, &ai, TY_MEDIUM);       /* rivales igualados a tu coche */
        c->nitro_idx = -1; c->is_ai = 1; c->skill = 91 + rnd_range(8);
        c->lane = (rnd_range(7) - 3) * 256;
        map[k] = s; k++;
    }
    for (int i = 0; i < MAXCARS; i++) { r->car[i] = tmp[i]; place_on_grid(&r->car[i], map[i]); update_progress(&r->car[i], 1); r->car[i].idx = r->car[i].idx; }
    for (int i = 0; i < MAXCARS; i++) { r->car[i].lap = -1; }
    r->count = 60 * 4; r->state = 0; r->ticks = 0;
}

/* busca el punto mas cercano de la linea central */
static void update_progress(Car *c, int full) {
    const int n = TRK.n;
    int best = c->idx, bd = 0x7fffffff, from = full ? 0 : -3, to = full ? n - 1 : 10;
    for (int d = from; d <= to; d++) {
        int i = full ? d : ((c->idx + d) % n + n) % n;
        int dx = (c->x - TRK.px[i]) >> 4, dz = (c->z - TRK.pz[i]) >> 4;
        int d2 = dx * dx + dz * dz;
        if (d2 < bd) { bd = d2; best = i; }
    }
    int old = c->idx; c->idx = best;
    if (!full) {
        if (old > n * 3 / 4 && best < n / 4) c->lap++, c->prog = 0x40000000; /* marca evento */
        else if (old < n / 4 && best > n * 3 / 4) c->lap--;
    }
}

static int fine_progress(const Car *c) {
    int i = c->idx;
    int dx = c->x - TRK.px[i], dz = c->z - TRK.pz[i];
    int dot = (dx * TRK.tx[i] + dz * TRK.tz[i]) >> 12;
    return (c->lap * TRK.n + i) * 6000 + iclamp(dot, -2999, 2999) + 3000;
}

static void ai_drive(Race *r, Car *c, PadIn *o, int humanprog) {
    const int n = TRK.n;
    int h = (c->heading >> 8) & 4095, s = isin(h), co = icos(h);
    int fwd = (c->vx * s + c->vz * co) >> 12;
    int ms = (iabs(fwd) * 60) >> 16;
    int look = 2 + ms / 9;
    int ti = (c->idx + look) % n;
    int dx = TRK.px[ti] + (-TRK.tz[ti] * c->lane >> 12) - c->x;
    int dz = TRK.pz[ti] + ( TRK.tx[ti] * c->lane >> 12) - c->z;
    dx >>= 4; dz >>= 4;
    int dist = isqrt32((uint32_t)(dx * dx + dz * dz)); if (dist < 1) dist = 1;
    int dotr = (co * dx - s * dz) >> 12, dotf = (s * dx + co * dz) >> 12;
    int steer = dotr * 900 / dist;
    if (dotf < 0) steer = dotr >= 0 ? 256 : -256;
    o->steer = iclamp(steer, -256, 256);

    /* ajuste de dificultad: si voy muy por delante del jugador freno un poco */
    int adj = 0;
    if (humanprog > -100000) adj = iclamp((humanprog - (c->lap * n + c->idx)) * 3, -7, 7);
    int sk = c->skill + adj;
    c->vmax_eff = c->pp_vmax * sk / 100;
    int tv = c->vmax_eff;
    for (int k = 0; k <= 7; k++) {
        int v = TRK.vm[(c->idx + k) % n] * sk / 100;
        if (v < tv) tv = v;
    }
    o->thr = o->brk = 0; o->nitro = 0; o->pit = 0;
    if (fwd < tv - tv / 20) o->thr = 256;
    else if (fwd > tv + tv / 14) o->brk = iclamp((fwd - tv) * 256 / (tv / 4 + 1), 40, 256);
    else o->thr = 80;
    if (r->state == 0) { o->thr = 0; o->brk = 0; }
}

static void car_physics(Race *r, Car *c, const PadIn *in) {
    int thr = in->thr, brk = in->brk, st = in->steer;
    c->vx = iclamp(c->vx, -100000, 100000); c->vz = iclamp(c->vz, -100000, 100000);
    if (r->state == 0) { thr = 0; brk = 0; st = 0; }
    int h = (c->heading >> 8) & 4095, s = isin14(h), co = icos14(h);
    int fwd = (c->vx * s + c->vz * co) >> 14;
    int lat = (c->vx * co - c->vz * s) >> 14;

    /* ---- limites segun superficie, nitro, combustible, neumaticos ---- */
    int vmaxE = c->is_ai ? c->vmax_eff : c->vmax, accE = c->accel;
    int boost = 0;
    c->nitro_on = 0;
    if (!c->is_ai && r->nitro_on && c->nitro_idx >= 0 && in->nitro && c->nitro_gauge > 0 && thr > 0) {
        const NitroDef *nd = &NITRO[c->nitro_idx];
        vmaxE += vmaxE * nd->bcap / 200;
        accE += accE * nd->baccel / 40;
        c->nitro_gauge -= nd->cons; if (c->nitro_gauge < 0) c->nitro_gauge = 0;
        c->nitro_on = 1; boost = 1;
    } else if (!c->is_ai && c->nitro_idx >= 0) {
        int cap = NITRO[c->nitro_idx].cap * 100;
        c->nitro_gauge = imin(cap, c->nitro_gauge + NITRO[c->nitro_idx].regen);
    }
    int gripE = c->grip * TYRES[c->tyre].grip / 100 * weather_grip(r->weather, c->tyre) / 100;
    gripE = gripE * (100 - (c->wear / 1000) * 35 / 100) / 100;
    if (c->is_ai) gripE = c->grip * weather_grip(r->weather, TY_MEDIUM) / 100;
    gripE = imax(gripE, 20);
    if (!c->is_ai && c->fuel <= 0) vmaxE = vmaxE * 40 / 100;
    if (c->offroad) { vmaxE = vmaxE * 55 / 100; accE /= 2; }

    /* ---- boxes ---- */
    if (c->pit_state == 1) { thr = 0; brk = 256; st = 0; }
    else if (c->pit_state == 2) { thr = 0; brk = 256; st = 0; }

    /* ---- longitudinal ---- */
    if (thr > 0) {
        int r256 = (vmaxE - fwd) * 256 / vmaxE;
        if (r256 < -48) r256 = -48;
        fwd += accE * thr / 256 * r256 / 256;
    } else {
        fwd -= fwd >> 7;                       /* freno motor / rodadura */
    }
    if (brk > 0) {
        int bv = 330 * imin(gripE, 140) / 100;
        if (fwd > 0) { fwd -= bv * brk / 256; if (fwd < 0) fwd = 0; }
        else { fwd -= accE * brk / 512; if (fwd < -vmaxE / 5) fwd = -vmaxE / 5; }
    }
    if (c->offroad) fwd -= fwd >> 5;

    /* ---- direccion (radio de giro crece con v^2: subviraje) ---- */
    int target = iclamp(st, -256, 256);
    int step = target == 0 ? 44 : 30;
    if (c->steer < target) c->steer = imin(target, c->steer + step);
    else if (c->steer > target) c->steer = imax(target, c->steer - step);
    int v16 = (iabs(fwd) * 15) >> 10;
    int g16 = 384 * gripE / 100;
    int R16 = 96 + v16 * v16 / g16;
    int yaw = c->steer * v16 * 11 / R16;
    if (fwd < 0) yaw = -yaw;

    /* reconstruyo velocidad con el rumbo antiguo y roto */
    c->vx = (fwd * s + lat * co) >> 14;
    c->vz = (fwd * co - lat * s) >> 14;
    c->heading = (c->heading + yaw) & ((4096 << 8) - 1);
    h = (c->heading >> 8) & 4095; s = isin14(h); co = icos14(h);
    fwd = (c->vx * s + c->vz * co) >> 14;
    lat = (c->vx * co - c->vz * s) >> 14;

    /* agarre lateral */
    int gk = 22 * gripE / 100;
    int dl = lat * gk / 256;
    lat -= dl;
    if (fwd > 0) fwd -= iabs(dl) / 8;
    c->drift = iabs(lat) > 2200;
    c->vx = (fwd * s + lat * co) >> 14;
    c->vz = (fwd * co - lat * s) >> 14;
    c->x += c->vx >> 8; c->z += c->vz >> 8;

    /* ---- consumo ---- */
    if (!c->is_ai && r->state == 1) {
        c->fuel -= 12 + 18 * thr / 256 + boost * 6; if (c->fuel < 0) c->fuel = 0;
        int sp = iclamp(iabs(fwd) * 256 / (c->vmax ? c->vmax : 1), 0, 256);
        int corner = iabs(c->steer) * sp / 256;
        c->wear += TYRES[c->tyre].wear * (24 + (sp >> 4) + (corner >> 3)) / 500;
        if (c->wear > 100000) c->wear = 100000;
    }
    c->kmh = iabs(fwd) * 15 / 2048;
    c->braking = brk > 0 && fwd > 0;
    {   /* marchas y rpm */
        static const uint8_t gb[7] = {0, 12, 27, 43, 60, 78, 100};
        int ratio = iclamp(iabs(fwd) * 100 / (c->vmax ? c->vmax : 1), 0, 100);
        int g = 0; while (g < 5 && ratio >= gb[g + 1]) g++;
        c->gear = fwd < -200 ? -1 : g + 1;
        c->rpm = iclamp(25 + (ratio - gb[g]) * 75 / (gb[g + 1] - gb[g]), 0, 100);
        if (thr == 0) c->rpm = imax(20, c->rpm - 15);
    }
}

static void track_constraints(Car *c) {
    const Trk *t = &TRK; int i = c->idx;
    int dx = c->x - t->px[i], dz = c->z - t->pz[i];
    int nx = -t->tz[i], nz = t->tx[i];                  /* normal izquierda */
    int off = (dx * nx + dz * nz) >> 12;
    c->lat_off = off;
    c->offroad = iabs(off) > t->hw * 256 + 384;
    int wall = (t->hw + 16) * 256;
    if (iabs(off) > wall) {
        int sgn = off > 0 ? 1 : -1, ex = iabs(off) - wall;
        c->x -= (nx * ex >> 12) * sgn; c->z -= (nz * ex >> 12) * sgn;
        int vn = (c->vx * nx + c->vz * nz) >> 12;
        if (vn * sgn > 0) { c->vx -= (nx * vn >> 12); c->vz -= (nz * vn >> 12); }
        c->vx = c->vx * 7 / 8; c->vz = c->vz * 7 / 8;
    }
}

static void do_collisions(Race *r) {
    for (int i = 0; i < r->ncars; i++) for (int j = i + 1; j < r->ncars; j++) {
        Car *a = &r->car[i], *b = &r->car[j];
        int dx = (b->x - a->x) >> 2, dz = (b->z - a->z) >> 2;
        if (iabs(dx) > 400 || iabs(dz) > 400) continue;
        int d2 = dx * dx + dz * dz;
        const int R = 270;                              /* 4.2 m en unidades>>2 */
        if (d2 >= R * R) continue;
        int len = isqrt32((uint32_t)d2); if (len < 1) { len = 1; dx = 1; }
        int ov = R - len, nx = dx * 4096 / len, nz = dz * 4096 / len;
        int push = ov * 2;                               /* mitad del solape en 1/256 m */
        a->x -= nx * push >> 12; a->z -= nz * push >> 12;
        b->x += nx * push >> 12; b->z += nz * push >> 12;
        int rv = ((b->vx - a->vx) * nx + (b->vz - a->vz) * nz) >> 12;
        if (rv < 0) {
            int j2 = rv * 6 / 10;
            a->vx += nx * j2 >> 12; a->vz += nz * j2 >> 12;
            b->vx -= nx * j2 >> 12; b->vz -= nz * j2 >> 12;
            a->vx = a->vx * 31 / 32; a->vz = a->vz * 31 / 32;
            b->vx = b->vx * 31 / 32; b->vz = b->vz * 31 / 32;
        }
    }
}

static void order_cars(Race *r) {
    int key[MAXCARS], ord[MAXCARS];
    for (int i = 0; i < r->ncars; i++) {
        const Car *c = &r->car[i];
        key[i] = c->finished ? 0x7ffffff0 - c->finish_time : fine_progress(c);
        ord[i] = i;
    }
    for (int i = 0; i < r->ncars; i++) for (int j = i + 1; j < r->ncars; j++)
        if (key[ord[j]] > key[ord[i]]) { int t = ord[i]; ord[i] = ord[j]; ord[j] = t; }
    for (int i = 0; i < r->ncars; i++) r->car[ord[i]].rank = i;
}

void race_tick(Race *r, const PadIn in[2]) {
    if (r->count > 0) { r->count--; if (r->state == 0 && r->count <= 60) { r->state = 1; r->ticks = 0; } }
    if (r->state >= 1) r->ticks++;
    int hprog = -1000000;
    for (int i = 0; i < r->nhumans; i++) if (!r->car[i].finished) {
        int p = r->car[i].lap * TRK.n + r->car[i].idx;
        if (hprog == -1000000 || p > hprog) hprog = p;
    }
    for (int i = 0; i < r->ncars; i++) {
        Car *c = &r->car[i]; PadIn p = {0, 0, 0, 0, 0, 0};
        if (c->is_ai || c->auto_drive) {
            if (!c->skill) c->skill = 88;
            ai_drive(r, c, &p, c->is_ai ? hprog : -1000000);
        } else p = in[i];
        if (c->finished && !c->is_ai) c->auto_drive = 1;

        /* boxes: peticion y cambio de neumaticos */
        if (!c->is_ai && r->state == 1 && !c->finished) {
            if (p.pit) c->pit_req = !c->pit_req;
            if (p.tyre) c->pit_tyre = (c->pit_tyre + 1) % NTYRES;
            if (c->pit_state == 1 && c->kmh <= 2) {
                c->pit_state = 2;
                int litres = (600000 - c->fuel) / 10000;
                c->pit_timer = 60 * (3 + litres / 10) + (c->pit_tyre != c->tyre || c->wear > 8000 ? 60 * 3 : 0);
            } else if (c->pit_state == 2 && --c->pit_timer <= 0) {
                c->pit_state = 0; c->pit_req = 0; c->fuel = 600000; c->wear = 0;
                c->tyre = c->pit_tyre; c->pits++;
            }
        }
        int oldprog = c->prog;
        (void)oldprog;
        c->prog = 0;
        car_physics(r, c, &p);
        c->in_thr = p.thr; c->in_brk = p.brk; c->in_steer = p.steer;
        update_progress(c, 0);
        int crossed = (c->prog == 0x40000000);
        c->prog = c->lap * TRK.n + c->idx;
        track_constraints(c);
        if (crossed && r->state >= 1) {
            if (c->lap >= 1) {
                int lt = r->ticks - c->lap_start;
                c->last_lap = lt; if (!c->best_lap || lt < c->best_lap) c->best_lap = lt;
            }
            c->lap_start = r->ticks;
            if (c->lap >= r->nlaps && !c->finished) { c->finished = 1; c->finish_time = r->ticks; }
            if (!c->is_ai && c->pit_req && !c->finished && c->lap < r->nlaps) c->pit_state = 1;
        }
    }
    do_collisions(r);
    order_cars(r);
    if (r->state == 1) {
        int done = 1;
        for (int i = 0; i < r->nhumans; i++) if (!r->car[i].finished) done = 0;
        if (done) r->state = 2;
    }
}

int race_reward_for(const Race *r, int rank) {
    static const uint8_t mult[MAXCARS] = {100, 60, 40, 25, 15, 10};
    int base = TRACK_DEFS[r->track].reward * r->nlaps / TRACK_DEFS[r->track].laps;
    return base * mult[iclamp(rank, 0, MAXCARS - 1)] / 100;
}

void race_finish_save(Race *r, Save *s) {
    if (r->reward_done) return;
    r->reward_done = 1;
    r->reward = r->car[0].finished ? race_reward_for(r, r->car[0].rank) : 0;
    s->credits += r->reward;
    Owned *o = &s->own[s->cur];
    int km = TRK.length_m * r->nlaps / 1000;
    o->km = (uint16_t)imin(65000, o->km + km);
    if (o->cond > 40 && (rnd() & 1)) o->cond--;
    s->mseed = s->mseed * 1664525u + 1013904223u + (uint32_t)r->ticks;
    market_refresh(s);
    s->races++;
}
