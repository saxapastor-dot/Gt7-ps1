import sys,importlib.util
from PIL import Image,ImageDraw
spec=importlib.util.spec_from_file_location("g","tools/gen_tracks.py"); g=importlib.util.module_from_spec(spec); spec.loader.exec_module(g)
txt,info=g.emit()
W=420; im=Image.new("RGB",(W*2,W*2),(20,20,24)); d=ImageDraw.Draw(im)
for idx,(t,n,no,L,mr,mv,Mv,pts,objs) in enumerate(info):
    ox=(idx%2)*W; oy=(idx//2)*W
    xs=[p[0] for p in pts]; zs=[p[1] for p in pts]
    sc=(W-60)/max(max(xs)-min(xs),max(zs)-min(zs)); cx=(max(xs)+min(xs))/2; cz=(max(zs)+min(zs))/2
    f=lambda p:(ox+W/2+(p[0]-cx)*sc, oy+W/2+(p[1]-cz)*sc)
    v=g.build(t)[2]
    for i in range(n):
        a=pts[i]; b=pts[(i+1)%n]; k=int(255*min(1,v[i]/45))
        d.line([f(a),f(b)],fill=(255-k,k,60),width=3)
    for k,x,z,s in objs: 
        p=f((x,z)); d.point(p,fill=(120,120,120))
    p=f(pts[0]); d.ellipse([p[0]-4,p[1]-4,p[0]+4,p[1]+4],fill=(255,255,255))
    d.text((ox+8,oy+6),f"{t['name']}  {L:.0f}m  Rmin {mr:.0f}m",fill=(255,255,255))
im.save("/tmp/claude-0/-home-claude/ef47cb91-9778-5281-a4db-b6633cc74132/scratchpad/tracks.png")
