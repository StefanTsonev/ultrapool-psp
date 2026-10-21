#include "ultrapool/core.hpp"
#include "ultrapool/psp_audio.hpp"
#include <pspaudio.h>
#include <pspkernel.h>
#include <pspsysmem.h>
namespace up {
int PspAudio::entry(unsigned, void *args) {
  PspAudio *self = *static_cast<PspAudio **>(args);
  self->loop();
  return 0;
}
bool PspAudio::init() {
  mixer.smooth_volume=true;
  FILE *flag = std::fopen("audio-diagnostics.flag", "r");
  diagnostics = flag != nullptr;
  if (flag)
    std::fclose(flag);
  if (!mixer.load("assets/private/audio"))
    return false;
  cache_partition =
      sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "audioCache",
                                    PSP_SMEM_Low, 1536 * 1024, nullptr);
  if (cache_partition >= 0)
    mixer.preload(sceKernelGetBlockHeadAddr(cache_partition), 1536 * 1024);
  mutex = sceKernelCreateSema("audioCommands", 0, 1, 1, nullptr);
  channel =
      sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
  if (mutex < 0 || channel < 0) {
    shutdown();
    return false;
  }
  thread = sceKernelCreateThread("ultrapoolAudio", entry, 0x12, 0x10000,
                                 PSP_THREAD_ATTR_USER, nullptr);
  if (thread < 0) {
    shutdown();
    return false;
  }
  running = true;
  PspAudio *self = this;
  if (sceKernelStartThread(thread, sizeof(self), &self) < 0) {
    running = false;
    sceKernelDeleteThread(thread);
    thread = -1;
    shutdown();
    return false;
  }
  return true;
}
void PspAudio::update(Game &game) {
  if (thread < 0) {
    SoundEvent discard;
    while (game.sounds.pop(discard)) {
    }
    return;
  }
  sceKernelWaitSema(mutex, 1, nullptr);
  shop = game.phase == Phase::Shop;
  ambient = game.phase != Phase::Menu;
  mute = game.audio_muted;music_percent=game.music_percent;sfx_percent=game.sfx_percent;
  SoundEvent event;
  while (game.sounds.pop(event))
    commands.push(event.name, event.gain, event.pitch);
  sceKernelSignalSema(mutex, 1);
}
void PspAudio::loop() {
  alignas(64) int16_t buffers[2][1024];
  unsigned index = 0, max_render_us = 0, music_changes = 0;
  bool last_shop = false;
  while (running) {
    SoundQueue work;
    bool nextShop, nextMute, nextAmbient;
    unsigned drops,nextMusic,nextSfx;
    sceKernelWaitSema(mutex, 1, nullptr);
    work = commands;
    commands.first = commands.count = 0;
    nextShop = shop;
    nextMute = mute;
    nextAmbient = ambient;
    drops = commands.dropped;nextMusic=music_percent;nextSfx=sfx_percent;
    sceKernelSignalSema(mutex, 1);
    uint64_t begin = sceKernelGetSystemTimeWide();
    if (nextShop != last_shop) {
      ++music_changes;
      last_shop = nextShop;
    }
    mixer.set_shop(nextShop);
    mixer.muted = nextMute;
    mixer.music_volume=nextMusic*.01f;mixer.sfx_volume=nextSfx*.01f;
    mixer.ambient_enabled = nextAmbient;
    SoundEvent event;
    while (work.pop(event))
      mixer.play(event);
    mixer.render(buffers[index], 512);
    unsigned elapsed =
        static_cast<unsigned>(sceKernelGetSystemTimeWide() - begin);
    if (elapsed > max_render_us)
      max_render_us = elapsed;
    sceAudioOutputBlocking(channel, PSP_AUDIO_VOLUME_MAX, buffers[index]);
    index ^= 1;
    if (diagnostics && mixer.blocks % 430 == 0) {
      FILE *f = std::fopen("audio-status.txt", "w");
      if (f) {
        std::fprintf(
            f,
            "blocks=%u nonzero=%u cues=%u played=%u errors=%u clipped=%u "
            "command_drops=%u mixer_bytes=%u cache_bytes=%u max_render_us=%u "
            "music_changes=%u "
            "muted=%u\n",
            mixer.blocks, mixer.nonzero_blocks, mixer.cue_total(), mixer.played,
            mixer.errors, mixer.clipped_samples, drops,
            static_cast<unsigned>(sizeof(mixer)), mixer.cache_bytes,
            max_render_us, music_changes, nextMute ? 1u : 0u);
        std::fclose(f);
      }
    }
  }
}
void PspAudio::shutdown() {
  running = false;
  if (thread >= 0) {
    sceKernelWaitThreadEnd(thread, nullptr);
    sceKernelDeleteThread(thread);
    thread = -1;
  }
  if (channel >= 0) {
    sceAudioChRelease(channel);
    channel = -1;
  }
  if (mutex >= 0) {
    sceKernelDeleteSema(mutex);
    mutex = -1;
  }
  mixer.shutdown();
  if (cache_partition >= 0) {
    sceKernelFreePartitionMemory(cache_partition);
    cache_partition = -1;
  }
}
} // namespace up
