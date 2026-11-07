/* GT7 PS1 - menus y bucle de la aplicacion (independiente de la consola) */
#ifndef UI_H
#define UI_H
#include "render.h"

enum { BT_UP = 1, BT_DOWN = 2, BT_LEFT = 4, BT_RIGHT = 8, BT_A = 16, BT_B = 32, BT_X = 64, BT_Y = 128,
       BT_L1 = 256, BT_R1 = 512, BT_L2 = 1024, BT_R2 = 2048, BT_START = 4096, BT_SELECT = 8192 };

typedef struct { uint16_t down, held; int stick; } Pad;       /* stick: -127..127 (0 = sin analogico) */

enum { S_TITLE, S_MENU, S_SETUP, S_GARAGE, S_TUNE, S_DEALER, S_USED, S_RACE, S_RESULT };

typedef struct {
    int state, sel, sel2, cat, frame, angle;
    int paused, pause_sel, confirm;
    int msg_timer; char msg[40];
    Save save;
    RaceCfg cfg; int car2;                 /* ajustes de carrera, coche del jugador 2 */
    Race race; Cam cam[2];
    int result_wait;
} App;

void app_init(App *a);
void app_frame(App *a, const Pad pads[2], int nticks);

#endif
