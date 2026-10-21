#include "ultrapool/presentation.hpp"
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <cstdio>
namespace up {
static unsigned draw_offset=0,draw_us=0;
static unsigned int list[65536] __attribute__((aligned(16)));
static Skin skin;
struct Vertex { unsigned color; float x,y,z; };
struct TexVertex { float u,v; unsigned color; float x,y,z; };
class GuPainter : public Painter {
    struct TextRun { int x,y; uint32_t color; char value[96]; };
    std::array<TextRun,128> labels;
    unsigned count=0;
    const Texture* bound=nullptr;
public:
    void begin() { count=0; bound=nullptr; }
    void rect(float x,float y,float w,float h,uint32_t color) override {
        sceGuDisable(GU_TEXTURE_2D);
        Vertex* v=static_cast<Vertex*>(sceGuGetMemory(2*sizeof(Vertex)));
        v[0]={color,x,y,0}; v[1]={color,x+w,y+h,0};
        sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
    }
    void line(float x,float y,float xx,float yy,uint32_t color) override {
        sceGuDisable(GU_TEXTURE_2D);
        Vertex* v=static_cast<Vertex*>(sceGuGetMemory(2*sizeof(Vertex)));
        v[0]={color,x,y,0}; v[1]={color,xx,yy,0};
        sceGuDrawArray(GU_LINES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
    }
    void circle(float x,float y,float radius,uint32_t color) override {
        sceGuDisable(GU_TEXTURE_2D);
        const unsigned segments=16;
        Vertex* v=static_cast<Vertex*>(sceGuGetMemory((segments+2)*sizeof(Vertex)));
        v[0]={color,x,y,0};
        for(unsigned i=0;i<=segments;++i) {
            float angle=i*6.283185307f/segments;
            v[i+1]={color,x+std::cos(angle)*radius,y+std::sin(angle)*radius,0};
        }
        sceGuDrawArray(GU_TRIANGLE_FAN,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,segments+2,0,v);
    }
    void sprite(const Texture& t,float x,float y,float w,float h,uint32_t tint) override {
        sprite_region(t,x,y,w,h,0,0,1,1,tint);
    }
    void sprite_region(const Texture& t,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t tint) override {
        if(!t.pixels) return;
        sceGuEnable(GU_TEXTURE_2D);
        if(bound!=&t) {
            sceGuTexMode(GU_PSM_4444,0,0,GU_FALSE);
            sceGuTexImage(0,t.width,t.height,t.width,t.pixels);
            sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
            sceGuTexFilter(GU_LINEAR,GU_LINEAR);
            sceGuTexWrap(GU_CLAMP,GU_CLAMP);
            sceGuTexFlush(); bound=&t;
        }
        TexVertex* v=static_cast<TexVertex*>(sceGuGetMemory(2*sizeof(TexVertex)));
        v[0]={u0*t.width,v0*t.height,tint,x,y,0}; v[1]={u1*t.width,v1*t.height,tint,x+w,y+h,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
    }
    void sprite_rotated(const Texture& t,float cx,float cy,float w,float h,float angle,uint32_t tint) override {
        if(!t.pixels)return;
        sceGuEnable(GU_TEXTURE_2D);
        if(bound!=&t){sceGuTexMode(GU_PSM_4444,0,0,GU_FALSE);sceGuTexImage(0,t.width,t.height,t.width,t.pixels);sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);sceGuTexFilter(GU_LINEAR,GU_LINEAR);sceGuTexWrap(GU_CLAMP,GU_CLAMP);sceGuTexFlush();bound=&t;}
        TexVertex* v=static_cast<TexVertex*>(sceGuGetMemory(4*sizeof(TexVertex)));float c=std::cos(angle),s=std::sin(angle);
        for(unsigned i=0;i<4;++i){float u=i==1 || i==2?1:0,tv=i>=2?1:0,x=(u-.5f)*w,y=(tv-.5f)*h;v[i]={u*t.width,tv*t.height,tint,cx+x*c-y*s,cy+x*s+y*c,0};}
        sceGuDrawArray(GU_TRIANGLE_FAN,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,0,v);
    }
    void sprite_region_rotated(const Texture& t,float x,float y,float w,float h,float cx,float cy,float u0,float v0,float u1,float v1,float angle,uint32_t tint) override {
        if(!t.pixels || w<=0 || h<=0)return;
        sceGuEnable(GU_TEXTURE_2D);
        if(bound!=&t){sceGuTexMode(GU_PSM_4444,0,0,GU_FALSE);sceGuTexImage(0,t.width,t.height,t.width,t.pixels);sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);sceGuTexFilter(GU_LINEAR,GU_LINEAR);sceGuTexWrap(GU_CLAMP,GU_CLAMP);sceGuTexFlush();bound=&t;}
        float c=std::cos(angle),s=std::sin(angle);TexVertex* v=static_cast<TexVertex*>(sceGuGetMemory(4*sizeof(TexVertex)));
        const float xs[]={x,x+w,x+w,x},ys[]={y,y,y+h,y+h},us[]={u0,u1,u1,u0},vs[]={v0,v0,v1,v1};
        for(unsigned i=0;i<4;++i){float dx=xs[i]-cx,dy=ys[i]-cy;v[i]={us[i]*t.width,vs[i]*t.height,tint,cx+dx*c-dy*s,cy+dx*s+dy*c,0};}
        sceGuDrawArray(GU_TRIANGLE_FAN,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,0,v);
    }
    void text(int x,int y,uint32_t color,const char* value) override {
        if(skin.font.pixels){draw_ui_text(*this,skin,float(x),float(y),12,color,value);return;}
        if(count==labels.size()) return;
        TextRun& run=labels[count++]; run.x=x; run.y=y; run.color=color;
        std::snprintf(run.value,sizeof(run.value),"%s",value);
    }
    void flush_text() {
        pspDebugScreenSetOffset(static_cast<int>(draw_offset));
        pspDebugScreenEnableBackColor(0);
        for(unsigned i=0;i<count;++i) {
            const TextRun& t=labels[i]; int x=t.x;
            for(const char* c=t.value;*c && x+6<=480;++c,x+=6)
                if(x>=0 && t.y>=0 && t.y+8<=272) pspDebugScreenPutChar(x,t.y,t.color,static_cast<unsigned char>(*c));
        }
    }
};
static GuPainter painter;
void Renderer::init() {
    pspDebugScreenInit();
    pspDebugScreenPrintf("Loading graphics...");
    skin.load("assets/private");
    FILE* diagnostic=std::fopen("audio-diagnostics.flag","r");
    if(diagnostic){std::fclose(diagnostic);FILE* status=std::fopen("graphics-status.txt","w");if(status){std::fprintf(status,"textures=%u resident=%u\n",skin.loaded,unsigned(skin.resident));std::fclose(status);}}

    sceKernelDcacheWritebackAll();
    sceGuInit(); sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(480,272,(void*)(512*272*4),512);
    sceGuOffset(2048-240,2048-136); sceGuViewport(2048,2048,480,272);
    sceGuScissor(0,0,480,272); sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    sceGuShadeModel(GU_SMOOTH); sceGuFinish(); sceGuSync(0,0);
    sceDisplayWaitVblankStart(); sceGuDisplay(GU_TRUE);
}
void Renderer::draw(const Game& g,unsigned physics_us,unsigned free_bytes) {
    uint64_t begin=sceKernelGetSystemTimeWide();
    sceGuStart(GU_DIRECT,list); sceGuClearColor(0xff211c28); sceGuClear(GU_COLOR_BUFFER_BIT);
    painter.begin(); paint_scene(painter,skin,g,physics_us,free_bytes,draw_us);
    sceGuFinish(); sceGuSync(0,0); painter.flush_text();
    draw_us=static_cast<unsigned>(sceKernelGetSystemTimeWide()-begin);
    sceDisplayWaitVblankStart(); void* next=sceGuSwapBuffers();
    draw_offset=static_cast<unsigned>(reinterpret_cast<uintptr_t>(next));
}
void Renderer::shutdown() { sceGuTerm(); }
}
