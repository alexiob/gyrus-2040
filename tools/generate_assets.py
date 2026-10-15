#!/usr/bin/env python3
"""Generate Gyrus's original indexed sprites and review PNGs, using only stdlib."""
from pathlib import Path
import argparse
import json
import math
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
# RGB888 authoring palette; C output is RGB565, unused entries are black.
PALETTE = [
    ('BLACK', '000000'), ('WHITE', 'ffffff'), ('IVORY', 'fff3cb'),
    ('CYAN', '5ce8f2'), ('BLUE', '3284ed'), ('DEEP_BLUE', '17347c'),
    ('AMBER', 'ffbd48'), ('ORANGE', 'f77728'), ('RED', 'e83b3b'),
    ('MAGENTA', 'db46ca'), ('PINK', 'ff96cd'), ('VIOLET', '8c66eb'),
    ('GREEN', '41c28a'), ('LIME', 'a3e77b'), ('TEAL', '238d9c'),
    ('STEEL', '809bab'), ('DARK_STEEL', '364959'), ('BROWN', '805135'),
    ('SAND', 'dca76e'), ('YELLOW', 'ffe763'), ('PALE_BLUE', 'bcefff'),
    ('DARK_RED', '792937'), ('DARK_GREEN', '1c5d5a'), ('PURPLE', '462b76'),
]
COLORS = [tuple(bytes.fromhex(h)) for _, h in PALETTE]
# Single-character, deliberately authored pixel art. Dot is transparency.
# Explicit keys keep sprite matrices easy to edit without palette-index mistakes.
INK = {'.':0,'W':1,'I':2,'C':3,'B':4,'D':5,'A':6,'O':7,'R':8,'M':9,'P':10,
       'V':11,'G':12,'L':13,'T':14,'S':15,'K':16,'H':17,'N':18,'Y':19,'U':20,
       'E':21,'F':22,'Q':23}

FONT = {
'A':['01110','10001','10001','11111','10001','10001','10001'],
'B':['11110','10001','10001','11110','10001','10001','11110'],
'C':['01111','10000','10000','10000','10000','10000','01111'],
'D':['11110','10001','10001','10001','10001','10001','11110'],
'E':['11111','10000','10000','11110','10000','10000','11111'],
'F':['11111','10000','10000','11110','10000','10000','10000'],
'G':['01111','10000','10000','10111','10001','10001','01111'],
'H':['10001','10001','10001','11111','10001','10001','10001'],
'I':['11111','00100','00100','00100','00100','00100','11111'],
'J':['00111','00010','00010','00010','10010','10010','01100'],
'K':['10001','10010','10100','11000','10100','10010','10001'],
'L':['10000','10000','10000','10000','10000','10000','11111'],
'M':['10001','11011','10101','10101','10001','10001','10001'],
'N':['10001','11001','11001','10101','10011','10011','10001'],
'O':['01110','10001','10001','10001','10001','10001','01110'],
'P':['11110','10001','10001','11110','10000','10000','10000'],
'Q':['01110','10001','10001','10001','10101','10010','01101'],
'R':['11110','10001','10001','11110','10100','10010','10001'],
'S':['01111','10000','10000','01110','00001','00001','11110'],
'T':['11111','00100','00100','00100','00100','00100','00100'],
'U':['10001','10001','10001','10001','10001','10001','01110'],
'V':['10001','10001','10001','10001','10001','01010','00100'],
'W':['10001','10001','10001','10101','10101','10101','01010'],
'X':['10001','10001','01010','00100','01010','10001','10001'],
'Y':['10001','10001','01010','00100','00100','00100','00100'],
'Z':['11111','00001','00010','00100','01000','10000','11111'],
'0':['01110','10001','10011','10101','11001','10001','01110'],
'1':['00100','01100','00100','00100','00100','00100','01110'],
'2':['01110','10001','00001','00010','00100','01000','11111'],
'3':['11110','00001','00001','01110','00001','00001','11110'],
'4':['00010','00110','01010','10010','11111','00010','00010'],
'5':['11111','10000','10000','11110','00001','00001','11110'],
'6':['01110','10000','10000','11110','10001','10001','01110'],
'7':['11111','00001','00010','00100','01000','01000','01000'],
'8':['01110','10001','10001','01110','10001','10001','01110'],
'9':['01110','10001','10001','01111','00001','00001','01110'],
' ':['00000']*7, ':':['00000','00100','00100','00000','00100','00100','00000'],
'.':['00000']*5+['00100','00100'], ',':['00000']*4+['00100','00100','01000'],
'-':['00000']*3+['11111']+['00000']*3, '+':['00000','00100','00100','11111','00100','00100','00000'],
'/':['00001','00001','00010','00100','01000','10000','10000'],
'!':['00100']*5+['00000','00100'], '?':['01110','10001','00001','00010','00100','00000','00100'],
'=':['00000','00000','11111','00000','11111','00000','00000'],
'<':['00010','00100','01000','10000','01000','00100','00010'],
'>':['01000','00100','00010','00001','00010','00100','01000'],
'(' : ['00010','00100','01000','01000','01000','00100','00010'],
')' : ['01000','00100','00010','00010','00010','00100','01000'],
'%':['11001','11010','00010','00100','01000','01011','10011'],
'*':['00000','10101','01110','11111','01110','10101','00000'],
"'":['00100','00100','00000','00000','00000','00000','00000'],
'_' : ['00000']*6+['11111'],
}


def canvas(w, h=None):
    return [[0] * w for _ in range(h if h is not None else w)]


def art(lines, size=None):
    width = max(map(len, lines))
    rows = [[INK[c] for c in line.ljust(width, '.')] for line in lines]
    if size is None:
        return rows
    dst = canvas(size)
    xoff, yoff = (size-width)//2, (size-len(rows))//2
    for y,row in enumerate(rows):
        for x,pixel in enumerate(row):
            dst[y+yoff][x+xoff] = pixel
    return dst


def rotated(src, angle):
    h,w = len(src), len(src[0])
    dst = canvas(w,h)
    cx,cy = (w-1)/2,(h-1)/2
    c,s = math.cos(angle), math.sin(angle)
    for y in range(h):
        for x in range(w):
            sx = int(math.floor(c*(x-cx)+s*(y-cy)+cx+0.5))
            sy = int(math.floor(-s*(x-cx)+c*(y-cy)+cy+0.5))
            if 0 <= sx < w and 0 <= sy < h:
                dst[y][x] = src[sy][sx]
    return dst


def headings(src, count):
    return [rotated(src, i*math.tau/count) for i in range(count)]


def line(img,x0,y0,x1,y1,color):
    steps=max(abs(x1-x0),abs(y1-y0),1)
    for i in range(steps+1):
        x=round(x0+(x1-x0)*i/steps); y=round(y0+(y1-y0)*i/steps)
        if 0<=y<len(img) and 0<=x<len(img[0]): img[y][x]=color


def planet(kind):
    size = 29 if kind == 'SATURN' else 23
    p = canvas(size, 23)
    cx, cy, radius = (size-1)/2,11,9 if kind == 'SATURN' else 10
    for y in range(23):
        for x in range(size):
            xx,yy=(x-cx)/radius,(y-cy)/radius
            dist=xx*xx+yy*yy
            if dist>1: continue
            light = -xx*0.45-yy*0.4+math.sqrt(1-dist)*0.7
            noise=math.sin(x*1.77+y*.9)+math.sin(x*.62-y*1.6)
            if kind=='EARTH':
                continent = math.sin(xx*4+yy*2)+math.sin(yy*6-xx*2)+math.sin(xx*9+yy*4)*.4 > .95
                col=(13 if light>.8 else 12 if light>.3 else 22) if continent else (3 if light>.85 else 4 if light>.2 else 5)
                if abs(yy)>.83: col=20 if light>.2 else 4
                if light>.5 and math.sin(x*.8+y*1.8)> .94: col=20
            elif kind=='MARS':
                col=6 if light>.9 else 7 if light>.3 else 21
                if noise>1: col=18 if light>.4 else 17
                if yy<-.79 and xx<.4: col=2
            elif kind in ('JUPITER','SATURN'):
                band=int((y+math.sin(xx*3)*.7)/2.3)%5
                col=([2,18,17,18,6][band] if kind=='JUPITER' else [2,18,6,18,17][band])
                if kind=='JUPITER' and ((xx-.35)/.3)**2+((yy-.3)/.14)**2<1: col=7
                if light<.25: col=17
            elif kind=='URANUS': col=20 if light>.85 else 3 if light>.4 else 14 if light>0 else 5
            else:
                col=3 if light>.9 else 4 if light>.1 else 5
                if .2<yy<.4 and -.5<xx<.3: col=5
                if -.4<yy<-.3 and light>.6: col=20
            p[y][x]=col
    if kind=='SATURN':
        # A tipped ring with rear arc masked by the planet body.
        for j in range(360):
            a=j*math.tau/360
            x=round(cx+13*math.cos(a)); y=round(cy+4*math.sin(a)-3*math.cos(a))
            front=math.sin(a)>0
            if p[y][x]==0 or front: p[y][x]=18 if front else 17
            if front and y+1<23 and p[y+1][x]==0: p[y+1][x]=6
    return p


def build():
    sprites=[]
    def add(name,frames): sprites.append((name,frames))
    player=art([
        '.....I.....', '....IWI....', '....IWI....', '....ICI....',
        '...IICI I...'.replace(' ',''), '...ICBCI...', '..IIIBIII..',
        '.CIIRIRIIC.', 'CIIRIIIRIIC', 'CIRIIIIIRIC', 'CBIIIIIIIBC',
        'C..ICCCI..C', '...O...O...', '...Y...Y...', '...A...A...',
    ],17)
    add('PLAYER', headings(player,32))
    scout=art(['.....A.....','....AIA....','....ROR....','R...ROR...R','RO..RAR..OR',
               '.RORRRRROR.','..RRORORR..','...RO.OR...','...R...R...','...A...A...'],15)
    add('SCOUT', headings(scout,16))
    fighter=art(['.....I.....','....AYA....','....AOA....','...AAOAA...','..AAAOAAA..',
                 '.AOAAOAAOA.','AORAAAAAROA','A..AOAOA..A','....O.O....','....R.R....'],15)
    add('FIGHTER',headings(fighter,16))
    moth=art(['..P.....P..','.PM.....MP.','PMQ..I..QMP','MQQ.MWM.QQM',
              '.QMMPIPMMQ.','..MMQMQMM..','.MM..M..MM.','PM...M...MP','P....V....P'],15)
    add('MOTH',headings(moth,16))
    satellite=art(['....CC.....','...CUUC....','...CBBC....','....TC.....','....TT.....',
                   '..CCTTC....','.CUUBCUC...','.CBBCBBC...','..CC.CC....'],15)
    # Cyan orbit pods change their accent every two frames.
    satellites=[]
    for i in range(8):
        frame=rotated(satellite,i*math.tau/8)
        if i%2: frame=[[20 if v==3 else 3 if v==20 else v for v in row] for row in frame]
        satellites.append(frame)
    add('SATELLITE',satellites)
    asteroid=art(['....AAA....','..AAOOOAA..','.AOONNOOOA.','AONHHNOOOOA','AOHKHHOOHNA',
                  'AOHHNOOHKHA','AOOONOOHHNA','.AOOOHNOOA.','..ANNNAAA..','....AAA....'],15)
    add('ASTEROID',headings(asteroid,8))
    emitter=art(['.....C.....','....CWC....','...CTBTC...','..CTBBBTC..','.CCTBBBTCC.',
                 'CWCCTBTCCWC','.CCCTBTCCC.','...CTBTC...','....CCC....','.....C.....'],15)
    emitters=[]
    for i in range(8):
        frame=rotated(emitter,i*math.tau/8)
        if i>=4: frame=[[19 if v==3 else 7 if v==4 else v for v in row] for row in frame]
        emitters.append(frame)
    add('LASER_EMITTER',emitters)
    explosions=[]
    for frame in range(6):
        p=canvas(21)
        for y in range(21):
            for x in range(21):
                xx,yy=x-10,y-10
                d=math.hypot(xx,yy)
                angle=math.atan2(yy,xx)
                edge=(2+frame*1.4)+(math.sin(angle*7+frame)*.9)
                if frame<3 and d<edge: p[y][x]=1 if d<edge*.28 else 19 if d<edge*.55 else 7 if d<edge*.8 else 8
                elif frame>=3 and edge-2.2<d<edge and (x+y+frame)%3!=0: p[y][x]=6 if d<edge-1 else 8 if frame<5 else 21
        for j in range(8):
            a=j*math.tau/8+.2
            r=min(9.5,3+frame*1.6)
            x,y=round(10+math.cos(a)*r),round(10+math.sin(a)*r)
            p[y][x]=1 if frame<2 else 19 if frame<4 else 7
        explosions.append(p)
    add('EXPLOSION',explosions)
    for kind in ['EARTH','MARS','JUPITER','SATURN','URANUS','NEPTUNE']: add('PLANET_'+kind,[planet(kind)])
    add('LIFE',[art(['...I...','...I...','..ICI..','.IIRII.','CIIIIIC','C.O.O.C'],9)])
    add('PLAYER_BULLET',[art(['C','W','W','C'])])
    add('ENEMY_BULLET',[art(['.A.','AWA','.A.'])])
    add('POWERUP',[art(['...CC...','..CWWC..','.CWCCC..','CWCCWCC.','CWCCWCC.',
                       '.CWCCC..','..CWWC..','...CC...'],11)])
    return sprites


def chunk(kind,payload):
    return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)


def png(path,pixels,scale=1):
    h,w=len(pixels),len(pixels[0])
    rows=[]
    for row in pixels:
        rgb=b''.join(bytes(COLORS[p])*scale for p in row)
        rows.extend([b'\0'+rgb]*scale)
    data=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w*scale,h*scale,8,2,0,0,0))
    data+=chunk(b'IDAT',zlib.compress(b''.join(rows),9))+chunk(b'IEND',b'')
    path.write_bytes(data)


def blit(dst,src,x,y):
    for sy,row in enumerate(src):
        for sx,v in enumerate(row):
            if v and 0<=y+sy<len(dst) and 0<=x+sx<len(dst[0]): dst[y+sy][x+sx]=v


def text(dst,word,x,y,color=2,scale=1):
    for char in word.upper():
        for yy,row in enumerate(FONT.get(char,FONT[' '])):
            for xx,p in enumerate(row):
                if p=='1':
                    for dy in range(scale):
                        for dx in range(scale):
                            tx,ty=x+xx*scale+dx,y+yy*scale+dy
                            if 0<=ty<len(dst) and 0<=tx<len(dst[0]): dst[ty][tx]=color
        x+=6*scale


def contact(sprites):
    sheet=canvas(620,70+len(sprites)*60)
    text(sheet,'GYRUS / ORIGINAL PIXEL ASSETS',10,10,20,2)
    for i,(name,frames) in enumerate(sprites):
        y=60+i*60
        text(sheet,name,10,y,2)
        text(sheet,str(len(frames))+' FR',160,y,15)
        chosen=list(range(min(len(frames),16)))
        for j,k in enumerate(chosen): blit(sheet,frames[k],10+j*36,y+15)
    return sheet


def preview(sprites,title=False):
    frames={name:f for name,f in sprites}
    p=canvas(240)
    # Deterministic sparse stars; much darker than active sprites.
    for i in range(75):
        x=(i*67+31)%240; y=(i*i*13+19)%240
        p[y][x]=[16,16,5,15,14][i%5]
    if title:
        text(p,'GYRUS',33,37,16,5)
        text(p,'GYRUS',30,34,20,5)
        text(p,'2040',86,82,6,3)
        blit(p,frames['PLANET_EARTH'][0],109,117)
        text(p,'ORBITAL DEFENCE',78,149,3)
        text(p,'PRESS FIRE',90,177,2)
        text(p,'RP2040  /  NATIVE C',66,214,15)
        return p
    text(p,'1UP',8,5,6); text(p,'HI',102,5,8);text(p,'STAGE 3',187,5,2)
    text(p,'024800',8,15,2);text(p,'100000',90,15,2)
    for k in range(3): blit(p,frames['LIFE'][0],8+k*12,220)
    text(p,'MARS',197,222,15)
    blit(p,frames['PLANET_MARS'][0],109,107)
    positions=[(78,72,2),(98,59,3),(122,51,4),(148,58,5),(169,75,6),(173,99,7)]
    for i,(x,y,h) in enumerate(positions): blit(p,frames['SCOUT' if i%2 else 'FIGHTER'][h],x,y)
    for i,(x,y) in enumerate([(62,125),(77,147),(98,161)]): blit(p,frames['MOTH'][8+i],x,y)
    blit(p,frames['SATELLITE'][1],168,145);blit(p,frames['ASTEROID'][2],45,92)
    blit(p,frames['LASER_EMITTER'][2],141,95)
    blit(p,frames['PLAYER'][19],66,188)
    blit(p,frames['PLAYER_BULLET'][0],91,175)
    blit(p,frames['PLAYER_BULLET'][0],104,153)
    blit(p,frames['ENEMY_BULLET'][0],120,178)
    blit(p,frames['ENEMY_BULLET'][0],151,159)
    return p


def emit(sprites, out):
    out.mkdir(parents=True,exist_ok=True)
    c=['/* Generated by tools/generate_assets.py. Original authored assets. */',
       '#include "gyrus/assets.h"','', 'const uint16_t gyrus_palette[256] = {']
    rgb565=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in COLORS]
    c.append('    '+', '.join('0x%04x'%v for v in rgb565)+',');c+=['};','']
    pixels_total=0; report=[]
    for name,frames in sprites:
        w,h=len(frames[0][0]),len(frames[0])
        for k,frame in enumerate(frames):
            assert len(frame)==h and all(len(row)==w for row in frame)
            assert all(0<=p<len(PALETTE) for row in frame for p in row)
            c.append(f'static const uint8_t pixels_{name.lower()}_{k}[{w*h}] = {{')
            c.extend('    '+','.join(str(v) for v in row)+',' for row in frame)
            c+=['};']
        c.append(f'static const gyrus_asset frames_{name.lower()}[{len(frames)}] = {{')
        c.extend(f'    {{{w}, {h}, pixels_{name.lower()}_{k}}},' for k in range(len(frames)))
        c+=['};','']
        cost=w*h*len(frames);pixels_total+=cost
        report.append(dict(id=name,width=w,height=h,frames=len(frames),pixel_bytes=cost))
        strip=canvas(w*len(frames),h)
        for k,f in enumerate(frames):blit(strip,f,k*w,0)
        png(out/(name.lower()+'.png'),strip)
    c+=['static const struct { const gyrus_asset *frames; uint8_t count; } sprite_table[] = {']
    c.extend(f'    {{frames_{name.lower()}, {len(frames)}}},' for name,frames in sprites)
    c+=['};','','const gyrus_asset *gyrus_sprite(gyrus_sprite_id id, unsigned frame_or_heading) {',
        '    if ((unsigned)id >= GYRUS_SPRITE_COUNT) return 0;',
        '    return &sprite_table[id].frames[frame_or_heading % sprite_table[id].count];','}',
        'unsigned gyrus_sprite_frame_count(gyrus_sprite_id id) {',
        '    return (unsigned)id < GYRUS_SPRITE_COUNT ? sprite_table[id].count : 0;','}','',
        'static const uint8_t font[96][7] = {']
    for code in range(32,128):
        glyph=FONT.get(chr(code).upper(),FONT[' '])
        c.append('    {'+','.join(str(int(row,2)) for row in glyph)+'}, /* ASCII '+str(code)+' */')
    c+=['};','','const uint8_t *gyrus_glyph(char character) {',
        '    unsigned code = (unsigned char)character;',
        '    return font[code >= 32 && code < 128 ? code - 32 : 0];','}','']
    (ROOT/'src/assets.c').write_text('\n'.join(c))
    png(out/'contact-sheet.png',contact(sprites),2)
    png(out/'preview-gameplay.png',preview(sprites))
    png(out/'preview-gameplay-4x.png',preview(sprites),4)
    png(out/'preview-title.png',preview(sprites,True))
    png(out/'preview-title-4x.png',preview(sprites,True),4)
    meta=dict(palette_entries=len(PALETTE),palette_bytes=512,font_bytes=96*7,
              sprite_pixel_bytes=pixels_total,frame_descriptors=sum(len(f) for _,f in sprites),
              sprites=report,notes='Const data in flash; no decompression or runtime rotation; ordinary RGB565 palette.')
    (out/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(json.dumps(meta,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'assets')
    args=parser.parse_args()
    emit(build(),args.output)
