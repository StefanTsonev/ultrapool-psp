#include "ultrapool/core.hpp"
#include "ultrapool/presentation.hpp"
#include <new>
#include <cstdio>
#include <cstring>
#include <cstdlib>
namespace up {
static uint16_t u16(const unsigned char* p) { return uint16_t(p[0])|uint16_t(p[1])<<8; }
bool load_achievement_text(const char* filename,std::unique_ptr<AchievementText[]>& out){
 FILE* f=std::fopen(filename,"rb");if(!f)return false;unsigned char header[8];bool ok=std::fread(header,1,8,f)==8 && !std::memcmp(header,"UAT1",4);unsigned count=ok?unsigned(header[4])|(unsigned(header[5])<<8)|(unsigned(header[6])<<16)|(unsigned(header[7])<<24):0;
 if(!ok || count>achievement_count){std::fclose(f);return false;}
 std::unique_ptr<AchievementText[]> next(new(std::nothrow) AchievementText[achievement_count]);if(!next){std::fclose(f);return false;}bool seen[achievement_count]{};
 for(unsigned n=0;n<count && ok;++n){unsigned char row[6];if(std::fread(row,1,6,f)!=6){ok=false;break;}unsigned id=u16(row),title=u16(row+2),description=u16(row+4);if(id>=achievement_count || seen[id] || title>=sizeof(next[id].title) || description>=sizeof(next[id].description)){ok=false;break;}seen[id]=true;ok=std::fread(next[id].title,1,title,f)==title && std::fread(next[id].description,1,description,f)==description;for(unsigned i=0;i<title;++i)ok&=next[id].title[i]>=32 && next[id].title[i]<=126;for(unsigned i=0;i<description;++i)ok&=next[id].description[i]>=32 && next[id].description[i]<=126;}
 ok=ok && std::fgetc(f)==EOF && !std::ferror(f);std::fclose(f);if(ok)out=std::move(next);return ok;
}
const char* PrivateTextBank::title(unsigned id)const{return id<offsets.size() && offsets[id][0]<data.size()?data.data()+offsets[id][0]:"";}
const char* PrivateTextBank::text(unsigned id)const{return id<offsets.size() && offsets[id][1]<data.size()?data.data()+offsets[id][1]:"";}
bool PrivateTextBank::load(const char* filename,unsigned limit){
 FILE* f=std::fopen(filename,"rb");if(!f)return false;
 unsigned char header[8];bool ok=std::fread(header,1,8,f)==8 && !std::memcmp(header,"UTX1",4);
 unsigned count=ok?unsigned(header[4])|(unsigned(header[5])<<8)|(unsigned(header[6])<<16)|(unsigned(header[7])<<24):0;
 if(!ok || count>limit || limit>2048){std::fclose(f);return false;}
 PrivateTextBank next;next.offsets.resize(limit);next.data.push_back(0);
 for(unsigned n=0;n<count && ok;++n){
  unsigned char row[6];if(std::fread(row,1,6,f)!=6){ok=false;break;}
  unsigned id=u16(row),title_len=u16(row+2),text_len=u16(row+4);
  if(id>=limit || next.offsets[id][0] || title_len>255 || text_len>1023 || next.data.size()+title_len+text_len+2>262144){ok=false;break;}
  unsigned sizes[]={title_len,text_len};
  for(unsigned k=0;k<2 && ok;++k){unsigned start=unsigned(next.data.size());next.offsets[id][k]=start;next.data.resize(start+sizes[k]+1,0);
   ok=std::fread(next.data.data()+start,1,sizes[k],f)==sizes[k];
   for(unsigned c=0;c<sizes[k];++c){unsigned char v=next.data[start+c];if(v<32||v>126)ok=false;}
  }
 }
 ok=ok && std::fgetc(f)==EOF && !std::ferror(f);std::fclose(f);if(ok)*this=std::move(next);return ok;
}
Texture::~Texture() { clear(); }
void Texture::clear() { std::free(allocation); allocation=nullptr; pixels=nullptr; width=height=0; }
bool load_texture(const char* filename,Texture& out) {
    FILE* f=std::fopen(filename,"rb"); if(!f) return false;
    unsigned char h[8];
    bool ok=std::fread(h,1,8,f)==8 && std::memcmp(h,"UPT1",4)==0;
    unsigned w=ok?u16(h+4):0, height=ok?u16(h+6):0;
    ok=ok && w>0 && height>0 && w<=512 && height<=512 && !(w&(w-1)) && !(height&(height-1));
    std::size_t bytes=std::size_t(w)*height*2;
    void* memory=ok?std::malloc(bytes+15):nullptr;
    if(!memory) { std::fclose(f); return false; }
    uint16_t* pixels=reinterpret_cast<uint16_t*>((reinterpret_cast<uintptr_t>(memory)+15)&~uintptr_t(15));
    ok=std::fread(pixels,1,bytes,f)==bytes && std::fgetc(f)==EOF && !std::ferror(f);
    std::fclose(f);
    if(!ok) { std::free(memory); return false; }
    unsigned char* raw=reinterpret_cast<unsigned char*>(pixels);
    for(std::size_t i=0;i<bytes/2;++i) pixels[i]=u16(raw+i*2);
    out.clear(); out.width=static_cast<uint16_t>(w); out.height=static_cast<uint16_t>(height);
    out.allocation=memory; out.pixels=pixels; return true;
}
bool load_sprite(const char* filename,Asset& out) {
    FILE* f=std::fopen(filename,"rb"); if (!f) return false;
    unsigned char h[8];
    bool ok=std::fread(h,1,8,f)==8 && std::memcmp(h,"UPS1",4)==0;
    unsigned w=ok?u16(h+4):0, height=ok?u16(h+6):0;
    ok=ok && w>0 && height>0 && w<=64 && height<=64;
    Asset tmp;
    if (ok) {
        tmp.width=static_cast<uint16_t>(w); tmp.height=static_cast<uint16_t>(height);
        for (unsigned i=0;i<w*height;++i) {
            unsigned char p[2]; if (std::fread(p,1,2,f)!=2) { ok=false; break; }
            tmp.pixels[i]=u16(p);
        }
        if (std::fgetc(f)!=EOF) ok=false;
    }
    std::fclose(f); if (ok) out=tmp; return ok;
}
bool load_definitions(const char* filename,std::array<Definition,226>& out,std::size_t& count) {
    FILE* f=std::fopen(filename,"r"); if (!f) return false;
    std::array<Definition,226> tmp; std::size_t n=0; bool ok=true;
    char line[256];
    while (std::fgets(line,sizeof(line),f)) {
        if (line[0]=='#' || line[0]=='\n') continue;
        unsigned id,ability,color; float r,m,d; int score; char extra;
        int fields=std::sscanf(line,"%u %f %f %f %d %x %u %c",&id,&r,&m,&d,&score,&color,&ability,&extra);
        if (fields!=7 || n==tmp.size() || id>=content_count || (ability>=AbilityCount && ability!=Unsupported) ||
            !std::isfinite(r) || !std::isfinite(m) || !std::isfinite(d) ||
            r<=0 || r>100 || m<=0 || m>10000 || d<0 || d>100 || score< -100000000 || score>100000000) { ok=false; break; }
        for (std::size_t i=0;i<n;++i) if (tmp[i].id==id) ok=false;
        if (!ok) break;
        Definition def; def.id=static_cast<uint16_t>(id); def.radius=r; def.mass=m; def.damping=d;
        def.base_score=score; def.color=color; def.ability=static_cast<uint16_t>(ability); tmp[n++]=def;
    }
    if (std::ferror(f)) ok=false;
    std::fclose(f);
    if (ok && n>0) { out=tmp; count=n; return true; }
    return false;
}
}