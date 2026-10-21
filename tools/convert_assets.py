"""Convert manifest-selected owned images to bounded native textures; never bundles source assets."""
import argparse
import json
import math
from pathlib import Path
import struct

def owned_path(root, relative):
    path=(root/relative).resolve()
    if not path.is_relative_to(root):
        raise ValueError("source escapes installation directory")
    return path

def cube_uv(direction):
    # Standard major-axis cube projection into the recovered 4x3 cross layout.
    x,y,z=direction
    axis=max(range(3),key=lambda i: abs(direction[i]))
    major=abs(direction[axis])
    if axis==0:
        u,v=(-z if x>0 else z),-y
        col,row=(2 if x>0 else 0),1
    elif axis==1:
        u,v=x,(z if y>0 else -z)
        col,row=1,(0 if y>0 else 2)
    else:
        u,v=(x if z>0 else -x),-y
        col,row=(1 if z>0 else 3),1
    return ((col+(u/major+1)*0.5)/4,(row+(v/major+1)*0.5)/3)

def bake_sphere(texture,size,projection,face="right",lighting=True):
    from PIL import Image
    sample_size=size*2
    out=Image.new("RGBA",(sample_size,sample_size))
    for py in range(sample_size):
        for px in range(sample_size):
            x=2*(px+0.5)/sample_size-1
            y=2*(py+0.5)/sample_size-1
            radial=x*x+y*y
            if radial>=1:
                continue
            z=math.sqrt(1-radial)
            if projection=="cube_sphere":
                direction=(z,x,-y) if face=="right" else (x,-y,z)
                u,v=cube_uv(direction)
            else:
                u=0.5+math.atan2(x,z)/(2*math.pi)
                v=0.5+math.asin(y)/math.pi
            sx=min(texture.width-1,max(0,int(u*texture.width)))
            sy=min(texture.height-1,max(0,int(v*texture.height)))
            r,g,b,a=texture.getpixel((sx,sy))
            if lighting:
                light=0.68+0.32*z
                highlight=0.12*math.exp(-((x+0.35)**2+(y+0.4)**2)*25)
                r,g,b=(min(255,round(channel*light+255*highlight)) for channel in (r,g,b))
            out.putpixel((px,py),(r,g,b,a))
    return out.resize((size,size),Image.Resampling.LANCZOS)

def render_entry(source,entry,legacy=False):
    from PIL import Image
    dims=entry.get("size",32)
    if isinstance(dims,int):
        dims=[dims,dims]
    limit=64 if legacy else 512
    if not isinstance(dims,list) or len(dims)!=2 or any(type(n) is not int or not 1<=n<=limit for n in dims):
        raise ValueError("invalid dimensions")
    if not legacy and any(n&(n-1) for n in dims):
        raise ValueError("native texture dimensions must be powers of two")
    projection=entry.get("projection","flat")
    path=owned_path(source,entry["source"])
    if projection=="font":
        if dims!=[256,128]:raise ValueError("font atlas must be 256 by 128")
        from PIL import ImageDraw,ImageFont
        font=ImageFont.truetype(str(path),16)
        image=Image.new("RGBA",(256,128))
        for slot in range(96):
            char=chr(slot+32) if slot<95 else "?"
            if char=="$":char="\u20ac"
            cell=Image.new("RGBA",(16,20));draw=ImageDraw.Draw(cell)
            draw.text((1,1),char,font=font,fill=(255,255,255,255))
            image.alpha_composite(cell,((slot%16)*16,(slot//16)*20))
            advance=max(1,min(15,round(font.getlength(char))))
            image.putpixel((slot,127),(advance*16,0,0,0))
        return image
    with Image.open(path) as original:
        image=original.convert("RGBA")
    if projection in ("sphere","cube_sphere"):
        if dims[0]!=dims[1]:
            raise ValueError("sphere dimensions must be square")
        if projection=="cube_sphere" and image.width*3!=image.height*4:
            raise ValueError("cube map must have 4:3 dimensions")
        image=bake_sphere(image,dims[0],projection,entry.get("face","right"),entry.get("lighting",projection=="cube_sphere"))
    elif projection=="table":
        with Image.open(owned_path(source,entry["overlay"])) as overlay_source:
            overlay=overlay_source.convert("RGBA")
        canvas=Image.new("RGBA",overlay.size)
        offset=((overlay.width-image.width)//2,(overlay.height-image.height)//2)
        canvas.alpha_composite(image,offset)
        canvas.alpha_composite(overlay)
        image=canvas.transpose(Image.Transpose.ROTATE_270).resize(tuple(dims),Image.Resampling.LANCZOS)
    elif projection=="layers":
        from PIL import ImageChops
        canvas=Image.new("RGBA",image.size)
        for layer_entry in entry["layers"]:
            with Image.open(owned_path(source,layer_entry["source"])) as layer_source:
                layer=layer_source.convert("RGBA").resize(canvas.size,Image.Resampling.LANCZOS)
            tint=tuple(layer_entry.get("tint",[255,255,255,255]))
            layer=ImageChops.multiply(layer,Image.new("RGBA",layer.size,tint))
            canvas.alpha_composite(layer)
        if entry.get("trim") and canvas.getchannel("A").getbbox():canvas=canvas.crop(canvas.getchannel("A").getbbox())
        image=canvas.resize(tuple(dims),Image.Resampling.LANCZOS)
    elif projection=="flat":
        if entry.get("logo_mask"):
            from PIL import ImageOps,ImageChops
            mask=ImageOps.grayscale(image).point(lambda value:max(0,min(255,(value-165)*255//90)))
            mask=ImageChops.multiply(mask,image.getchannel("A"))
            image=Image.new("RGBA",image.size,(205,231,147,0));image.putalpha(mask)
        if entry.get("trim") and image.getchannel("A").getbbox():image=image.crop(image.getchannel("A").getbbox())
        if legacy:
            image.thumbnail(tuple(dims),Image.Resampling.LANCZOS)
        else:
            image=image.resize(tuple(dims),Image.Resampling.LANCZOS)
    else:
        raise ValueError("unknown projection")
    return image

def convert(source,output,manifest):
    source=source.resolve()
    records=json.loads(manifest.read_text(encoding="utf-8"))
    if not isinstance(records,list):
        raise ValueError("manifest must be a list")
    destinations=set()
    # Validate the complete manifest before creating outputs.
    for entry in records:
        if not isinstance(entry,dict):
            raise ValueError("entry must be an object")
        name=entry["output"]
        if not isinstance(name,str) or Path(name).name!=name or not name.endswith((".ups",".upt")) or name in destinations:
            raise ValueError("outputs must be unique flat .ups/.upt filenames")
        destinations.add(name)
        owned_path(source,entry["source"])
        if "overlay" in entry:
            owned_path(source,entry["overlay"])
        if entry.get("projection")=="layers":
            layers=entry.get("layers")
            if not isinstance(layers,list) or not layers:raise ValueError("layers must be a nonempty list")
            for layer in layers:
                if not isinstance(layer,dict):raise ValueError("layer must be an object")
                owned_path(source,layer["source"])
                tint=layer.get("tint",[255,255,255,255])
                if not isinstance(tint,list) or len(tint)!=4 or any(not isinstance(v,int) or v<0 or v>255 for v in tint):raise ValueError("layer tint must contain four bytes")
        if (output/name).exists():
            raise FileExistsError(output/name)
    output.mkdir(parents=True,exist_ok=True)
    report=[]
    for entry in records:
        name=entry["output"]
        legacy=name.endswith(".ups")
        image=render_entry(source,entry,legacy)
        data=bytearray(struct.pack("<4sHH",b"UPS1" if legacy else b"UPT1",image.width,image.height))
        for r,g,b,a in getattr(image,"get_flattened_data",image.getdata)():
            data+=struct.pack("<H",(r>>4)|((g>>4)<<4)|((b>>4)<<8)|((a>>4)<<12))
        with (output/name).open("xb") as f:
            f.write(data)
        report.append({"file":name,"width":image.width,"height":image.height,"resident_bytes":image.width*image.height*2})
        print(f"{name}: {image.width}x{image.height}, {len(data)} bytes")
    (output/"conversion-report.json").write_text(json.dumps({"textures":report,"resident_bytes":sum(r["resident_bytes"] for r in report)},indent=2))

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--source",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--manifest",type=Path,required=True)
    args=parser.parse_args()
    convert(args.source,args.output,args.manifest)

if __name__=="__main__":
    main()