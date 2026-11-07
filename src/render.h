/* GT7 PS1 - render 3D por software (independiente de la consola).
 * La plataforma implementa gfx_* ; en PS1 se convierten en primitivas del GPU. */
#ifndef RENDER_H
#define RENDER_H
#include "game.h"

/* ---- backend grafico (lo implementa main.c en PS1 / pc_gfx.c en PC) ---- */
void gfx_tri(int x0, int y0, int x1, int y1, int x2, int y2, int r, int g, int b, int z);
void gfx_rect(int x, int y, int w, int h, int r, int g, int b, int z);
void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b, int z);
void gfx_text(int x, int y, const char *s);

#define SCREEN_W 320
#define SCREEN_H 240
#define OT_LEN   1024

typedef struct { int x0, y0, x1, y1; } Rect;
typedef struct { int yaw; int mode; int init; } Cam;

void cam_update(Cam *cam, const Car *c);
void render_world(const Race *r, int ci, Cam *cam, Rect vp, int frame);
void render_hud(const Race *r, int ci, Rect vp, int compact, int frame);
void render_car_preview(const Owned *o, Rect vp, int angle);
void render_backdrop(int theme, int weather, int tod, Rect vp);   /* fondo de menus */

/* utilidades de dibujo para los menus */
void ui_panel(int x, int y, int w, int h, int z);
void ui_frame(int x, int y, int w, int h, int z);
void ui_trackmap(int track, int x, int y, int w, int h);
void ui_bar(int x, int y, int w, int h, int pct, int r, int g, int b);
void ui_digits(int x, int y, int h, int value, int ndig, int r, int g, int b);

#endif
