#include "ultrapool/render.hpp"
#include "ultrapool/psp_audio.hpp"
#include <pspkernel.h>
#include <cstdio>
#include <pspctrl.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspsysmem.h>
PSP_MODULE_INFO(UltrapoolNative,0,0,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(6144);
PSP_MAIN_THREAD_STACK_SIZE_KB(128);
static volatile bool running=true;
static int exit_callback(int,int,void*) { running=false; return 0; }
static int callback_thread(SceSize,void*) {
    int cb=sceKernelCreateCallback("Exit",exit_callback,0);
    sceKernelRegisterExitCallback(cb); sceKernelSleepThreadCB(); return 0;
}
int main() {
    int cb=sceKernelCreateThread("callbacks",callback_thread,0x11,0x1000,0,0);
    if (cb>=0) sceKernelStartThread(cb,0,0);
    sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    scePowerSetClockFrequency(333,333,166);
    static up::Game game; game.load_content("assets/definitions.txt"); game.reset();game.profile.load("ultrapool.profile");game.daily_history.load("ultrapool.daily");game.load_settings("ultrapool.cfg"); game.phase=up::Phase::Menu;{FILE* bank=std::fopen("assets/private/tutorial_text.uat","rb");game.tutorial.enabled=bank!=nullptr && game.tutorial_hints;if(bank)std::fclose(bank);FILE* off=std::fopen("tutorial-disabled.flag","rb");if(off){game.tutorial.enabled=false;std::fclose(off);}} up::Renderer renderer; renderer.init();
    FILE* debug_flag=std::fopen("audio-diagnostics.flag","r");if(debug_flag){game.debug_hud=true;std::fclose(debug_flag);}
    static up::PspAudio audio;if(!audio.init())game.notice("Audio unavailable: check assets/private/audio");
    uint64_t previous=sceKernelGetSystemTimeWide();
    while (running) {
        uint64_t now=sceKernelGetSystemTimeWide();
        double seconds=double(now-previous)/1000000; previous=now;
        up::Input input=up::read_input();
        if(input.more && game.phase==up::Phase::Menu){ScePspDateTime calendar;if(sceRtcGetCurrentClock(&calendar,0)>=0)game.platform_date=calendar.year*10000+calendar.month*100+calendar.day;}
        game.input(input,static_cast<float>(seconds));
        uint64_t begin=sceKernelGetSystemTimeWide(); game.update(seconds);
        unsigned elapsed=static_cast<unsigned>(sceKernelGetSystemTimeWide()-begin);
        if(game.save_pending) { bool ok=game.save_run(game.daily_date?"ultrapool.daily.sav":"ultrapool.sav"); game.save_pending=false; if(!ok) game.notice("SAVE FAILED: check Memory Stick space"); }
        if(!game.daily_date && (game.phase==up::Phase::Lost || game.phase==up::Phase::RunWon)) { std::remove("ultrapool.sav"); std::remove("ultrapool.sav.bak"); }
        if(game.daily_save_pending || game.daily_history.dirty){if(!game.flush_daily())game.notice("Daily save failed");}
        if(game.profile.dirty){if(game.profile.save("ultrapool.profile"))game.profile.dirty=false;else game.notice("Profile save failed");}
        if(game.settings_dirty){bool ok=game.save_settings("ultrapool.cfg");game.settings_dirty=false;if(!ok)game.notice("Settings could not be saved");}
        audio.update(game);
        renderer.draw(game,elapsed,sceKernelTotalFreeMemSize());
    }
    audio.shutdown();renderer.shutdown(); sceKernelExitGame(); return 0;
}