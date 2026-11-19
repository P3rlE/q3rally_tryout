"""Build the native console's fixed-cell font atlas and ioquake3 font metadata."""
import argparse, shutil, struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

def build(source, target):
    target.mkdir(parents=True, exist_ok=True)
    font=ImageFont.truetype(str(source/'ShareTechMono-Regular.ttf'),18)
    atlas=Image.new('RGBA',(512,512),(255,255,255,0)); draw=ImageDraw.Draw(atlas)
    data=bytearray(); shader=b'fonts/q3r_console'
    for code in range(256):
        char=chr(code); left,top,right,bottom=font.getbbox(char,anchor='ls')
        width=max(1,right-left);height=max(1,bottom-top)
        x=(code%16)*32+2;y=(code//16)*32+2
        if code>=32:draw.text((x-left,y-top),char,font=font,anchor='ls',fill='white')
        data+=struct.pack('<7i4fi64s',height,-top,bottom,0,10,width,height,x/512,y/512,(x+width)/512,(y+height)/512,0,shader)
    data+=struct.pack('<f64s',48/17,b'fonts/fontImage_17.dat')
    assert len(data)==28740
    atlas.save(target/'q3r_console.png')
    (target/'fontImage_17.dat').write_bytes(data)
    shutil.copyfile(source/'ShareTechMono-OFL.txt',target/'ShareTechMono-OFL.txt')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('source',type=Path);parser.add_argument('target',type=Path);args=parser.parse_args();build(args.source,args.target)
