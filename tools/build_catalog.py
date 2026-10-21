"""Normalize behavioral metadata from an owned recovered project.
Exports identifiers, numeric rule facts and source pointers, never scripts or original
localized description text. Existing native IDs stay stable when catalog grows.
"""
import argparse
import csv
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]

def prop(text,name,default=None):
    match=re.search(r'^'+re.escape(name)+r'\s*=\s*(.*)$',text,re.M)
    if not match:return default
    raw=match.group(1).strip()
    if raw.startswith('"'):return json.loads(raw)
    if raw in ('true','false'):return raw=='true'
    if re.fullmatch(r'-?\d+',raw):return int(raw)
    return raw

def symbol(value):
    result=re.sub('[^A-Za-z0-9_]','_',value).upper()
    return ('B_' if result[:1].isdigit() else '')+result

def read_records(original,folder):
    records=[]
    for path in sorted((original/folder).rglob('*.tres')):
        text=path.read_text(encoding='utf-8-sig');key=prop(text,'id','')
        textures=dict(re.findall(r'\[ext_resource type="Texture2D"[^\n]*path="res://([^"\n]+)"[^\n]*id="([^"\n]+)"',text))
        inverse={identifier:source for source,identifier in textures.items()}
        texture=prop(text,'texture','');match=re.search(r'ExtResource\("([^"\n]+)"\)',texture)
        color=prop(text,'main_color','Color(0, 0, 0, 1)')
        channels=[float(v) for v in re.findall(r'-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?',color)]
        if len(channels)!=4:raise ValueError('Invalid color in '+str(path))
        packed=sum(max(0,min(255,round(value*255)))<<(8*i) for i,value in enumerate(channels))
        tags=re.findall(r'"([^"\n]+)"',prop(text,'tags',''))
        records.append(dict(key=key,source=path.relative_to(original).as_posix(),texture=inverse.get(match.group(1),'') if match else '',set=prop(text,'from_set',''),tags=tags,base=prop(text,'base_score',1),start=prop(text,'start_level',1),rarity=prop(text,'rarity',0),drop=prop(text,'can_drop',False),upgrade=prop(text,'upgradeable',True),card=prop(text,'has_card',True),color=packed))
    return records

def build(original):
    from PIL import Image
    bindings=json.loads((ROOT/'tools/native_behaviors.json').read_text())
    signal_names=['SCORE','POCKET-ANOTHER','ROUND-START','ROUND-END','SHOOT','SPAWN','SPAWN-ANOTHER','HIT','HIT-WALL','BUFF','TRANSFORM','TRANSFORM-ANOTHER','SCORE-CHANGE','REACH-SCORE','MAX-ROLL','PICKUP-DROPLET','ENTER-SHOP','UPGRADE-BALL','SELL-ANOTHER','REROLL-SHOP','SELL-SELF','BUY','BUY-ANOTHER','POCKET']
    event_source=(original/'event_manager.gd').read_text(encoding='utf-8-sig')
    event_ids={}
    for constant,event in [('POCKET_EFFECTS','POCKET'),('SPAWN_EFFECTS','SPAWN')]:
        body=re.search(r'const '+constant+r'[^=]*=\s*\{(.*?)\n\}',event_source,re.S).group(1)
        event_ids[event]=re.findall(r'&"([^"]+)"\s*:',body)
    body=re.search(r'const HARDCODED_EVENT_LISTENERS[^=]*=\s*\{(.*?)\n\}',event_source,re.S).group(1)
    for event,values in re.findall(r'&"([^"]+)"\s*:\s*\[(.*?)\]',body,re.S):event_ids[event]=re.findall(r'&"([^"]+)"',values)
    masks={}
    for event,keys in event_ids.items():
        if event not in signal_names:raise ValueError('Unmapped event '+event)
        for key in keys:masks[key]=masks.get(key,0)|(1<<signal_names.index(event))
    all_balls=read_records(original,'data/balls_data');all_snacks=read_records(original,'data/passives_data')
    for row in all_balls+all_snacks:row['events']=masks.get(row['key'],0)
    if len(all_balls)!=226 or len(all_snacks)!=51:raise ValueError('Original version does not match audited 226/51 resource snapshot')
    canonical={}
    for record in all_balls:
        if record['key']:canonical[record['key']]=record
    for kind,known in [('balls',set(canonical)),('snacks',{r['key'] for r in all_snacks})]:
        unknown=set(bindings[kind])-known
        if unknown:raise ValueError('Unknown native behavior keys: '+str(sorted(unknown)))
    ids_path=ROOT/'tools/catalog_ids.json';ids=json.loads(ids_path.read_text())
    for kind,records in [('balls',canonical),('snacks',{r['key']:r for r in all_snacks})]:
        for key in sorted(records):
            if key not in ids[kind]:ids[kind].append(key)
    ids_path.write_text(json.dumps(ids,indent=2)+'\n')
    balls=[canonical[key] for key in ids['balls']]
    snack_map={r['key']:r for r in all_snacks}
    empty=dict(key='',source='',texture='',set='',tags=[],base=1,start=1,rarity=0,drop=False,upgrade=False,card=False,color=0xff000000,events=0)
    snacks=[empty]+[snack_map[key] for key in ids['snacks'][1:]]
    tags=sorted({tag for row in balls+snacks for tag in row['tags']});sets=['']+sorted({row['set'] for row in balls+snacks if row['set']})
    tag_bits={tag:1<<i for i,tag in enumerate(tags)}
    header=['#pragma once','#include <cstdint>','namespace up {',f'constexpr unsigned content_count={len(balls)};',f'constexpr unsigned snack_count={len(snacks)};','enum class ItemSet : uint8_t { '+', '.join(symbol(value) if value else 'NONE' for value in sets)+' };','enum class Tag : uint32_t { '+', '.join(symbol(tag)+'='+str(tag_bits[tag])+'u' for tag in tags)+' };']
    for kind,name in [('balls','BallKey'),('snacks','SnackKey')]:
        symbols=[symbol(key) if key else 'NONE' for key in ids[kind]]
        if len(symbols)!=len(set(symbols)):raise ValueError('Enum symbol collision')
        header += ['enum class '+name+' : uint16_t { '+', '.join(key+'='+str(i) for i,key in enumerate(symbols))+' };','constexpr unsigned id_of('+name+' key) { return static_cast<unsigned>(key); }']
    header += ['struct ContentInfo { const char* key; const char* name; const char* source; uint32_t tags; uint32_t color; int base_score; int start_level; uint8_t rarity; ItemSet set; bool can_drop,upgradeable,has_card; uint16_t ability; bool implemented; uint32_t events; };','const ContentInfo& ball_info(unsigned id);','const ContentInfo& snack_info(unsigned id);','unsigned find_ball(const char* key);','unsigned find_snack(const char* key);','}']
    (ROOT/'include/ultrapool/catalog.hpp').write_text('\n'.join(header)+'\n')
    old_ball_names=['Cue']+[f'{i}-Ball' for i in range(1,9)]+['Pinball','Bricks','Gold','Smiley','Bomb','Seal','Basketball','Tennis','Football','Beach Ball','Dice','Marble','Plushie','Roulette','Pants','Yinyang','Juggling Ball','Overflow Ball','Upgrade Ball','CoolGuy','Spent Smiley','Slot','Nesting Doll','Mirror']
    old_snack_names=['Empty','Olive','Croissant','Chocolate','Cereal','Poker Chip','Strawberry','Blackberry','Ham Sandwich','Crisp Sandwich','Black Olive','Muffin','Mini Muffin']
    source=['#include "ultrapool/core.hpp"','#include <cstring>','namespace up {','namespace {']
    for name,records,old_names in [('balls',balls,old_ball_names),('snacks',snacks,old_snack_names)]:
        source.append('const ContentInfo '+name+'[]={')
        for i,row in enumerate(records):
            display=old_names[i] if i<len(old_names) else row['key'].replace('_',' ').replace('-',' ').title()
            fields=[json.dumps(row['key']),json.dumps(display),json.dumps(row['source']),str(sum(tag_bits[tag] for tag in row['tags']))+'u',hex(row['color'])+'u',str(row['base']),str(row['start']),str(row['rarity']),'ItemSet::'+(symbol(row['set']) if row['set'] else 'NONE')]+[str(row[key]).lower() for key in ['drop','upgrade','card']]
            binding=bindings[name].get(row['key'])
            fields += [binding if name=='balls' and binding else 'Plain' if binding else 'Unsupported',str(bool(binding)).lower()]
            source.append('    {'+','.join(fields+[str(row['events'])+'u'])+'},')
        source.append('};')
    source += ['}','const ContentInfo& ball_info(unsigned id) { return balls[id<content_count?id:0]; }','const ContentInfo& snack_info(unsigned id) { return snacks[id<snack_count?id:0]; }','unsigned find_ball(const char* key) { if(key) for(unsigned i=0;i<content_count;++i) if(std::strcmp(key,balls[i].key)==0) return i; return content_count; }','unsigned find_snack(const char* key) { if(key) for(unsigned i=1;i<snack_count;++i) if(std::strcmp(key,snacks[i].key)==0) return i; return snack_count; }','}']
    ability_masks={}
    for row in balls:
        binding=bindings['balls'].get(row['key'])
        if binding:ability_masks[binding]=ability_masks.get(binding,0)|row['events']
    source.pop()
    native_signals=['Score','PocketAnother','RoundStart','RoundEnd','Shot','Spawn','SpawnAnother','Hit','Wall','Buff','Transform','TransformAnother','ScoreChange','ReachScore','MaxRoll','Pickup','EnterShop','Upgrade','SellAnother','Reroll','SellSelf','Buy','BuyAnother','Pocket']
    for i,signal in enumerate(native_signals):source.append('static_assert(signal_bit(Signal::'+signal+')=='+str(1<<i)+'u, "Regenerate catalog after signal schema change");')
    source.append('uint32_t ability_signals(unsigned ability) {')
    for binding,mask in sorted(ability_masks.items()):source.append('    if(ability=='+binding+') return '+str(mask)+'u;')
    source += ['    if(ability==Respawn) return signal_bit(Signal::Pocket); // Internal unused-source helper, no stock binding.', '    return 0;', '}']
    source.append('uint32_t ability_handlers(unsigned ability) {')
    for key,events in bindings.get('direct_events',{}).items():
        if key not in bindings['balls']:raise ValueError('Direct handler without native binding: '+key)
        mask=sum(1<<signal_names.index(event) for event in set(events))
        source.append('    if(ability=='+bindings['balls'][key]+') return ability_signals(ability)|'+str(mask)+'u;')
    source += ['    return ability_signals(ability);','}','}']
    (ROOT/'src/core/catalog.cpp').write_text('\n'.join(source)+'\n')
    manifest=json.loads((ROOT/'tools/manifest_psp.json').read_text());by_output={row['output']:row for row in manifest}
    for prefix,records in [('ball',balls),('snack',snacks)]:
        for i,row in enumerate(records):
            if not row['texture']:continue
            filename=f'{prefix}_{i}.upt';previous=by_output.get(filename)
            if previous:
                # Cue resource actually selects player3.png; retain its chosen viewing face.
                if prefix=='ball' and i==0:previous['source']=row['texture']
                continue
            with Image.open(original/row['texture']) as image:cross=image.width*3==image.height*4
            by_output[filename]=dict(output=filename,source=row['texture'],size=64,projection='cube_sphere' if prefix=='ball' and cross else 'flat',lighting=prefix=='ball',face='right')
    (ROOT/'tools/manifest_psp.json').write_text(json.dumps(list(by_output.values()),indent=2)+'\n')
    enum_source=re.search(r'enum Ability[^\{]*\{([^}]+)',(ROOT/'include/ultrapool/core.hpp').read_text()).group(1)
    abilities={};value=0
    for part in enum_source.split(','):
        name=part.strip()
        if '=' in name:name,number=name.split('=');value=int(number)
        abilities[name.strip()]=value;value+=1
    definitions=['# Native behavioral definitions: id radius mass damping base_score ABGR ability']
    for i,row in enumerate(balls[1:],1):
        weight=.5 if row['key'] in ('FISH','PLUTO','ANT') else 5 if row['key']=='JUPITER' else 60 if row['key']=='TUNGSTEN-BALL' else 1
        radius=17*(.8 if weight==.5 else 1.2 if weight==5 else 1.3 if weight==60 else 1)
        definitions.append(f"{i} {radius:g} {weight:g} 1.5 {row['base']} {row['color']:08x} {abilities[bindings['balls'].get(row['key'],'Unsupported')]}")
    (ROOT/'assets/definitions.txt').write_text('\n'.join(definitions)+'\n')
    with (ROOT/'docs/CATALOG_RECORDS.csv').open('w',newline='') as stream:
        writer=csv.writer(stream);writer.writerow(['kind','native_id','key','source','can_drop','rarity','base_score','start_level','tags'])
        for kind,records,mapping in [('ball',all_balls,ids['balls']),('passive',all_snacks,ids['snacks'])]:
            for row in records:writer.writerow([kind,mapping.index(row['key']) if row['key'] else '',row['key'],row['source'],row['drop'],row['rarity'],row['base'],row['start'],'|'.join(row['tags'])])
    with (ROOT/'docs/EVENT_REGISTRATIONS.csv').open('w',newline='') as stream:
        writer=csv.writer(stream);writer.writerow(['kind','key','source','event_mask','events'])
        for kind,records in [('ball',all_balls),('passive',all_snacks)]:
            for row in records:writer.writerow([kind,row['key'],row['source'],row['events'],'|'.join(name for i,name in enumerate(signal_names) if row['events']&(1<<i))])
    facts=dict(balls=balls,snacks=snacks,records=all_balls+all_snacks,tags=tag_bits)
    (ROOT/'build/catalog-facts.json').write_text(json.dumps(facts,indent=2)+'\n')
    print(json.dumps({'ball_records':len(all_balls),'unique_ball_ids':len(balls),'passive_records':len(all_snacks),'textures':len(by_output),'tags':len(tags)}))
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--original',type=Path,required=True);build(parser.parse_args().original.resolve())
