#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
namespace up {
struct SoundEvent {
  char name[40]{};
  float gain = 1, pitch = 1;
};
struct SoundQueue {
  std::array<SoundEvent, 64> entries{};
  unsigned first = 0, count = 0, dropped = 0;
  bool push(const char *name, float gain = 1, float pitch = 1);
  bool pop(SoundEvent &event);
};
// Single-owner streaming mixer. Platform code delivers commands between blocks.
class AudioMixer {
  struct Stream {
    const int16_t *memory = nullptr;
    bool opened() const { return file || memory; }
    FILE *file = nullptr;
    unsigned frames = 0, channels = 0, cache_start = 0, cache_count = 0;
    uint32_t position = 0, fraction = 0, step = 32768;
    float gain = 1, pitch = 1;
    bool loop = false;
    std::array<int16_t, 8192> cache{};
    bool open(const char *path, bool looping);
    void close();
    int sample(unsigned channel);
    void advance();
    void skip(unsigned count);
  };
  struct Cue {
    char name[40]{};
    char files[12][20]{};
    float gain = 1, pitch = 1, random_pitch = 1, random_db = 0;
    unsigned count = 0, mode = 0, polyphony = 1, last = 99;
  };
  std::array<Cue, 100> cues{};
  unsigned cue_count = 0;
  char root[192]{};
  std::array<Stream, 16> voices{};
  std::array<unsigned, 16> owners{};
  std::array<uint32_t, 16> ages{};
  struct Cached {
    char name[20]{};
    const int16_t *data = nullptr;
    unsigned frames = 0, channels = 0;
  };
  std::array<Cached, 64> cached{};
  unsigned cached_count = 0;
  Stream music[2], ambient;
  float shop_blend = 0;
  float current_music=.22f,current_sfx=.65f,unmute_gain=1;bool volume_ready=false;
  bool shop = false;
  uint32_t rng = 0x72a59d31, serial = 0;
  float random();
  bool open_file(Stream &stream, const char *name, bool loop = false);

public:
  unsigned cache_bytes = 0, io_reads = 0,limited_blocks=0;
  void preload(void *storage, unsigned bytes);
  unsigned errors = 0, played = 0, blocks = 0, nonzero_blocks = 0,
           clipped_samples = 0;
  float music_volume = .22f, sfx_volume = .65f;
  bool muted = false, ambient_enabled = true,smooth_volume=false;
  ~AudioMixer();
  bool load(const char *directory);
  bool play(const SoundEvent &event);
  void set_shop(bool value) { shop = value; }
  void render(int16_t *stereo, unsigned frames);
  void shutdown();
  unsigned cue_total() const { return cue_count; }
};
} // namespace up
