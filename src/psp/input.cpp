#include "ultrapool/render.hpp"
#include <pspctrl.h>
#include <cmath>
namespace up {
Input read_input() {
    static unsigned previous=0;
    SceCtrlData pad; sceCtrlReadBufferPositive(&pad,1);
    unsigned pressed=pad.Buttons & ~previous; previous=pad.Buttons;
    Input in;
    float x=(int(pad.Lx)-128)/127.0f, y=(int(pad.Ly)-128)/127.0f;
    in.analog_x=std::fabs(x)>0.16f ? x:0;
    in.analog_y=std::fabs(y)>0.16f ? y:0;
    in.left=pad.Buttons & PSP_CTRL_LEFT; in.right=pad.Buttons & PSP_CTRL_RIGHT;
    in.up=pad.Buttons & PSP_CTRL_UP; in.down=pad.Buttons & PSP_CTRL_DOWN;
    in.less=pad.Buttons & PSP_CTRL_LTRIGGER; in.more=pad.Buttons & PSP_CTRL_RTRIGGER;
    in.confirm=pressed & PSP_CTRL_CROSS; in.cancel=pressed & PSP_CTRL_CIRCLE;
    in.inspect=pressed & PSP_CTRL_SQUARE; in.finish=pressed & PSP_CTRL_TRIANGLE;
    in.auxiliary=pressed & PSP_CTRL_SELECT;in.pause=pressed & PSP_CTRL_START; return in;
}
}