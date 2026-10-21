#include "ultrapool/audio.hpp"
#include "ultrapool/core.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace up;
namespace {
unsigned checks = 0;
void check(bool value, const char *why) {
  ++checks;
  if (!value) {
    std::fprintf(stderr, "FAIL AUDIO: %s\n", why);
    std::exit(1);
  }
}
void pcm(const char *path, int16_t left, int16_t right) {
  FILE *f = std::fopen(path, "wb");
  check(f != nullptr, "create synthetic stream");
  uint32_t h[] = {0x31415055, 22050, 2, 1000};
  std::fwrite(h, 4, 4, f);
  for (unsigned i = 0; i < 1000; ++i) {
    std::fwrite(&left, 2, 1, f);
    std::fwrite(&right, 2, 1, f);
  }
  std::fclose(f);
}
} // namespace
unsigned audio_tests() {
  checks = 0;
  SoundQueue q;
  for (unsigned i = 0; i < 64; ++i)
    check(q.push("score", float(i), 1), "sound queue accepts capacity");
  check(!q.push("score") && q.dropped == 1,
        "sound queue counts bounded overflow");
  SoundEvent e;
  for (unsigned i = 0; i < 64; ++i) {
    check(q.pop(e) && e.gain == float(i), "sound queue preserves FIFO payload");
  }
  check(!q.pop(e) && !q.push("score", 1, 0),
        "empty pop and invalid pitch rejected");
  pcm("build/audio-test-main.upa", 1000, -1000);
  pcm("build/audio-test-shop.upa", 2000, -2000);
  pcm("build/audio-test-sfx.upa", 3000, 3000);
  FILE *bank = std::fopen("build/bank.txt", "w");
  check(bank != nullptr, "create synthetic bank");
  std::fputs(
      "UPBANK1\nMainSong 1 1 1 0 0 1 1 audio-test-main.upa\nShopSong 1 1 1 0 0 "
      "1 1 audio-test-shop.upa\nscore 1 1 1 0 0 1 1 audio-test-sfx.upa\n",
      bank);
  std::fclose(bank);
  FILE *ramp = std::fopen("build/audio-test-ramp.upa", "wb");
  check(ramp != nullptr, "create resampling fixture");
  uint32_t ramp_header[] = {0x31415055, 22050, 2, 1000};
  std::fwrite(ramp_header, 4, 4, ramp);
  for (int i = 0; i < 1000; ++i) {
    int16_t value = static_cast<int16_t>(i * 20 - 10000);
    std::fwrite(&value, 2, 1, ramp);
    std::fwrite(&value, 2, 1, ramp);
  }
  std::fclose(ramp);
  bank = std::fopen("build/bank.txt", "a");
  std::fputs("ramp 1 1 1 0 0 1 1 audio-test-ramp.upa\n", bank);
  std::fclose(bank);
  AudioMixer m;
  check(m.load("build") && m.cue_total() == 4 && m.errors == 0,
        "loads bounded bank and both music streams");
  m.music_volume = .3f;
  m.sfx_volume = .5f;
  int16_t samples[2048];
  m.render(samples, 1024);
  check(samples[0] > 0 && samples[1] < 0, "music preserves stereo channels");
  unsigned before = m.blocks;
  for (unsigned i = 0; i < 12; ++i)
    m.render(samples, 1024);
  check(m.blocks == before + 12 && m.nonzero_blocks == m.blocks,
        "music loops beyond source end");
  std::array<int16_t, 2048> memory_cache{};
  m.preload(memory_cache.data(), sizeof(memory_cache));
  check(m.cache_bytes == 4000,
        "common effects preload into caller-owned bounded memory");
  std::remove("build/audio-test-sfx.upa");
  e = SoundEvent();
  std::strcpy(e.name, "score");
  check(m.play(e), "cached cue plays after its disk file is removed");
  m.render(samples, 1024);
  check(samples[0] > samples[1] && samples[1] > 0,
        "sfx mixes with stereo music");
  m.muted = true;
  m.render(samples, 1024);
  bool silent = true;
  for (auto sample : samples)
    silent &= sample == 0;
  check(silent, "mute produces zero output");
  m.muted = false;
  m.set_shop(true);
  for (unsigned i = 0; i < 12; ++i)
    m.render(samples, 1024);
  check(std::abs(samples[0] - 600) <= 1 && std::abs(samples[1] + 600) <= 1,
        "shop crossfade completes in 200 ms while music continues");
  m.music_volume = 0;
  m.sfx_volume = 1;
  std::strcpy(e.name, "ramp");
  e.pitch = 1.5f;
  check(m.play(e), "fixed-point resampling fixture starts");
  m.render(samples, 513);
  for (unsigned i = 0; i < 513; ++i)
    check(samples[2 * i] == -10000 + static_cast<int>(i) * 15 &&
              samples[2 * i + 1] == samples[2 * i],
          "fractional pitch produces reference ramp on both channels");
  m.render(samples, 100);
  check(samples[0] == -2305,
        "fractional phase remains continuous across render blocks");
  std::strcpy(e.name,"ramp");e.gain=100;check(m.play(e),"overload fixture starts");m.render(samples,512);check(m.limited_blocks>0&&m.clipped_samples==0,"peak limiter prevents boosted output from hard clipping");for(auto sample:samples)check(std::abs(int(sample))<=32700,"limited samples retain headroom");
  std::strcpy(e.name, "unknown");
  check(!m.play(e) && m.errors == 1, "missing cue reported without crashing");
  m.smooth_volume=true;m.muted=true;m.render(samples,1024);
  bool muted_clean=true;for(auto sample:samples)muted_clean&=sample==0;check(muted_clean,"PSP smoothing keeps mute immediately silent");
  m.muted=false;m.music_volume=.1f;m.sfx_volume=.2f;for(unsigned block=0;block<4;++block)m.render(samples,512);
  check(m.clipped_samples==0,"PSP volume and unmute ramps preserve output headroom");
  m.shutdown();
  m.render(samples, 1024);
  silent = true;
  for (auto sample : samples)
    silent &= sample == 0;
  check(silent, "shutdown closes every voice");
  Game g;
  g.reset();
  g.phase = Phase::Aiming;
  g.aim_preparing = true;
  float original = g.angle;
  Input in;
  in.right = true;
  for (unsigned i = 0; i < 120; ++i)
    g.input(in, 1.0f / 120);
  check(
      std::fabs((g.angle - original) - .24f) < .0001f,
      "D-pad aiming turns .24 radians per second independently of frame rate");
  in.right = false;
  in.left = true;
  for (unsigned i = 0; i < 60; ++i)
    g.input(in, 1.0f / 60);
  check(std::fabs(g.angle - original) < .0001f,
        "left aiming has matching slower speed");
  std::printf("Audio and precise aim checks: %u\n", checks);
  return checks;
}
