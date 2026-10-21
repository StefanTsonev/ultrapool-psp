#include "ultrapool/audio.hpp"
#include <cstdio>
#include <cstring>
#include <pspkernel.h>
#include <psppower.h>
PSP_MODULE_INFO(AudioBenchmark, 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);
static up::AudioMixer mixer;
static int16_t cache[1536 * 512], samples[1024];
int main() {
  scePowerSetClockFrequency(333, 333, 166);
  FILE *report = std::fopen("benchmark.txt", "w");
  if (!report)
    return 1;
  if (!mixer.load("assets/private/audio")) {
    std::fprintf(report, "load failed\n");
    std::fclose(report);
    sceKernelExitGame();
    return 1;
  }
  mixer.preload(cache, sizeof(cache));
  const char *names[] = {
      "BallOnBall",        "BallOnBall",        "BallOnBall",
      "BallOnWall",        "BallOnWall",        "BallOnWall",
      "player_ball_shoot", "player_ball_shoot", "player_ball_shoot",
      "buff_land",         "buff_land",         "buff_land",
      "buff_spawn",        "buff_spawn",        "score",
      "ball_pocket"};
  for (unsigned mode = 0; mode < 4; ++mode) {
    mixer.muted = mode == 3;
    mixer.set_shop(mode == 2);
    unsigned long long total = 0;
    unsigned maximum = 0;
    for (unsigned block = 0; block < 96; ++block) {
      if (mode == 1 && block % 8 == 0)
        for (const char *name : names) {
          up::SoundEvent event;
          std::strcpy(event.name, name);
          mixer.play(event);
        }
      unsigned long long begin = sceKernelGetSystemTimeWide();
      mixer.render(samples, 512);
      unsigned elapsed =
          static_cast<unsigned>(sceKernelGetSystemTimeWide() - begin);
      if (block >= 8) {
        total += elapsed;
        if (elapsed > maximum)
          maximum = elapsed;
      }
    }
    std::fprintf(
        report, "mode=%u blocks=88 average_us=%u maximum_us=%u errors=%u\n",
        mode, static_cast<unsigned>(total / 88), maximum, mixer.errors);
    std::fflush(report);
    sceKernelDelayThread(10000);
  }
  std::fclose(report);
  mixer.shutdown();
  sceKernelExitGame();
  return 0;
}
