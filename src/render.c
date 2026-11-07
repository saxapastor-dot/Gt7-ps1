/* GT7 PS1 - render 3D por software.  Unidades de dibujo: 1/16 m.
 * Todo se convierte en triangulos planos (gfx_tri) ordenados por profundidad. */
#include "render.h"

typedef struct { int x, y, z; } V3;
typedef struct { int x, y; } P2;

#define NEAR 20

static struct {
    Rect vp; int cx, cy, focal;
    int camx, camy, camz, sn, cs;
    int light, fr, fg, fb, fog0, fog1;
    int night, weather, tod, theme;
    int sky_top[3], sky_hor[3];
} V;

static int cl(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int ab(int v) { return v < 0 ? -v : v; }
static int mix(int a, int b, int t256) { return a + (((b - a) * t256) >> 8); }

/* ---------------------------------------------------------------- tema */
typedef struct { uint8_t grass_a[3], grass_b[3], hill[3], road[3]; } Theme;
static const Theme THEMES[4] = {
    {{ 62,152, 62}, { 54,138, 54}, { 70, 95,125}, { 82, 82, 86}},   /* clasico  */
    {{ 30,112, 46}, { 25, 98, 40}, { 28, 66, 62}, { 76, 74, 72}},   /* bosque   */
    {{ 58, 46, 42}, { 48, 38, 35}, { 96, 36, 22}, { 62, 60, 62}},   /* volcan   */
    {{198,112, 62}, {184, 98, 52}, {172, 84, 52}, {112, 94, 88}},   /* canon    */
};

static void env_setup(int theme, int weather, int tod) {
    static const int top[3][3] = {{70,130,225},{70,60,130},{6,9,28}};
    static const int hor[3][3] = {{175,215,245},{255,160,90},{32,42,78}};
    static const int lig[3] = {256, 196, 86};
    int i, grey, gk, wl = 256;
    for (i = 0; i < 3; i++) { V.sky_top[i] = top[tod][i]; V.sky_hor[i] = hor[tod][i]; }
    if (theme == 2) for (i = 0; i < 3; i++) V.sky_hor[i] = mix(V.sky_hor[i], i == 0 ? 220 : (i == 1 ? 80 : 40), 90);
    if (theme == 3) for (i = 0; i < 3; i++) V.sky_hor[i] = mix(V.sky_hor[i], i == 0 ? 255 : (i == 1 ? 200 : 150), 50);
    if (weather != WE_SUNNY) {
        gk = weather == WE_RAIN ? 150 : (weather == WE_STORM ? 210 : 200);
        grey = weather == WE_STORM ? 70 : (weather == WE_FOG ? 190 : 118);
        if (tod == TOD_NIGHT) grey = grey / 4; else if (tod == TOD_DUSK) grey = grey * 3 / 4;
        for (i = 0; i < 3; i++) { V.sky_top[i] = mix(V.sky_top[i], grey, gk); V.sky_hor[i] = mix(V.sky_hor[i], grey + 12, gk); }
        wl = weather == WE_STORM ? 190 : (weather == WE_RAIN ? 222 : 240);
    }
    V.light = lig[tod] * wl >> 8;
    V.fr = V.sky_hor[0]; V.fg = V.sky_hor[1]; V.fb = V.sky_hor[2];
    switch (weather) {
        case WE_RAIN:  V.fog0 = 70 * 16;  V.fog1 = 250 * 16; break;
        case WE_STORM: V.fog0 = 40 * 16;  V.fog1 = 170 * 16; break;
        case WE_FOG:   V.fog0 = 10 * 16;  V.fog1 = 80 * 16;  break;
        default:       V.fog0 = 100 * 16; V.fog1 = 340 * 16; break;
    }
    if (tod == TOD_NIGHT) { V.fog0 = V.fog0 * 6 / 10; V.fog1 = V.fog1 * 7 / 10; }
    V.night = tod == TOD_NIGHT; V.weather = weather; V.tod = tod; V.theme = theme;
}

static void shade(int r, int g, int b, int Z, int light, int *o) {
    int t = Z <= V.fog0 ? 0 : (Z >= V.fog1 ? 256 : (Z - V.fog0) * 256 / (V.fog1 - V.fog0));
    r = r * light >> 8; g = g * light >> 8; b = b * light >> 8;
    o[0] = mix(r, V.fr, t); o[1] = mix(g, V.fg, t); o[2] = mix(b, V.fb, t);
}

/* ------------------------------------------------------------ poligonos */
static int zbucket(int Z, int bias) { int b = 3 + (Z >> 3) + bias; return cl(b, 2, OT_LEN - 8); }

static int clip_near(const V3 *in, int n, V3 *out) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        const V3 *a = &in[i], *b = &in[(i + 1) % n];
        int ia = a->z >= NEAR, ib = b->z >= NEAR;
        if (ia) out[m++] = *a;
        if (ia != ib) {
            int den = b->z - a->z, t = (NEAR - a->z) * 4096 / den;
            out[m].x = a->x + ((b->x - a->x) * t >> 12);
            out[m].y = a->y + ((b->y - a->y) * t >> 12);
            out[m].z = NEAR; m++;
        }
    }
    return m;
}

static int clip2d(const P2 *in, int n, P2 *out, int axis, int bound, int keep_ge) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        P2 a = in[i], b = in[(i + 1) % n];
        int ca = axis ? a.y : a.x, cb = axis ? b.y : b.x;
        int ia = keep_ge ? ca >= bound : ca <= bound, ib = keep_ge ? cb >= bound : cb <= bound;
        if (ib) {
            if (!ia) { P2 p; int64_t t = (int64_t)(bound - ca);
                int64_t d = cb - ca;
                if (axis) { p.y = bound; p.x = a.x + (int)((int64_t)(b.x - a.x) * t / d); }
                else      { p.x = bound; p.y = a.y + (int)((int64_t)(b.y - a.y) * t / d); }
                out[m++] = p; }
            out[m++] = b;
        } else if (ia) {
            P2 p; int64_t t = (int64_t)(bound - ca), d = cb - ca;
            if (axis) { p.y = bound; p.x = a.x + (int)((int64_t)(b.x - a.x) * t / d); }
            else      { p.x = bound; p.y = a.y + (int)((int64_t)(b.y - a.y) * t / d); }
            out[m++] = p;
        }
    }
    return m;
}

static void poly(const V3 *v, int n, int r, int g, int b, int bias, int light) {
    V3 c[10]; P2 p[16], q[16];
    int m = clip_near(v, n, c);
    if (m < 3) return;
    int zs = 0, i;
    for (i = 0; i < m; i++) {
        zs += c[i].z;
        p[i].x = V.cx + c[i].x * V.focal / c[i].z;
        p[i].y = V.cy - c[i].y * V.focal / c[i].z;
    }
    zs /= m;
    int minx = p[0].x, maxx = p[0].x, miny = p[0].y, maxy = p[0].y;
    for (i = 1; i < m; i++) {
        if (p[i].x < minx) minx = p[i].x; if (p[i].x > maxx) maxx = p[i].x;
        if (p[i].y < miny) miny = p[i].y; if (p[i].y > maxy) maxy = p[i].y;
    }
    if (maxx < V.vp.x0 || minx >= V.vp.x1 || maxy < V.vp.y0 || miny >= V.vp.y1) return;
    if (maxx - minx < 1 && maxy - miny < 1) return;
    if (minx < V.vp.x0 || maxx >= V.vp.x1 || miny < V.vp.y0 || maxy >= V.vp.y1) {
        m = clip2d(p, m, q, 0, V.vp.x0, 1); if (m < 3) return;
        m = clip2d(q, m, p, 0, V.vp.x1 - 1, 0); if (m < 3) return;
        m = clip2d(p, m, q, 1, V.vp.y0, 1); if (m < 3) return;
        m = clip2d(q, m, p, 1, V.vp.y1 - 1, 0); if (m < 3) return;
    }
    int col[3]; shade(r, g, b, zs, light, col);
    int zb = zbucket(zs, bias);
    for (i = 1; i + 1 < m; i++)
        gfx_tri(p[0].x, p[0].y, p[i].x, p[i].y, p[i + 1].x, p[i + 1].y, col[0], col[1], col[2], zb);
}

static void to_cam(int wx, int wy, int wz, V3 *o) {
    int dx = wx - V.camx, dz = wz - V.camz;
    o->x = (dx * V.cs - dz * V.sn) >> 12;
    o->z = (dx * V.sn + dz * V.cs) >> 12;
    o->y = wy - V.camy;
}

static void quadw(const V3 *a, const V3 *b, const V3 *c, const V3 *d, const uint8_t *col, int bias, int light) {
    V3 t[4] = {*a, *b, *c, *d};
    poly(t, 4, col[0], col[1], col[2], bias, light);
}

/* ---------------------------------------------------------- caja generica */
static void draw_box(int px, int pz, int heading, int cx, int y0, int cz, int hx, int h, int hz,
                     int r, int g, int b, int bias, int light) {
    int s = isin(heading), c = icos(heading);
    int bx = px + ((cx * c + cz * s) >> 12), bz = pz + ((-cx * s + cz * c) >> 12);
    int cdx = V.camx - bx, cdz = V.camz - bz;
    int lx = (cdx * c - cdz * s) >> 12, lz = (cdx * s + cdz * c) >> 12;
    V3 cv[8];
    for (int i = 0; i < 8; i++) {
        int ox = (i & 1) ? hx : -hx, oy = (i & 2) ? h : 0, oz = (i & 4) ? hz : -hz;
        int wx = px + (((cx + ox) * c + (cz + oz) * s) >> 12);
        int wz = pz + ((-(cx + ox) * s + (cz + oz) * c) >> 12);
        to_cam(wx, y0 + oy, wz, &cv[i]);
    }
    int camy_rel = V.camy - y0;
    V3 f[4];
    if (camy_rel > h) { f[0] = cv[2]; f[1] = cv[3]; f[2] = cv[7]; f[3] = cv[6]; poly(f, 4, r, g, b, bias, light); }
    if (lz > hz)  { f[0] = cv[4]; f[1] = cv[5]; f[2] = cv[7]; f[3] = cv[6]; poly(f, 4, r * 215 >> 8, g * 215 >> 8, b * 215 >> 8, bias, light); }
    if (lz < -hz) { f[0] = cv[0]; f[1] = cv[1]; f[2] = cv[3]; f[3] = cv[2]; poly(f, 4, r * 190 >> 8, g * 190 >> 8, b * 190 >> 8, bias, light); }
    if (lx > hx)  { f[0] = cv[1]; f[1] = cv[5]; f[2] = cv[7]; f[3] = cv[3]; poly(f, 4, r * 170 >> 8, g * 170 >> 8, b * 170 >> 8, bias, light); }
    if (lx < -hx) { f[0] = cv[0]; f[1] = cv[4]; f[2] = cv[6]; f[3] = cv[2]; poly(f, 4, r * 150 >> 8, g * 150 >> 8, b * 150 >> 8, bias, light); }
}

/* ------------------------------------------------------------ coches */
/* Cada pieza es un tronco de piramide (la parte superior puede ser mas pequena -> lunas y morro inclinados) */
typedef struct { int8_t y0, h, cx, cz, hx, hz, tcx, tcz, thx, thz; uint8_t col, side, opt; } Part3D;
enum { C_BODY, C_DARK, C_GLASS, C_TAIL, C_HEAD, C_WHEEL, C_WHITE, C_WING, C_RIM };
/* side: 0 = mismo color en todas las caras, 1 = lados cristal y techo carroceria (cabina) */
#define BX(cx,y0,cz,hx,h,hz,col,opt) {y0,h,cx,cz,hx,hz,cx,cz,hx,hz,col,0,opt}
#define FR(y0,h,cx,cz,hx,hz,tcx,tcz,thx,thz,col,side,opt) {y0,h,cx,cz,hx,hz,tcx,tcz,thx,thz,col,side,opt}
#define WHEEL4(R,X,Z,TW) \
    BX(-X,0,-Z,TW,R,5,C_WHEEL,0), BX(X,0,-Z,TW,R,5,C_WHEEL,0), BX(-X,0,Z,TW,R,5,C_WHEEL,0), BX(X,0,Z,TW,R,5,C_WHEEL,0), \
    BX(-(X+TW),R/4,-Z,1,R/2,3,C_RIM,0), BX(X+TW,R/4,-Z,1,R/2,3,C_RIM,0), BX(-(X+TW),R/4,Z,1,R/2,3,C_RIM,0), BX(X+TW,R/4,Z,1,R/2,3,C_RIM,0)

static const Part3D M_GT[] = {
    WHEEL4(10, 14, 23, 3),
    FR(4,7, 0,0,15,36, 0,-4,14,29, C_BODY,0,0),
    FR(11,6, 0,-3,12,18, 0,-5,10,10, C_GLASS,1,0),
    BX(-11,7,-37,3,2,1,C_TAIL,0), BX(11,7,-37,3,2,1,C_TAIL,0), BX(-11,7,37,3,2,1,C_HEAD,0), BX(11,7,37,3,2,1,C_HEAD,0),
    BX(0,15,-35,14,2,4,C_WING,1), BX(-8,11,-34,1,4,1,C_DARK,1), BX(8,11,-34,1,4,1,C_DARK,1),
};
static const Part3D M_SUPER[] = {
    WHEEL4(10, 14, 24, 3),
    FR(3,6, 0,0,16,38, 0,-6,14,28, C_BODY,0,0),
    FR(9,5, 0,-4,11,17, 0,-6,9,9, C_GLASS,1,0),
    BX(-12,6,-39,3,2,1,C_TAIL,0), BX(12,6,-39,3,2,1,C_TAIL,0), BX(-12,6,39,3,2,1,C_HEAD,0), BX(12,6,39,3,2,1,C_HEAD,0),
    BX(0,13,-37,15,2,4,C_WING,1), BX(-9,9,-36,1,4,1,C_DARK,1), BX(9,9,-36,1,4,1,C_DARK,1),
};
static const Part3D M_TOON[] = {
    WHEEL4(11, 14, 22, 3),
    FR(4,8, 0,0,15,34, 0,-3,14,29, C_BODY,0,0),
    FR(12,9, 0,2,12,14, 0,0,10,10, C_GLASS,1,0),
    BX(-6,15,13,3,4,1,C_WHITE,0), BX(6,15,13,3,4,1,C_WHITE,0),
    BX(-8,9,35,4,4,1,C_WHITE,0), BX(8,9,35,4,4,1,C_WHITE,0),
    BX(-11,8,-35,3,2,1,C_TAIL,0), BX(11,8,-35,3,2,1,C_TAIL,0), BX(0,13,-33,14,2,3,C_WING,1),
};
static const Part3D M_F1[] = {
    BX(-17,0,-25,4,14,7,C_WHEEL,0), BX(17,0,-25,4,14,7,C_WHEEL,0), BX(-17,0,26,3,12,6,C_WHEEL,0), BX(17,0,26,3,12,6,C_WHEEL,0),
    BX(-21,4,-25,1,6,3,C_RIM,0), BX(21,4,-25,1,6,3,C_RIM,0), BX(-20,4,26,1,5,3,C_RIM,0), BX(20,4,26,1,5,3,C_RIM,0),
    FR(3,6, 0,-3,5,32, 0,-5,3,27, C_BODY,0,0),
    FR(3,4, 0,32,4,7, 0,35,2,4, C_BODY,0,0),
    BX(-9,3,-6,3,5,14,C_BODY,0), BX(9,3,-6,3,5,14,C_BODY,0),
    BX(0,9,-8,4,3,8,C_DARK,0), BX(0,12,-8,3,3,3,C_WHITE,0),
    BX(0,2,38,17,2,5,C_WING,0), BX(0,19,-36,12,2,4,C_WING,0), BX(0,12,-34,1,7,1,C_DARK,0), BX(0,4,-38,3,2,1,C_TAIL,0),
};
static const Part3D M_KART[] = {
    BX(-11,0,-11,3,7,4,C_WHEEL,0), BX(11,0,-11,3,7,4,C_WHEEL,0), BX(-10,0,11,2,6,3,C_WHEEL,0), BX(10,0,11,2,6,3,C_WHEEL,0),
    FR(3,4, 0,0,9,15, 0,0,7,11, C_BODY,0,0), BX(0,7,-2,6,3,6,C_DARK,0), BX(0,10,-3,3,4,3,C_WHITE,0),
    BX(0,3,17,10,3,3,C_BODY,0), BX(0,3,-17,9,3,3,C_BODY,0),
};

static void car_colour(int idx, const uint8_t *body, int braking, int *o) {
    switch (idx) {
        case C_BODY: o[0] = body[0]; o[1] = body[1]; o[2] = body[2]; break;
        case C_DARK: o[0] = 36; o[1] = 36; o[2] = 40; break;
        case C_GLASS: o[0] = 44; o[1] = 70; o[2] = 110; break;
        case C_TAIL: o[0] = braking ? 255 : 150; o[1] = braking ? 40 : 14; o[2] = braking ? 40 : 14; break;
        case C_HEAD: o[0] = 255; o[1] = 250; o[2] = 200; break;
        case C_WHEEL: o[0] = 24; o[1] = 24; o[2] = 27; break;
        case C_WHITE: o[0] = 235; o[1] = 235; o[2] = 240; break;
        case C_RIM: o[0] = 176; o[1] = 178; o[2] = 188; break;
        default: o[0] = 22; o[1] = 22; o[2] = 26; break;
    }
}

static void draw_part(int px, int pz, int heading, const Part3D *p, const int *col, const uint8_t *body, int light) {
    int s = isin(heading), c = icos(heading), i;
    V3 v[8];
    for (i = 0; i < 8; i++) {
        int top = i >> 2, k = i & 3;
        int cx = top ? p->tcx : p->cx, cz = top ? p->tcz : p->cz, hx = top ? p->thx : p->hx, hz = top ? p->thz : p->hz;
        int lx = cx + ((k == 0 || k == 3) ? -hx : hx), lz = cz + ((k < 2) ? -hz : hz);
        to_cam(px + ((lx * c + lz * s) >> 12), p->y0 + (top ? p->h : 0), pz + ((-lx * s + lz * c) >> 12), &v[i]);
    }
    static const uint8_t fi[5][4] = {{0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}};
    static const int fs[5] = {190, 170, 215, 150, 256};
    V3 cp = {0, 0, 0};
    for (i = 0; i < 8; i++) { cp.x += v[i].x; cp.y += v[i].y; cp.z += v[i].z; }
    cp.x >>= 3; cp.y >>= 3; cp.z >>= 3;
    for (int f = 0; f < 5; f++) {
        V3 q[4]; for (i = 0; i < 4; i++) q[i] = v[fi[f][i]];
        V3 cf = {(q[0].x + q[1].x + q[2].x + q[3].x) >> 2, (q[0].y + q[1].y + q[2].y + q[3].y) >> 2, (q[0].z + q[1].z + q[2].z + q[3].z) >> 2};
        int ax = q[1].x - q[0].x, ay = q[1].y - q[0].y, az = q[1].z - q[0].z;
        int bx = q[2].x - q[0].x, by = q[2].y - q[0].y, bz = q[2].z - q[0].z;
        int nx = (ay * bz - az * by) >> 4, ny = (az * bx - ax * bz) >> 4, nz = (ax * by - ay * bx) >> 4;
        if (nx * (cf.x - cp.x) + ny * (cf.y - cp.y) + nz * (cf.z - cp.z) < 0) { nx = -nx; ny = -ny; nz = -nz; }
        if (((nx >> 2) * (cf.x >> 2) + (ny >> 2) * (cf.y >> 2) + (nz >> 2) * (cf.z >> 2)) >= 0) continue;   /* cara trasera */
        int r = col[0], g = col[1], b = col[2];
        if (p->side == 1 && f == 4) { r = body[0]; g = body[1]; b = body[2]; }
        poly(q, 4, r * fs[f] >> 8, g * fs[f] >> 8, b * fs[f] >> 8, 0, light);
    }
}

static void draw_car_model(int px, int pz, int heading, int model, const uint8_t *body, int spoiler, int braking, int hide) {
    if (hide) return;
    const Part3D *bx; int n;
    switch (MODELS[model].shape) {
        case SH_SUPER: bx = M_SUPER; n = (int)(sizeof(M_SUPER) / sizeof(Part3D)); break;
        case SH_TOON:  bx = M_TOON;  n = (int)(sizeof(M_TOON) / sizeof(Part3D)); break;
        case SH_F1:    bx = M_F1;    n = (int)(sizeof(M_F1) / sizeof(Part3D)); break;
        case SH_KART:  bx = M_KART;  n = (int)(sizeof(M_KART) / sizeof(Part3D)); break;
        default:       bx = M_GT;    n = (int)(sizeof(M_GT) / sizeof(Part3D)); break;
    }
    {   /* sombra */
        int s = isin(heading), c = icos(heading), k; V3 sh[4]; uint8_t col[3] = {34, 34, 38};
        int hx = MODELS[model].shape == SH_KART ? 13 : 17, hz = MODELS[model].shape == SH_KART ? 19 : 38;
        static const int sx[4] = {-1, 1, 1, -1}, sz[4] = {-1, -1, 1, 1};
        for (k = 0; k < 4; k++) {
            int lx = sx[k] * hx, lz = sz[k] * hz;
            to_cam(px + ((lx * c + lz * s) >> 12), 1, pz + ((-lx * s + lz * c) >> 12), &sh[k]);
        }
        quadw(&sh[0], &sh[1], &sh[2], &sh[3], col, 8, V.light);
    }
    for (int i = 0; i < n; i++) {
        const Part3D *b = &bx[i]; int col[3];
        if (b->opt && !spoiler) continue;
        car_colour(b->col, body, braking, col);
        int light = V.light;
        if (b->col == C_HEAD || (b->col == C_TAIL && braking)) light = 256;
        draw_part(px, pz, heading, b, col, body, light);
    }
}

/* ------------------------------------------------------- fondo y cielo */
static void draw_sky(void) {
    int y0 = V.vp.y0, hy = cl(V.cy, y0, V.vp.y1), w = V.vp.x1 - V.vp.x0, nb = 10, i;
    for (i = 0; i < nb; i++) {
        int ya = y0 + (hy - y0) * i / nb, yb = y0 + (hy - y0) * (i + 1) / nb;
        int t = i * 256 / (nb - 1);
        int r = mix(V.sky_top[0], V.sky_hor[0], t), g = mix(V.sky_top[1], V.sky_hor[1], t), b = mix(V.sky_top[2], V.sky_hor[2], t);
        gfx_rect(V.vp.x0, ya, w, yb - ya + 1, r, g, b, OT_LEN - 2);
    }
}

static void draw_ground_bg(const Theme *th) {
    int w = V.vp.x1 - V.vp.x0, y = V.cy, h = V.vp.y1 - y; int col[3];
    if (h <= 0) return;
    shade(th->grass_a[0], th->grass_a[1], th->grass_a[2], V.fog1, V.light, col);
    gfx_rect(V.vp.x0, y, w, h, col[0], col[1], col[2], OT_LEN - 3);
    shade(th->grass_a[0], th->grass_a[1], th->grass_a[2], (V.fog0 + V.fog1) / 2 + (V.fog1 - V.fog0) / 4, V.light, col);
    gfx_rect(V.vp.x0, y, w, cl(14, 0, h), col[0], col[1], col[2], OT_LEN - 3);
    gfx_rect(V.vp.x0, y, w, cl(5, 0, h), V.fr, V.fg, V.fb, OT_LEN - 3);
}

static void draw_hills(const Theme *th, int yaw) {
    static const uint16_t hp[14][3] = {{0,70,22},{60,110,30},{130,80,18},{190,120,26},{250,60,34},{320,100,20},{390,90,28},
                                      {450,140,16},{520,70,30},{590,120,24},{660,60,18},{720,100,28},{790,80,22},{850,120,20}};
    int span = V.focal * 6283 / 1000;               /* pixeles de una vuelta completa */
    int hs = (V.vp.y1 - V.vp.y0);
    if (V.weather == WE_FOG) return;
    for (int layer = 0; layer < 2; layer++) {
        int col[3], k = layer ? 7 : 10;
        shade(th->hill[0] * k / 10, th->hill[1] * k / 10, th->hill[2] * k / 10, V.fog1 * (layer ? 80 : 95) / 100, V.light, col);
        for (int i = 0; i < 14; i++) {
            int base = (hp[i][0] * 2 + layer * 37) * span / 1700;
            int x = (((base - yaw * span / 4096) % span) + span) % span;
            if (x > span - 200) x -= span;
            x += V.vp.x0 - 40;
            int hh = hp[i][2] * (layer ? 3 : 4) / 4 * hs / 240 + (layer ? 0 : 6), hwid = hp[i][1] * (layer ? 2 : 3) / 2;
            P2 a[3] = {{x - hwid / 2, V.cy + 1}, {x + hwid / 2, V.cy + 1}, {x, V.cy - hh}}, p[12], q[12];
            int m = clip2d(a, 3, p, 0, V.vp.x0, 1); if (m < 3) continue;
            m = clip2d(p, m, q, 0, V.vp.x1 - 1, 0); if (m < 3) continue;
            for (int j = 1; j + 1 < m; j++)
                gfx_tri(q[0].x, q[0].y, q[j].x, q[j].y, q[j + 1].x, q[j + 1].y, col[0], col[1], col[2], OT_LEN - 1 - layer);
        }
    }
}

/* ---------------------------------------------------------- decorado */
static void bq(int X, int Z, int x0, int y0, int x1, int y1, int r, int g, int b, int bias, int light) {
    int c0 = V.camy;
    V3 t[4] = {{X + x0, y0 - c0, Z}, {X + x1, y0 - c0, Z}, {X + x1, y1 - c0, Z}, {X + x0, y1 - c0, Z}};
    poly(t, 4, r, g, b, bias, light);
}
static void btri(int X, int Z, int ax, int ay, int bx, int by, int cx, int cy, int r, int g, int b, int bias, int light) {
    int c0 = V.camy;
    V3 t[3] = {{X + ax, ay - c0, Z}, {X + bx, by - c0, Z}, {X + cx, cy - c0, Z}};
    poly(t, 3, r, g, b, bias, light);
}

static void draw_object(const TrkObj *o) {
    int wx = o->x * (16 / TRK_UNIT), wz = o->z * (16 / TRK_UNIT), S = o->size * 16;
    V3 p; to_cam(wx, 0, wz, &p);
    int X = p.x, Z = p.z, L = V.light;
    if (Z < NEAR + 24 || Z > V.fog1) return;
    if (ab(X) > Z * 3 / 2 + S) return;
    switch (o->kind) {
    case 0: {                                           /* pino */
        int H = S, w1 = H * 28 / 100, w2 = H * 20 / 100, tw = H / 12;
        bq(X, Z, -tw, 0, tw, H * 30 / 100, 90, 62, 34, 1, L);
        btri(X, Z, -w1, H * 16 / 100, w1, H * 16 / 100, 0, H * 66 / 100, 18, 84, 42, 0, L);
        btri(X, Z, -w2, H * 44 / 100, w2, H * 44 / 100, 0, H, 28, 112, 54, -1, L);
        break; }
    case 1: {                                           /* arbol redondo */
        int H = S, R = H * 34 / 100, cy = H * 66 / 100, k;
        static const int ox[8] = {1000, 707, 0, -707, -1000, -707, 0, 707}, oy[8] = {0, 707, 1000, 707, 0, -707, -1000, -707};
        V3 t[8], u[8];
        bq(X, Z, -H / 14, 0, H / 14, H * 40 / 100, 96, 66, 36, 1, L);
        for (k = 0; k < 8; k++) { t[k].x = X + R * ox[k] / 1000; t[k].y = cy + R * oy[k] / 1000 - V.camy; t[k].z = Z;
                                  u[k].x = X - R / 5 + R * 6 / 10 * ox[k] / 1000; u[k].y = cy + R / 5 + R * 6 / 10 * oy[k] / 1000 - V.camy; u[k].z = Z; }
        poly(t, 8, 38, 126, 52, 0, L); poly(u, 8, 62, 160, 70, -1, L);
        break; }
    case 2: {                                           /* roca */
        int H = S;
        int c0 = V.camy;
        V3 t[5] = {{X - H, -c0, Z}, {X - H * 8 / 10, H * 6 / 10 - c0, Z}, {X, H - c0, Z}, {X + H * 9 / 10, H * 5 / 10 - c0, Z}, {X + H, -c0, Z}};
        poly(t, 5, 112, 108, 104, 0, L);
        btri(X, Z, -H * 8 / 10, H * 6 / 10, 0, H, -H / 5, 0, 138, 134, 128, -1, L);
        break; }
    case 3: {                                           /* cactus */
        int H = S * 2, w = H / 9;
        bq(X, Z, -w, 0, w, H, 52, 142, 72, 0, L);
        bq(X, Z, -w * 3, H * 38 / 100, -w, H * 38 / 100 + w, 52, 142, 72, -1, L);
        bq(X, Z, -w * 3, H * 38 / 100, -w * 2, H * 70 / 100, 52, 142, 72, -1, L);
        bq(X, Z, w, H * 52 / 100, w * 3, H * 52 / 100 + w, 52, 142, 72, -1, L);
        bq(X, Z, w * 2, H * 52 / 100, w * 3, H * 80 / 100, 52, 142, 72, -1, L);
        break; }
    case 4: {                                           /* mesa */
        int H = S, W = H * 9 / 10;
        int c0 = V.camy;
        V3 t[4] = {{X - W - W / 4, -c0, Z}, {X + W + W / 4, -c0, Z}, {X + W, H - c0, Z}, {X - W, H - c0, Z}};
        poly(t, 4, 168, 80, 48, 0, L);
        bq(X, Z, -W, H * 70 / 100, W, H, 204, 124, 72, -1, L);
        bq(X, Z, -W - W / 5, 0, W + W / 5, H * 18 / 100, 138, 62, 40, -1, L);
        break; }
    case 5: {                                           /* charco de lava (suelo) */
        int R = S;
        V3 a[4], b[4]; static const int sx[4] = {-1, 1, 1, -1}, sz[4] = {-1, -1, 1, 1};
        for (int k = 0; k < 4; k++) {
            to_cam(wx + sx[k] * R, 1, wz + sz[k] * R, &a[k]);
            to_cam(wx + sx[k] * R / 2, 1, wz + sz[k] * R / 2, &b[k]);
        }
        poly(a, 4, 238, 92, 20, 12, 256); poly(b, 4, 255, 190, 60, 11, 256);
        break; }
    case 6: {                                           /* grada */
        int W = S, H = S * 4 / 10;
        bq(X, Z, -W, 0, W, H * 60 / 100, 150, 152, 160, 1, L);
        for (int k = 0; k < 8; k++) {
            int x0 = -W + k * W / 4;
            bq(X, Z, x0 + 2, H * 14 / 100, x0 + W / 4 - 2, H * 52 / 100, (k & 1) ? 200 : 60, (k & 1) ? 60 : 80, (k & 1) ? 60 : 210, 0, L);
        }
        bq(X, Z, -W - 4, H * 60 / 100, W + 4, H * 76 / 100, 70, 72, 84, -1, L);
        break; }
    default: {                                          /* aguja volcanica */
        int H = S * 2;
        btri(X, Z, -H * 28 / 100, 0, H * 28 / 100, 0, 0, H, 52, 40, 38, 0, L);
        btri(X, Z, -H * 8 / 100, 0, H * 28 / 100, 0, 0, H, 86, 52, 40, -1, L);
        break; }
    }
}

static void draw_scenery(void) {
    const TrackDef *d = TRK.def;
    for (int i = 0; i < d->nobj; i++) draw_object(&d->obj[i]);
}

/* -------------------------------------------------------------- pista */
static V3 lerp3(const V3 *a, const V3 *b, int t16) {
    V3 o; o.x = a->x + ((b->x - a->x) * t16 >> 4); o.y = a->y + ((b->y - a->y) * t16 >> 4); o.z = a->z + ((b->z - a->z) * t16 >> 4);
    return o;
}

static void draw_road(int carIdx, int nv, const Theme *th) {
    const int n = TRK.n, hw = TRK.hw * 16, K = nv + 3;
    static V3 cv[48][10];
    const int off[10] = {hw + 480, hw + 19, hw, 3, -3, -hw, -(hw + 19), -(hw + 480), -(hw + 32), -(hw + 128)};
    if (K > 48) return;
    for (int k = 0; k < K; k++) {
        int i = ((carIdx - 2 + k) % n + n) % n;
        int cx = TRK.px[i] >> 4, cz = TRK.pz[i] >> 4, nx = -TRK.tz[i], nz = TRK.tx[i];
        for (int j = 0; j < 10; j++) to_cam(cx + (nx * off[j] >> 12), 0, cz + (nz * off[j] >> 12), &cv[k][j]);
    }
    int wet = V.weather == WE_RAIN || V.weather == WE_STORM;
    for (int k = 0; k + 1 < K; k++) {
        int i = ((carIdx - 2 + k) % n + n) % n, rel = k - 2, L = V.light;
        if (V.night && rel >= -1 && rel < 8) L = cl(V.light + (8 - rel) * 24, 0, 256);
        const V3 *a = cv[k], *b = cv[k + 1];
        uint8_t rc[3], gc[3], kc[3], lc[3] = {232, 232, 232};
        int sh = (i & 1) ? 5 : -5, gs = ((i >> 1) & 1) ? 0 : 1;
        for (int c = 0; c < 3; c++) {
            int v = th->road[c] + sh; if (wet) v = v * 78 / 100; rc[c] = (uint8_t)cl(v, 0, 255);
            gc[c] = gs ? th->grass_a[c] : th->grass_b[c];
        }
        if (i & 1) { kc[0] = 232; kc[1] = 232; kc[2] = 232; } else { kc[0] = 205; kc[1] = 38; kc[2] = 38; }
        quadw(&a[0], &a[1], &b[1], &b[0], gc, 14, L);
        quadw(&a[6], &a[7], &b[7], &b[6], gc, 14, L);
        quadw(&a[1], &a[2], &b[2], &b[1], kc, 12, L);
        quadw(&a[5], &a[6], &b[6], &b[5], kc, 12, L);
        quadw(&a[2], &a[5], &b[5], &b[2], rc, 13, L);
        if (!(i & 1)) quadw(&a[3], &a[4], &b[4], &b[3], lc, 10, L);
        if (i < 10) { uint8_t pc[3] = {98, 98, 106}; quadw(&a[8], &a[9], &b[9], &b[8], pc, 11, L); }
        if (i == 0) {                                   /* linea de meta a cuadros */
            static const uint8_t wh[3] = {240, 240, 240}, bl[3] = {20, 20, 20};
            for (int col = 0; col < 6; col++) for (int row = 0; row < 2; row++) {
                int u0 = col * 16 / 6, u1 = (col + 1) * 16 / 6, v0 = row * 3, v1 = row * 3 + 3;
                V3 p00 = lerp3(&a[2], &a[5], u0), p01 = lerp3(&a[2], &a[5], u1);
                V3 q00 = lerp3(&b[2], &b[5], u0), q01 = lerp3(&b[2], &b[5], u1);
                V3 A = lerp3(&p00, &q00, v0), B = lerp3(&p01, &q01, v0), C = lerp3(&p01, &q01, v1), D = lerp3(&p00, &q00, v1);
                quadw(&A, &B, &C, &D, ((col + row) & 1) ? bl : wh, 9, L);
            }
        }
        /* postes laterales */
        if (i % 3 == 0) {
            int cx = TRK.px[i] >> 4, cz = TRK.pz[i] >> 4;
            for (int sd = -1; sd <= 1; sd += 2) {
                int o2 = sd * (hw + 52), nx = -TRK.tz[i], nz = TRK.tx[i];
                int red = (i / 3) & 1;
                draw_box(cx + (nx * o2 >> 12), cz + (nz * o2 >> 12), 0, 0, 0, 0, 3, 14, 3, red ? 210 : 235, red ? 40 : 235, red ? 40 : 235, 0, L);
            }
        }
        if (i == 0) {                                   /* portico de salida */
            int cx = TRK.px[i] >> 4, cz = TRK.pz[i] >> 4, hd = TRK.heading[i];
            draw_box(cx, cz, hd, 0, 78, 0, hw + 40, 22, 6, 200, 34, 34, 0, L);
            draw_box(cx, cz, hd, -(hw + 30), 0, 0, 7, 80, 7, 210, 210, 214, 0, L);
            draw_box(cx, cz, hd, (hw + 30), 0, 0, 7, 80, 7, 210, 210, 214, 0, L);
        }
        if (i == 1 || i == 3 || i == 5 || i == 7) {     /* garajes de boxes */
            int cx = TRK.px[i] >> 4, cz = TRK.pz[i] >> 4, hd = TRK.heading[i];
            draw_box(cx, cz, hd, -(hw + 190), 0, 0, 46, 44, 62, 84, 96, 132, 0, L);
            draw_box(cx, cz, hd, -(hw + 150), 0, 0, 2, 30, 36, 20, 20, 24, 0, L);
        }
    }
}

/* ---------------------------------------------------------- mundo */
static void rain_fx(int frame) {
    if (V.weather != WE_RAIN && V.weather != WE_STORM) return;
    int n = V.weather == WE_STORM ? 80 : 42, w = V.vp.x1 - V.vp.x0, h = V.vp.y1 - V.vp.y0;
    for (int i = 0; i < n; i++) {
        unsigned hsh = (unsigned)(i * 2654435761u) ^ 0x9e3779b9u;
        int x = V.vp.x0 + (int)((hsh >> 8) % (unsigned)w);
        int y = V.vp.y0 + (int)(((hsh >> 3) + (unsigned)frame * 19u) % (unsigned)h);
        int len = 6 + (int)(hsh % 6);
        gfx_line(x, y, x - 2, y + len, 170, 180, 205, 2);
    }
}

void cam_update(Cam *cam, const Car *c) {
    int h = (c->heading >> 8) & 4095;
    if (!cam->init) { cam->yaw = h; cam->init = 1; }
    int d = ((h - cam->yaw + 6144) & 4095) - 2048;
    cam->yaw = (cam->yaw + d / 5) & 4095;
    if (cam->mode == 2) cam->yaw = h;
}

void render_world(const Race *r, int ci, Cam *cam, Rect vp, int frame) {
    const Car *c = &r->car[ci];
    const Theme *th = &THEMES[TRACK_DEFS[r->track].theme];
    env_setup(TRACK_DEFS[r->track].theme, r->weather, r->tod);
    V.vp = vp; V.cx = (vp.x0 + vp.x1) / 2; V.cy = vp.y0 + (vp.y1 - vp.y0) * 40 / 100;
    V.focal = (vp.x1 - vp.x0) * 69 / 100;
    static const int dist[3] = {112, 176, -8}, hgt[3] = {44, 66, 19};
    int m = cam->mode % 3;
    V.sn = isin(cam->yaw); V.cs = icos(cam->yaw);
    V.camx = (c->x >> 4) - (V.sn * dist[m] >> 12);
    V.camz = (c->z >> 4) - (V.cs * dist[m] >> 12);
    V.camy = hgt[m];
    draw_sky(); draw_ground_bg(th); draw_hills(th, cam->yaw);
    int nv = (vp.y1 - vp.y0) < 150 ? 22 : 34;
    if (V.fog1 < 200 * 16) nv = cl(V.fog1 / 160 + 3, 8, nv);
    draw_road(c->idx, nv, th);
    draw_scenery();
    for (int i = 0; i < r->ncars; i++) {
        const Car *o = &r->car[i]; uint8_t rgb[3];
        if (o->color >= 12) { rgb[0] = MODELS[o->model].r; rgb[1] = MODELS[o->model].g; rgb[2] = MODELS[o->model].b; }
        else { rgb[0] = PALETTE[o->color][0]; rgb[1] = PALETTE[o->color][1]; rgb[2] = PALETTE[o->color][2]; }
        draw_car_model(o->x >> 4, o->z >> 4, (o->heading >> 8) & 4095, o->model, rgb, o->spoiler || (i & 1), o->braking, i == ci && m == 2);
    }
    rain_fx(frame);
}

void render_backdrop(int theme, int weather, int tod, Rect vp) {
    env_setup(theme, weather, tod);
    V.vp = vp; V.cx = (vp.x0 + vp.x1) / 2; V.cy = vp.y0 + (vp.y1 - vp.y0) * 55 / 100; V.focal = (vp.x1 - vp.x0) * 69 / 100;
    V.camx = V.camz = 0; V.camy = 20; V.sn = 0; V.cs = 4096;
    draw_sky(); draw_ground_bg(&THEMES[theme & 3]); draw_hills(&THEMES[theme & 3], 300);
}

void render_car_preview(const Owned *o, Rect vp, int angle) {
    uint8_t rgb[3]; owned_rgb(o, rgb);
    env_setup(0, WE_SUNNY, TOD_DAY);
    V.vp = vp; V.cx = (vp.x0 + vp.x1) / 2; V.cy = vp.y0 + (vp.y1 - vp.y0) * 22 / 100; V.focal = (vp.x1 - vp.x0) * 100 / 100;
    V.camx = 0; V.camz = -118; V.camy = 40; V.sn = 0; V.cs = 4096;
    const Theme *th = &THEMES[0];
    draw_sky(); draw_ground_bg(th);
    {   /* plataforma */
        V3 p[4]; uint8_t pc[3] = {96, 98, 108}; static const int sx[4] = {-1, 1, 1, -1}, sz[4] = {-1, -1, 1, 1};
        for (int k = 0; k < 4; k++) to_cam(sx[k] * 150, 0, sz[k] * 150, &p[k]);
        quadw(&p[0], &p[1], &p[2], &p[3], pc, 12, V.light);
        for (int k = 0; k < 4; k++) to_cam(sx[k] * 150 + 0, 0, sz[k] * 150, &p[k]);
    }
    int braking = 0;
    draw_car_model(0, 0, angle & 4095, o->model, rgb, o->spoiler, braking, 0);
}

/* ------------------------------------------------------- utilidades UI */
void ui_panel(int x, int y, int w, int h, int z) {
    gfx_rect(x, y, w, h, 14, 18, 30, z + 1);
    gfx_rect(x, y, w, 1, 230, 190, 40, z);
    gfx_rect(x, y + h - 1, w, 1, 90, 70, 20, z);
    gfx_rect(x, y, 1, h, 120, 100, 30, z);
    gfx_rect(x + w - 1, y, 1, h, 120, 100, 30, z);
}

void ui_bar(int x, int y, int w, int h, int pct, int r, int g, int b) {
    pct = cl(pct, 0, 100);
    gfx_rect(x, y, w, h, 40, 44, 56, 6);
    if (pct > 0) gfx_rect(x, y, w * pct / 100 > 0 ? w * pct / 100 : 1, h, r, g, b, 5);
}

void ui_digits(int x, int y, int h, int value, int ndig, int r, int g, int b) {
    static const uint8_t seg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    int w = h * 5 / 9, t = h / 8 > 1 ? h / 8 : 1, d;
    if (value < 0) value = 0;
    for (d = ndig - 1; d >= 0; d--) {
        int dg = value % 10; value /= 10;
        int ox = x + d * (w + t * 2), s = seg[dg];
        if (d < ndig - 1 && dg == 0 && value == 0 && ndig > 1 && ((d == 0) ? 0 : 0)) s = 0;
        int hh = h / 2;
        if (s & 1)  gfx_rect(ox + t, y, w - t * 2, t, r, g, b, 3);
        if (s & 2)  gfx_rect(ox + w - t, y + t, t, hh - t, r, g, b, 3);
        if (s & 4)  gfx_rect(ox + w - t, y + hh, t, hh - t, r, g, b, 3);
        if (s & 8)  gfx_rect(ox + t, y + h - t, w - t * 2, t, r, g, b, 3);
        if (s & 16) gfx_rect(ox, y + hh, t, hh - t, r, g, b, 3);
        if (s & 32) gfx_rect(ox, y + t, t, hh - t, r, g, b, 3);
        if (s & 64) gfx_rect(ox + t, y + hh - t / 2, w - t * 2, t, r, g, b, 3);
    }
}

/* ---------------------------------------------------------------- HUD */
static int mm_track = -1, mm_minx, mm_minz, mm_scale;   /* escala del minimapa */

static void minimap(int x, int y, int w, int h, const Race *r, int ci) {
    if (mm_track != r->track) {
        int maxx, maxz; mm_minx = mm_minz = 0x7fffffff; maxx = maxz = -0x7fffffff;
        for (int i = 0; i < TRK.n; i++) {
            if (TRK.px[i] < mm_minx) mm_minx = TRK.px[i]; if (TRK.px[i] > maxx) maxx = TRK.px[i];
            if (TRK.pz[i] < mm_minz) mm_minz = TRK.pz[i]; if (TRK.pz[i] > maxz) maxz = TRK.pz[i];
        }
        int sx = (maxx - mm_minx) / (w - 6) + 1, sz = (maxz - mm_minz) / (h - 6) + 1;
        mm_scale = sx > sz ? sx : sz; mm_track = r->track;
    }
    gfx_rect(x, y, w, h, 10, 14, 24, 8);
    for (int i = 0; i < TRK.n; i += 2)
        gfx_rect(x + 3 + (TRK.px[i] - mm_minx) / mm_scale, y + 3 + (TRK.pz[i] - mm_minz) / mm_scale, 2, 2, 150, 154, 170, 6);
    for (int i = 0; i < r->ncars; i++) {
        const Car *c = &r->car[i];
        int px = x + 2 + (c->x - mm_minx) / mm_scale, py = y + 2 + (c->z - mm_minz) / mm_scale;
        if (i == ci) gfx_rect(px - 1, py - 1, 4, 4, 255, 255, 60, 4);
        else gfx_rect(px, py, 3, 3, c->is_ai ? 230 : 90, c->is_ai ? 60 : 200, c->is_ai ? 60 : 255, 4);
    }
}

void render_hud(const Race *r, int ci, Rect vp, int compact, int frame) {
    const Car *c = &r->car[ci];
    char t[48], tb[12];
    int x0 = vp.x0, y0 = vp.y0, w = vp.x1 - vp.x0, h = vp.y1 - vp.y0;
    int lap = c->lap + 1; if (lap < 1) lap = 1; if (lap > r->nlaps) lap = r->nlaps;
    /* cuenta atras */
    if (r->count > 0) {
        int sec = (r->count + 59) / 60 - 1;               /* 3,2,1,0 */
        if (r->state == 0 && sec >= 1 && sec <= 3) ui_digits(x0 + w / 2 - 10, y0 + h / 5, compact ? 24 : 44, sec, 1, 255, 210, 40);
        else if (r->count > 0 && r->state >= 1) gfx_text(x0 + w / 2 - 12, y0 + h / 5, "GO!");
    }
    /* velocidad */
    {
        int sh = compact ? 22 : 34, sx = x0 + w - (compact ? 58 : 88);
        ui_digits(sx, y0 + h - sh - (compact ? 14 : 22), sh, c->kmh, 3, 245, 245, 250);
        gfx_text(sx + (compact ? 12 : 20), y0 + h - (compact ? 12 : 18), "km/h");
        /* marcha y rpm */
        int gx = sx - (compact ? 22 : 34);
        if (c->gear < 0) gfx_text(gx, y0 + h - sh - (compact ? 14 : 22) + 6, "R");
        else ui_digits(gx, y0 + h - sh - (compact ? 14 : 22) + (compact ? 0 : 6), compact ? 16 : 24, c->gear, 1, 255, 190, 40);
        int segs = compact ? 8 : 12, on = c->rpm * segs / 100;
        for (int i = 0; i < segs; i++) {
            int rr = i < segs * 6 / 10 ? 70 : (i < segs * 8 / 10 ? 240 : 235), gg = i < segs * 6 / 10 ? 200 : (i < segs * 8 / 10 ? 200 : 50), bb = i < segs * 6 / 10 ? 90 : 40;
            gfx_rect(sx + i * (compact ? 4 : 6) - (compact ? 6 : 14), y0 + h - sh - (compact ? 22 : 34), compact ? 3 : 5, compact ? 4 : 6, i < on ? rr : 45, i < on ? gg : 48, i < on ? bb : 56, 4);
        }
    }
    /* posicion y vueltas */
    {
        char pc[8];
        pc[0] = (char)('0' + c->rank + 1); pc[1] = '/'; pc[2] = (char)('0' + r->ncars); pc[3] = 0;
        gfx_text(x0 + 6, y0 + 6, "POS");
        gfx_text(x0 + 6 + 32, y0 + 6, pc);
        t[0] = 'V'; t[1] = 'U'; t[2] = 'E'; t[3] = 'L'; t[4] = 'T'; t[5] = 'A'; t[6] = ' '; t[7] = (char)('0' + lap); t[8] = '/';
        if (r->nlaps >= 10) { t[9] = (char)('0' + r->nlaps / 10); t[10] = (char)('0' + r->nlaps % 10); t[11] = 0; }
        else { t[9] = (char)('0' + r->nlaps); t[10] = 0; }
        gfx_text(x0 + 6, y0 + 16, t);
        if (!compact) {
            format_time(tb, r->state >= 1 ? r->ticks : 0);
            gfx_text(x0 + 6, y0 + 26, tb);
            if (c->best_lap) { format_time(tb, c->best_lap); gfx_text(x0 + 6, y0 + 36, "MEJ"); gfx_text(x0 + 6 + 32, y0 + 36, tb); }
        }
    }
    if (!compact) minimap(x0 + w - 70, y0 + 6, 64, 50, r, ci);
    /* barras de recursos */
    {
        int bx = x0 + 6, by = y0 + h - (compact ? 30 : 44), bw = compact ? 44 : 64, bh = compact ? 4 : 5;
        if (c->nitro_idx >= 0 && r->nitro_on) {
            int cap = NITRO[c->nitro_idx].cap * 100;
            gfx_text(bx, by - 8, "N2O");
            ui_bar(bx + 28, by - 6, bw, bh, c->nitro_gauge * 100 / cap, c->nitro_on ? 255 : 40, c->nitro_on ? 220 : 190, 255);
        }
        gfx_text(bx, by + 2, "GAS");
        ui_bar(bx + 28, by + 4, bw, bh, c->fuel / 6000, c->fuel < 90000 ? 255 : 70, c->fuel < 90000 ? 60 : 200, 60);
        const TyreDef *td = &TYRES[c->tyre];
        gfx_text(bx, by + 12, "NEU");
        ui_bar(bx + 28, by + 14, bw, bh, 100 - c->wear / 1000, td->r, td->g, td->b);
    }
    /* boxes */
    if (c->pit_state == 2) { gfx_text(x0 + w / 2 - 60, y0 + h / 3, "** PARADA EN BOXES **"); }
    else if (c->pit_state == 1) gfx_text(x0 + w / 2 - 44, y0 + h / 3, "ENTRANDO A BOXES");
    else if (c->pit_req) {
        t[0] = 'B'; t[1] = 'O'; t[2] = 'X'; t[3] = 'E'; t[4] = 'S'; t[5] = ':'; t[6] = ' ';
        const char *nm = TYRES[c->pit_tyre].name; int k = 0; while (nm[k] && k < 20) { t[7 + k] = nm[k]; k++; } t[7 + k] = 0;
        gfx_text(x0 + w / 2 - 50, y0 + h - 12 - (compact ? 0 : 0), t);
    }
    if (r->state == 2 && c->finished) gfx_text(x0 + w / 2 - 36, y0 + h / 3, "META!  START");
    (void)frame;
}

void ui_frame(int x, int y, int w, int h, int z) {
    gfx_rect(x, y, w, 1, 230, 190, 40, z);
    gfx_rect(x, y + h - 1, w, 1, 90, 70, 20, z);
    gfx_rect(x, y, 1, h, 120, 100, 30, z);
    gfx_rect(x + w - 1, y, 1, h, 120, 100, 30, z);
}

void ui_trackmap(int track, int x, int y, int w, int h) {
    const TrackDef *d = &TRACK_DEFS[track];
    int minx = 0x7fffffff, maxx = -0x7fffffff, minz = 0x7fffffff, maxz = -0x7fffffff;
    for (int i = 0; i < d->npts; i++) {
        int px = d->pts[i * 2], pz = d->pts[i * 2 + 1];
        if (px < minx) minx = px; if (px > maxx) maxx = px;
        if (pz < minz) minz = pz; if (pz > maxz) maxz = pz;
    }
    int sx = (maxx - minx) / (w - 8) + 1, sz = (maxz - minz) / (h - 8) + 1, sc = sx > sz ? sx : sz;
    int ox = x + (w - (maxx - minx) / sc) / 2, oy = y + (h - (maxz - minz) / sc) / 2;
    for (int i = 0; i < d->npts; i++)
        gfx_rect(ox + (d->pts[i * 2] - minx) / sc, oy + (d->pts[i * 2 + 1] - minz) / sc, 3, 3, 170, 175, 195, 6);
    gfx_rect(ox + (d->pts[0] - minx) / sc - 1, oy + (d->pts[1] - minz) / sc - 1, 5, 5, 255, 220, 50, 4);
}
