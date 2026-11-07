/*
 * GT7 PS1 Edition - capa de plataforma para PlayStation 1 (PSn00bSDK)
 * Convierte las llamadas gfx_* del motor en primitivas del GPU y lee los mandos.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <psxgpu.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include "ui.h"

#define BUFFER_LENGTH 98304      /* bytes de primitivas por fotograma */

typedef struct {
    DISPENV disp_env;
    DRAWENV draw_env;
    uint32_t ot[OT_LEN];
    uint8_t buffer[BUFFER_LENGTH];
} RenderBuffer;

static RenderBuffer bufs[2];
static int active;
static uint8_t *next_packet;
static uint8_t *packet_end;
static uint32_t *cur_ot;

static void flip_buffers(void) {
    DrawSync(0);
    VSync(0);
    RenderBuffer *draw_buffer = &bufs[active];
    RenderBuffer *disp_buffer = &bufs[active ^ 1];
    PutDispEnv(&disp_buffer->disp_env);
    DrawOTagEnv(&draw_buffer->ot[OT_LEN - 1], &draw_buffer->draw_env);
    active ^= 1;
    next_packet = bufs[active].buffer;
    packet_end = bufs[active].buffer + BUFFER_LENGTH;
    cur_ot = bufs[active].ot;
    ClearOTagR(cur_ot, OT_LEN);
}

static void setup_video(void) {
    int pal = (GetVideoMode() == MODE_PAL);
    for (int i = 0; i < 2; i++) {
        SetDefDrawEnv(&bufs[i].draw_env, 0, i ? SCREEN_H : 0, SCREEN_W, SCREEN_H);
        SetDefDispEnv(&bufs[i].disp_env, 0, i ? SCREEN_H : 0, SCREEN_W, SCREEN_H);
        setRGB0(&bufs[i].draw_env, 0, 0, 0);
        bufs[i].draw_env.isbg = 1;
        if (pal) { bufs[i].disp_env.screen.y = 8; bufs[i].disp_env.screen.h = 240; }
    }
    active = 0;
    next_packet = bufs[0].buffer;
    packet_end = bufs[0].buffer + BUFFER_LENGTH;
    cur_ot = bufs[0].ot;
    ClearOTagR(cur_ot, OT_LEN);
    SetDispMask(1);
}

/* ------------------------------------------------ backend grafico del motor */
static inline int zclamp(int z) { return z < 0 ? 0 : (z >= OT_LEN ? OT_LEN - 1 : z); }

void gfx_tri(int x0, int y0, int x1, int y1, int x2, int y2, int r, int g, int b, int z) {
    int minx = x0, maxx = x0, miny = y0, maxy = y0;
    if (x1 < minx) minx = x1; if (x1 > maxx) maxx = x1;
    if (x2 < minx) minx = x2; if (x2 > maxx) maxx = x2;
    if (y1 < miny) miny = y1; if (y1 > maxy) maxy = y1;
    if (y2 < miny) miny = y2; if (y2 > maxy) maxy = y2;
    if (maxx < 0 || minx >= SCREEN_W || maxy < 0 || miny >= SCREEN_H) return;
    if (maxx - minx > 1000 || maxy - miny > 500) return;          /* el GPU descarta poligonos enormes */
    if (minx < -1000 || maxx > 1000 || miny < -1000 || maxy > 1000) return;
    if (next_packet + sizeof(POLY_F3) > packet_end) return;
    POLY_F3 *p = (POLY_F3 *)next_packet;
    next_packet += sizeof(POLY_F3);
    setPolyF3(p);
    setXY3(p, x0, y0, x1, y1, x2, y2);
    setRGB0(p, r, g, b);
    addPrim(&cur_ot[zclamp(z)], p);
}

void gfx_rect(int x, int y, int w, int h, int r, int g, int b, int z) {
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0; if (y < 0) y = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W; if (y1 > SCREEN_H) y1 = SCREEN_H;
    if (x1 <= x || y1 <= y) return;
    if (next_packet + sizeof(TILE) > packet_end) return;
    TILE *t = (TILE *)next_packet;
    next_packet += sizeof(TILE);
    setTile(t);
    setXY0(t, x, y);
    setWH(t, x1 - x, y1 - y);
    setRGB0(t, r, g, b);
    addPrim(&cur_ot[zclamp(z)], t);
}

void gfx_line(int x0, int y0, int x1, int y1, int r, int g, int b, int z) {
    if ((x0 < 0 && x1 < 0) || (y0 < 0 && y1 < 0) || (x0 >= SCREEN_W && x1 >= SCREEN_W) || (y0 >= SCREEN_H && y1 >= SCREEN_H)) return;
    if (x0 < -1000 || x0 > 1000 || x1 < -1000 || x1 > 1000 || y0 < -500 || y0 > 500 || y1 < -500 || y1 > 500) return;
    if (next_packet + sizeof(LINE_F2) > packet_end) return;
    LINE_F2 *l = (LINE_F2 *)next_packet;
    next_packet += sizeof(LINE_F2);
    setLineF2(l);
    setXY2(l, x0, y0, x1, y1);
    setRGB0(l, r, g, b);
    addPrim(&cur_ot[zclamp(z)], l);
}

void gfx_text(int x, int y, const char *s) {
    if (next_packet + 64 + 40 * (int)strlen(s) > packet_end) return;
    next_packet = (uint8_t *)FntSort(&cur_ot[0], next_packet, x, y, s);
}

/* -------------------------------------------------------------- mandos */
static uint8_t pad_buff[2][34];

static void read_pad(int port, uint16_t *prev, Pad *out) {
    PADTYPE *pad = (PADTYPE *)pad_buff[port];
    uint16_t hw = 0, bt = 0;
    int stick = 0;
    if (pad->stat == 0) {
        hw = (uint16_t)~pad->btn;                         /* activo a nivel bajo */
        if (pad->type == PAD_ID_ANALOG || pad->type == PAD_ID_ANALOG_STICK) stick = (int)pad->ls_x - 128;
    }
    if (hw & PAD_UP) bt |= BT_UP;
    if (hw & PAD_DOWN) bt |= BT_DOWN;
    if (hw & PAD_LEFT) bt |= BT_LEFT;
    if (hw & PAD_RIGHT) bt |= BT_RIGHT;
    if (hw & PAD_CROSS) bt |= BT_A;
    if (hw & PAD_CIRCLE) bt |= BT_B;
    if (hw & PAD_SQUARE) bt |= BT_X;
    if (hw & PAD_TRIANGLE) bt |= BT_Y;
    if (hw & PAD_L1) bt |= BT_L1;
    if (hw & PAD_R1) bt |= BT_R1;
    if (hw & PAD_L2) bt |= BT_L2;
    if (hw & PAD_R2) bt |= BT_R2;
    if (hw & PAD_START) bt |= BT_START;
    if (hw & PAD_SELECT) bt |= BT_SELECT;
    out->held = bt;
    out->down = (uint16_t)(bt & ~*prev);
    out->stick = stick;
    *prev = bt;
}

/* ---------------------------------------------------------------- main */
static App app;

int main(int argc, const char **argv) {
    static uint16_t prev[2];
    int pal;
    ResetGraph(0);
    setup_video();
    FntLoad(960, 0);
    InitPAD(pad_buff[0], 34, pad_buff[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    app_init(&app);
    pal = (GetVideoMode() == MODE_PAL);

    int last = VSync(-1), acc = 0;
    while (1) {
        Pad pads[2];
        int now = VSync(-1), dv = now - last, ticks;
        last = now;
        if (dv < 1) dv = 1;
        if (dv > 4) dv = 4;
        acc += dv * 60;                                  /* la fisica va a 60 Hz */
        ticks = acc / (pal ? 50 : 60);
        acc -= ticks * (pal ? 50 : 60);
        if (ticks > 4) ticks = 4;

        memset(pads, 0, sizeof pads);
        read_pad(0, &prev[0], &pads[0]);
        read_pad(1, &prev[1], &pads[1]);
        app_frame(&app, pads, ticks);
        flip_buffers();
    }
    return 0;
}
