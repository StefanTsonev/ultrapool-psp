#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "ultrapool/presentation.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
static up::Game game;
static up::Skin skin;
static bool pressed[256]={},edge[256]={};
static LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(m==WM_DESTROY) { PostQuitMessage(0); return 0; }
    if(m==WM_KEYDOWN && w<256) { if(!pressed[w]) edge[w]=true; pressed[w]=true; return 0; }
    if(m==WM_KEYUP && w<256) { pressed[w]=false; return 0; }
    if(m==WM_KILLFOCUS) { for(unsigned i=0;i<256;++i) pressed[i]=edge[i]=false; game.paused=true; }
    return DefWindowProc(h,m,w,l);
}
static COLORREF rgb(uint32_t c) { return RGB(c&255,(c>>8)&255,(c>>16)&255); }
class WindowsPainter : public up::Painter {
    struct Cached { const up::Texture* source=nullptr; uint32_t tint=0; HDC dc=nullptr; HBITMAP bitmap=nullptr; HGDIOBJ old=nullptr; };
    std::array<Cached,up::content_count+up::snack_count+64> cache;
    HDC target;
    HFONT font;
public:
    explicit WindowsPainter(HDC dc):target(dc) {
        font=CreateFontA(-8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,FIXED_PITCH,"Terminal");
    }
    ~WindowsPainter() {
        for(auto& c:cache) if(c.dc) { SelectObject(c.dc,c.old); DeleteObject(c.bitmap); DeleteDC(c.dc); }
        DeleteObject(font);
    }
    void rect(float x,float y,float w,float h,uint32_t color) override {
        RECT r={int(x),int(y),int(x+w),int(y+h)};
        HBRUSH brush=CreateSolidBrush(rgb(color)); FillRect(target,&r,brush); DeleteObject(brush);
    }
    void line(float x,float y,float xx,float yy,uint32_t color) override {
        HPEN pen=CreatePen(PS_SOLID,1,rgb(color)); HGDIOBJ old=SelectObject(target,pen);
        MoveToEx(target,int(x),int(y),nullptr); LineTo(target,int(xx),int(yy)); SelectObject(target,old); DeleteObject(pen);
    }
    void circle(float x,float y,float r,uint32_t color) override {
        HBRUSH brush=CreateSolidBrush(rgb(color)); HGDIOBJ old=SelectObject(target,brush);
        HGDIOBJ oldPen=SelectObject(target,GetStockObject(NULL_PEN));
        Ellipse(target,int(x-r),int(y-r),int(x+r+1),int(y+r+1));
        SelectObject(target,oldPen); SelectObject(target,old); DeleteObject(brush);
    }
    void sprite(const up::Texture& source,float x,float y,float w,float h,uint32_t tint) override {
        sprite_region(source,x,y,w,h,0,0,1,1,tint);
    }
    void sprite_region(const up::Texture& source,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t tint) override {
        Cached* slot=nullptr;
        for(auto& c:cache) if(c.source==&source && c.tint==(tint&0xffffff)) { slot=&c; break; }
        if(!slot) for(auto& c:cache) if(!c.source) {
            c.source=&source; c.tint=tint&0xffffff; c.dc=CreateCompatibleDC(target);
            BITMAPINFO info={}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth=source.width; info.bmiHeader.biHeight=-int(source.height);
            info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
            void* data=nullptr; c.bitmap=CreateDIBSection(target,&info,DIB_RGB_COLORS,&data,nullptr,0);
            if(!c.bitmap || !data) { DeleteDC(c.dc); c=Cached(); return; }
            uint32_t* pixels=static_cast<uint32_t*>(data);
            for(std::size_t i=0;i<std::size_t(source.width)*source.height;++i) {
                unsigned v=source.pixels[i],a=((v>>12)&15)*17;
                unsigned r=(v&15)*17*(tint&255)/255*a/255;
                unsigned g=((v>>4)&15)*17*((tint>>8)&255)/255*a/255;
                unsigned b=((v>>8)&15)*17*((tint>>16)&255)/255*a/255;
                pixels[i]=(a<<24)|(r<<16)|(g<<8)|b;
            }
            c.old=SelectObject(c.dc,c.bitmap); slot=&c; break;
        }
        if(slot) {
            BLENDFUNCTION blend={AC_SRC_OVER,0,static_cast<BYTE>(tint>>24),AC_SRC_ALPHA};
            AlphaBlend(target,int(x),int(y),int(w),int(h),slot->dc,int(u0*source.width),int(v0*source.height),int((u1-u0)*source.width),int((v1-v0)*source.height),blend);
        }
    }
    void sprite_rotated(const up::Texture& source,float cx,float cy,float w,float h,float angle,uint32_t tint) override {
        if(!source.pixels || w<=0 || h<=0)return;
        int extent=static_cast<int>(std::ceil(std::sqrt(w*w+h*h)))+2;
        HDC dc=CreateCompatibleDC(target);BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=extent;info.bmiHeader.biHeight=-extent;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* data=nullptr;HBITMAP bitmap=CreateDIBSection(target,&info,DIB_RGB_COLORS,&data,nullptr,0);if(!bitmap || !data){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return;}
        uint32_t* pixels=static_cast<uint32_t*>(data);float c=std::cos(angle),s=std::sin(angle);
        for(int y=0;y<extent;++y)for(int x=0;x<extent;++x){float dx=x+.5f-extent*.5f,dy=y+.5f-extent*.5f,u=(dx*c+dy*s)/w+.5f,v=(-dx*s+dy*c)/h+.5f;uint32_t color=0;if(u>=0 && u<1 && v>=0 && v<1){unsigned value=source.pixels[unsigned(v*source.height)*source.width+unsigned(u*source.width)],alpha=((value>>12)&15)*17;unsigned r=(value&15)*17*(tint&255)/255*alpha/255,g=((value>>4)&15)*17*((tint>>8)&255)/255*alpha/255,b=((value>>8)&15)*17*((tint>>16)&255)/255*alpha/255;color=(alpha<<24)|(r<<16)|(g<<8)|b;}pixels[y*extent+x]=color;}
        HGDIOBJ old=SelectObject(dc,bitmap);BLENDFUNCTION blend={AC_SRC_OVER,0,static_cast<BYTE>(tint>>24),AC_SRC_ALPHA};AlphaBlend(target,int(cx-extent*.5f),int(cy-extent*.5f),extent,extent,dc,0,0,extent,extent,blend);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    }
    void sprite_region_rotated(const up::Texture& source,float x,float y,float w,float h,float cx,float cy,float u0,float v0,float u1,float v1,float angle,uint32_t tint) override {
        if(!source.pixels || w<=0 || h<=0)return;
        float rx=std::max(std::fabs(x-cx),std::fabs(x+w-cx)),ry=std::max(std::fabs(y-cy),std::fabs(y+h-cy));
        int extent=static_cast<int>(std::ceil(2*std::sqrt(rx*rx+ry*ry)))+2;
        HDC dc=CreateCompatibleDC(target);BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=extent;info.bmiHeader.biHeight=-extent;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* data=nullptr;HBITMAP bitmap=CreateDIBSection(target,&info,DIB_RGB_COLORS,&data,nullptr,0);if(!bitmap || !data){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return;}
        uint32_t* pixels=static_cast<uint32_t*>(data);float c=std::cos(angle),s=std::sin(angle);
        for(int py=0;py<extent;++py)for(int px=0;px<extent;++px){float dx=px+.5f-extent*.5f+cx,dy=py+.5f-extent*.5f+cy,lx=(dx-cx)*c+(dy-cy)*s+x,ly=-(dx-cx)*s+(dy-cy)*c+y;uint32_t color=0;if(lx>=x && lx<x+w && ly>=y && ly<y+h){float u=u0+(lx-x)/w*(u1-u0),v=v0+(ly-y)/h*(v1-v0);unsigned value=source.pixels[unsigned(v*source.height)*source.width+unsigned(u*source.width)],alpha=((value>>12)&15)*17;unsigned r=(value&15)*17*(tint&255)/255*alpha/255,g=((value>>4)&15)*17*((tint>>8)&255)/255*alpha/255,b=((value>>8)&15)*17*((tint>>16)&255)/255*alpha/255;color=(alpha<<24)|(r<<16)|(g<<8)|b;}pixels[py*extent+px]=color;}
        HGDIOBJ old=SelectObject(dc,bitmap);BLENDFUNCTION blend={AC_SRC_OVER,0,static_cast<BYTE>(tint>>24),AC_SRC_ALPHA};AlphaBlend(target,int(cx-extent*.5f),int(cy-extent*.5f),extent,extent,dc,0,0,extent,extent,blend);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    }
    void text(int x,int y,uint32_t color,const char* value) override {
        if(skin.font.pixels){up::draw_ui_text(*this,skin,float(x),float(y),12,color,value);return;}
        HGDIOBJ old=SelectObject(target,font); SetBkMode(target,TRANSPARENT); SetTextColor(target,rgb(color));
        TextOutA(target,x,y,value,lstrlenA(value)); SelectObject(target,old);
    }
};
static void locate_assets() {
    char directory[MAX_PATH],candidate[MAX_PATH];
    DWORD n=GetModuleFileNameA(nullptr,directory,MAX_PATH);
    if(!n || n>=MAX_PATH) return;
    char* slash=std::strrchr(directory,'\\'); if(!slash) return; *slash=0;
    for(unsigned depth=0;depth<5;++depth) {
        std::snprintf(candidate,sizeof(candidate),"%s/assets/definitions.txt",directory);
        DWORD attributes=GetFileAttributesA(candidate);
        if(attributes!=INVALID_FILE_ATTRIBUTES && !(attributes&FILE_ATTRIBUTE_DIRECTORY)) {
            SetCurrentDirectoryA(directory); return;
        }
        slash=std::strrchr(directory,'\\'); if(!slash) break; *slash=0;
    }
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int show) {
    WNDCLASSA wc={}; wc.lpfnWndProc=proc; wc.hInstance=instance; wc.lpszClassName="UltrapoolNative";
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); RegisterClassA(&wc);
    RECT size={0,0,960,544}; AdjustWindowRect(&size,WS_OVERLAPPEDWINDOW,FALSE);
    HWND window=CreateWindowA(wc.lpszClassName,"Ultrapool native - fidelity development",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,nullptr);
    if(!window) return 1;
    locate_assets();
    game.load_content("assets/definitions.txt"); game.reset();game.profile.load("ultrapool.profile");game.daily_history.load("ultrapool.daily");game.load_settings("ultrapool.cfg"); game.phase=up::Phase::Menu;{FILE* bank=std::fopen("assets/private/tutorial_text.uat","rb");game.tutorial.enabled=bank!=nullptr && game.tutorial_hints;if(bank)std::fclose(bank);FILE* off=std::fopen("tutorial-disabled.flag","rb");if(off){game.tutorial.enabled=false;std::fclose(off);}}
    skin.load("assets/private"); ShowWindow(window,show);
    HDC dc=GetDC(window),back=CreateCompatibleDC(dc);
    HBITMAP bitmap=CreateCompatibleBitmap(dc,480,272); HGDIOBJ old=SelectObject(back,bitmap);
    {
        WindowsPainter painter(back); auto previous=std::chrono::steady_clock::now(); bool run=true;
        while(run) {
            MSG message;
            while(PeekMessage(&message,nullptr,0,0,PM_REMOVE)) {
                if(message.message==WM_QUIT) run=false;
                TranslateMessage(&message); DispatchMessage(&message);
            }
            auto now=std::chrono::steady_clock::now();
            float dt=std::chrono::duration<float>(now-previous).count(); previous=now;
            up::Input in; in.left=pressed[VK_LEFT]; in.right=pressed[VK_RIGHT];
            in.up=pressed[VK_UP]; in.down=pressed[VK_DOWN];
            in.analog_x=(pressed['D']?1.0f:0)-(pressed['A']?1.0f:0);
            in.analog_y=(pressed['S']?1.0f:0)-(pressed['W']?1.0f:0);
            in.more=pressed['E']; in.less=pressed['Q']; in.inspect=edge['I']; in.finish=edge['F'];
            in.confirm=edge[VK_RETURN]||edge[VK_SPACE]; in.cancel=edge[VK_ESCAPE]; in.pause=edge['P'];in.auxiliary=edge['M'];
            for(unsigned i=0;i<256;++i) edge[i]=false;
            game.input(in,dt); game.update(dt);
            if(game.save_pending) { bool ok=game.save_run(game.daily_date?"ultrapool.daily.sav":"ultrapool.sav"); game.save_pending=false; if(!ok) game.notice("SAVE FAILED: check the game folder is writable"); }
            if(!game.daily_date && (game.phase==up::Phase::Lost || game.phase==up::Phase::RunWon)) { std::remove("ultrapool.sav"); std::remove("ultrapool.sav.bak"); }
            if(game.daily_save_pending || game.daily_history.dirty){if(!game.flush_daily())game.notice("Daily save failed");}
        if(game.profile.dirty){if(game.profile.save("ultrapool.profile"))game.profile.dirty=false;else game.notice("Profile save failed");}
        if(game.settings_dirty){bool ok=game.save_settings("ultrapool.cfg");game.settings_dirty=false;if(!ok)game.notice("Settings could not be saved");}
            up::paint_scene(painter,skin,game,0,0);
            RECT client; GetClientRect(window,&client);
            StretchBlt(dc,0,0,client.right,client.bottom,back,0,0,480,272,SRCCOPY); Sleep(8);
        }
    }
    SelectObject(back,old); DeleteObject(bitmap); DeleteDC(back); ReleaseDC(window,dc); return 0;
}
