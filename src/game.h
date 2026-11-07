/* GT7 PS1 - logica del juego (sin dependencias de consola, solo enteros) */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include "tracks.h"

#define MAXCARS   6
#define MAXOWN    8
#define NLIST     6
#define NPARTS    43
#define NCAT      7
#define NTYRES    5

/* unidades: posicion 1/256 m, velocidad 1/65536 m por tick (60 ticks/s) */
#define POS_ONE 256

enum { CAR_PORSCHE, CAR_FERRARI, CAR_MCLAREN, CAR_MERCEDES, CAR_ASTON,
       CAR_RAYO, CAR_F1, CAR_KART, NMODELS };
enum { SH_GT, SH_SUPER, SH_TOON, SH_F1, SH_KART };
enum { WE_SUNNY, WE_RAIN, WE_STORM, WE_FOG, NWEATHER };
enum { TOD_DAY, TOD_DUSK, TOD_NIGHT, NTOD };
enum { TY_SOFT, TY_MEDIUM, TY_HARD, TY_INTER, TY_WET };

typedef struct {
    const char *name;
    int price;                 /* precio nuevo */
    int vmax_kmh;              /* velocidad maxima base */
    int accel, handling;       /* x100 */
    uint8_t r, g, b;           /* color por defecto */
    uint8_t shape;
} CarModel;

typedef struct {
    const char *name;
    uint16_t price;
    int8_t gs, ga, gh, gn;     /* ganancias velocidad / aceleracion / manejo / nitro */
    uint8_t cat;
    int8_t nitro;              /* indice en NITRO[] o -1 */
} Part;

typedef struct { const char *name; int cap, cons, regen, bcap, baccel; } NitroDef; /* cons/regen x100, bcap x100, baccel x1000 */
typedef struct { const char *name; int grip, wear; uint8_t r, g, b; } TyreDef;

extern const CarModel MODELS[NMODELS];
extern const Part PARTS[NPARTS];
extern const char *CAT_NAMES[NCAT];
extern const NitroDef NITRO[7];
extern const TyreDef TYRES[NTYRES];
extern const uint8_t PALETTE[12][3];
extern const char *WEATHER_NAMES[NWEATHER];
extern const char *TOD_NAMES[NTOD];

typedef struct {
    uint8_t model, color, cond, spoiler;
    uint16_t km;
    uint8_t parts[NPARTS];
} Owned;

typedef struct {
    int vmax_kmh, accel, grip;          /* efectivos (accel/grip x100) */
    int bar_speed, bar_accel, bar_grip; /* 0..100 */
    int nitro;                          /* -1 sin nitro */
    int pp;                             /* puntuacion de rendimiento */
} Stats;

typedef struct { uint8_t model, color, cond; uint16_t km; uint8_t nparts, parts[4]; int price; } Listing;

typedef struct {
    int credits;
    int nown, cur;
    Owned own[MAXOWN];
    Listing used[NLIST];
    uint32_t mseed;
    int races;
} Save;

/* ---- matematicas ---- */
void math_init(void);
int isin(int a);                 /* 4096 = 1.0, a: 0..4095 = 360 grados */
int icos(int a);
int isin14(int a);               /* 16384 = 1.0 (fisica) */
int icos14(int a);
int iatan2(int y, int x);        /* 0..4095 */
int isqrt32(uint32_t v);
uint32_t rnd(void);
void rnd_seed(uint32_t s);
int rnd_range(int n);

/* ---- coches / tuning / economia ---- */
void save_default(Save *s);
void owned_rgb(const Owned *o, uint8_t rgb[3]);
void car_stats(const Owned *o, Stats *st);
int owned_value(const Owned *o);                /* precio de venta */
int parts_total(const Owned *o);
int buy_part(Save *s, int slot, int part);      /* 1 ok, 0 sin dinero / ya la tienes */
int sell_part(Save *s, int slot, int part);
int dealer_buy(Save *s, int model);
int sell_car(Save *s, int slot);                /* no deja vender el ultimo */
void market_refresh(Save *s);
int used_buy(Save *s, int listing);

/* ---- pista (datos en tiempo de ejecucion) ---- */
typedef struct {
    int n, hw;                      /* puntos, semiancho (m) */
    int32_t px[256], pz[256];       /* 1/256 m */
    int16_t tx[256], tz[256];       /* tangente 12 bits */
    int vm[256];                    /* limite de curva (unidades V) */
    int heading[256];
    const TrackDef *def;
    int length_m;
} Trk;
extern Trk TRK;
void trk_load(int id);

/* ---- carrera ---- */
typedef struct { int thr, brk, steer, nitro, pit, tyre; } PadIn; /* steer -256..256 */

typedef struct {
    int32_t x, z, vx, vz;
    int heading;                    /* 1/256 de unidad: 0..(4096<<8)-1 */
    int steer;
    int vmax, accel, grip;          /* V, V/tick2, x100 */
    int nitro_idx, nitro_gauge, nitro_on;
    int idx, lap, prog;             /* lap empieza en -1 en parrilla */
    int finished, finish_time, rank;
    int lap_start, best_lap, last_lap;
    int fuel, wear, tyre;           /* 1/10000 L, 0..100000, tipo */
    int pit_req, pit_state, pit_timer, pit_tyre, pits;
    int is_ai, skill, lane;
    int model, color, spoiler;
    int kmh, rpm, gear, braking, offroad, lat_off, drift;
    int in_thr, in_brk, in_steer;
    int pp_vmax, auto_drive, vmax_eff;
} Car;

typedef struct {
    int track, nlaps, weather, tod, nitro_on, ncars, nhumans, start_tyre;
    Car car[MAXCARS];
    int state;                      /* 0 cuenta atras, 1 carrera, 2 final */
    int ticks, count;
    int reward, reward_done;
} Race;

typedef struct { int track, laps, weather, tod, nitro, tyre, players; } RaceCfg;

void race_init(Race *r, const RaceCfg *cfg, const Owned *p1, const Owned *p2);
void race_tick(Race *r, const PadIn in[2]);
int race_reward_for(const Race *r, int rank);
void race_finish_save(Race *r, Save *s);  /* paga premio y desgasta el coche */
int weather_grip(int weather, int tyre);  /* x100 */
void format_time(char *out, int ticks);   /* m:ss.cc */

#endif
