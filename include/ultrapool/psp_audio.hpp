#pragma once
#include "ultrapool/audio.hpp"
namespace up {
class Game;
class PspAudio {
  AudioMixer mixer;
  SoundQueue commands;
  int thread = -1, channel = -1, mutex = -1, cache_partition = -1;
  volatile bool running = false;
  bool shop = false, mute = false, ambient = true, diagnostics = false;
  unsigned music_percent=22,sfx_percent=65;
  static int entry(unsigned, void *);
  void loop();

public:
  bool init();
  void update(Game &game);
  void shutdown();
};
} // namespace up
