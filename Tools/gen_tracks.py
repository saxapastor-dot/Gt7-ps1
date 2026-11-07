#!/usr/bin/env python3
"""Genera src/tracks.h con 4 circuitos cerrados (puntos de la linea central,
limite de velocidad en curva y decorado). Ejecutar: python3 tools/gen_tracks.py"""
import math, random, sys

U = 8  # unidades por metro en tracks.h (1/8 m)

TRACKS = [
 # id, nombre, semilla, radio medio (m), armonicos (k, amp, fase), semieje z, ancho semi (m), tema
 dict(key="classic", name="Classic GP Circuit", seed=11, R=280, sz=0.72, hw=9,
      harm=[(2,0.20,0.3),(3,0.14,1.1),(5,0.07,2.0),(7,0.025,0.5)], theme=0, laps=3, reward=2000),
 dict(key="forest", name="Forest Ring", seed=23, R=262, sz=0.80, hw=8,
      harm=[(3,0.17,0.8),(4,0.13,2.2),(6,0.08,0.4),(9,0.02,1.0)], theme=1, laps=4, reward=3000),
 dict(key="volcano", name="Circuito Volcanico", seed=37, R=300, sz=0.62, hw=9,
      harm=[(2,0.30,2.4),(4,0.12,0.2),(7,0.05,1.3)], theme=2, laps=3, reward=3500),
 dict(key="canyon", name="Canon Rojo", seed=41, R=300, sz=0.78, hw=10,
      harm=[(2,0.18,1.6),(3,0.17,0.1),(5,0.08,1.8),(8,0.02,0.7)], theme=3, laps=3, reward=3500),
]

def centerline(t, N=480):
    pts = []
    for i in range(N):
        th = 2*math.pi*i/N
        r = t["R"]*(1+sum(a*math.cos(k*th+ph) for k,a,ph in t["harm"]))
        pts.append((r*math.cos(th), r*math.sin(th)*t["sz"]))
    return pts

def resample(pts, step):
    # longitud acumulada y remuestreo uniforme
    n = len(pts); d=[0.0]
    for i in range(1,n+1):
        a=pts[i-1]; b=pts[i%n]; d.append(d[-1]+math.hypot(b[0]-a[0],b[1]-a[1]))
    L=d[-1]; m=max(8,int(round(L/step))); out=[]; j=0
    for k in range(m):
        s=L*k/m
        while d[j+1]<s: j+=1
        f=(s-d[j])/(d[j+1]-d[j]); a=pts[j%n]; b=pts[(j+1)%n]
        out.append((a[0]+(b[0]-a[0])*f, a[1]+(b[1]-a[1])*f))
    return out, L

def smooth(p, it=3):
    n=len(p)
    for _ in range(it):
        p=[((p[i-1][0]+2*p[i][0]+p[(i+1)%n][0])/4,(p[i-1][1]+2*p[i][1]+p[(i+1)%n][1])/4) for i in range(n)]
    return p

def curvature_radius(p):
    n=len(p); R=[]
    for i in range(n):
        a=p[i-1]; b=p[i]; c=p[(i+1)%n]
        ab=math.hypot(b[0]-a[0],b[1]-a[1]); bc=math.hypot(c[0]-b[0],c[1]-b[1]); ca=math.hypot(a[0]-c[0],a[1]-c[1])
        cr=abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))
        R.append(1e9 if cr<1e-6 else ab*bc*ca/(2*cr))
    return R

def build(t):
    pts=centerline(t); pts,L=resample(pts,10.0); pts=smooth(pts,4)
    # rotar para que el punto 0 sea el inicio de la recta mas larga (menor curvatura media en 10 puntos)
    R=curvature_radius(pts); n=len(pts)
    best=max(range(n), key=lambda i: sum(min(R[(i+k)%n],600) for k in range(12)))
    pts=pts[best:]+pts[:best]; R=R[best:]+R[:best]
    # limite de velocidad (m/s): v = sqrt(a_lat * R), suavizado hacia atras (frenada)
    alat=21.0
    v=[min(60.0, math.sqrt(alat*max(r,1.0))) for r in R]
    for _ in range(6):
        for i in range(n-1,-1,-1):
            v[i]=min(v[i], (v[(i+1)%n]**2+2*14.0*10.0)**0.5)
    return pts,R,v

def rect_dist(p,pts,skip=None):
    return min(math.hypot(p[0]-q[0],p[1]-q[1]) for q in pts)

def scenery(t,pts,rng):
    objs=[]; n=len(pts); hw=t["hw"]; th=t["theme"]
    kinds={0:[(0,3),(1,3),(6,1)],1:[(0,8),(1,3),(2,1)],2:[(2,4),(5,3),(7,3)],3:[(3,3),(4,3),(2,3)]}[th]
    pool=[k for k,w in kinds for _ in range(w)]
    tries=0
    while len(objs)<170 and tries<20000:
        tries+=1
        i=rng.randrange(n); a=pts[i]; b=pts[(i+1)%n]
        dx,dz=b[0]-a[0],b[1]-a[1]; l=math.hypot(dx,dz); nx,nz=-dz/l,dx/l
        side=rng.choice((-1,1)); kind=rng.choice(pool)
        sz={0:rng.uniform(5,9),1:rng.uniform(4,7),2:rng.uniform(2,5),3:rng.uniform(3,5),4:rng.uniform(14,24),5:rng.uniform(6,14),6:rng.uniform(14,20),7:rng.uniform(10,22)}[kind]
        clear={0:3,1:3,2:sz+1,3:3,4:sz*0.95+10,5:sz+2,6:sz+4,7:sz*0.6+4}[kind]
        off=hw+clear+rng.uniform(2,60) if rng.random()<0.8 else hw+clear+rng.uniform(60,140)
        x=a[0]+nx*off*side; z=a[1]+nz*off*side
        need=hw+clear-1
        if rect_dist((x,z),pts)<need: continue
        if any(math.hypot(x-o[1],z-o[2])<(9 if kind!=4 else 40) for o in objs): continue
        objs.append((kind,x,z,sz))
    return objs

def emit():
    out=["/* GENERADO por tools/gen_tracks.py - no editar a mano */",
         "#ifndef TRACKS_H\n#define TRACKS_H\n#include <stdint.h>",
         f"#define TRK_UNIT {U}  /* unidades por metro */",
         "typedef struct { int8_t kind; int16_t x, z; uint8_t size; } TrkObj;",
         "typedef struct { const char *name; uint8_t theme, laps, hw; uint16_t reward; uint16_t npts, nobj;",
         "  const int16_t *pts; const uint8_t *vmax; const TrkObj *obj; int length_m; } TrackDef;\n"]
    info=[]
    for t in TRACKS:
        rng=random.Random(t["seed"]); pts,R,v=build(t); n=len(pts)
        objs=scenery(t,pts,rng)
        L=sum(math.hypot(pts[(i+1)%n][0]-pts[i][0],pts[(i+1)%n][1]-pts[i][1]) for i in range(n))
        out.append(f"static const int16_t trk_{t['key']}_pts[{n*2}] = {{")
        out.append(",".join(f"{int(round(p[0]*U))},{int(round(p[1]*U))}" for p in pts)+"};")
        out.append(f"static const uint8_t trk_{t['key']}_v[{n}] = {{"+",".join(str(int(min(255,x))) for x in v)+"};")
        out.append(f"static const TrkObj trk_{t['key']}_obj[{len(objs)}] = {{")
        out.append(",".join(f"{{{k},{int(round(x*U))},{int(round(z*U))},{int(round(s))}}}" for k,x,z,s in objs)+"};")
        info.append((t,n,len(objs),L,min(R),min(v),max(v),pts,objs))
    out.append("static const TrackDef TRACK_DEFS[%d] = {"%len(TRACKS))
    for t,n,no,L,mr,mv,Mv,_,_ in info:
        out.append(f'  {{"{t["name"]}",{t["theme"]},{t["laps"]},{t["hw"]},{t["reward"]},{n},{no},trk_{t["key"]}_pts,trk_{t["key"]}_v,trk_{t["key"]}_obj,{int(L)}}},')
    out.append("};\n#define NTRACKS %d\n#endif"%len(TRACKS))
    return "\n".join(out),info

def selfintersect(p):
    n=len(p)
    def ccw(a,b,c): return (c[1]-a[1])*(b[0]-a[0])>(b[1]-a[1])*(c[0]-a[0])
    def inter(a,b,c,d): return ccw(a,c,d)!=ccw(b,c,d) and ccw(a,b,c)!=ccw(a,b,d)
    for i in range(n):
        for j in range(i+2,n):
            if i==0 and j==n-1: continue
            if inter(p[i],p[(i+1)%n],p[j],p[(j+1)%n]): return True
    return False

if __name__=="__main__":
    txt,info=emit()
    path=sys.argv[1] if len(sys.argv)>1 else "src/tracks.h"
    open(path,"w").write(txt)
    for t,n,no,L,mr,mv,Mv,pts,objs in info:
        print(f"{t['name']:22s} pts={n:3d} largo={L:6.0f} m  radio_min={mr:5.1f} m  vmax={mv:4.1f}..{Mv:4.1f} m/s  obj={no} cruces={selfintersect(pts)}")
