#include "ultrapool/audio.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
int main(int argc, char **argv) {
  if (argc != 3)
    return 2;
  up::AudioMixer mixer;
  if (!mixer.load(argv[1]))
    return 3;
  static int16_t cache[1536 * 512];
  mixer.preload(cache, sizeof(cache));
  char path[300];
  std::snprintf(path, sizeof(path), "%s/bank.txt", argv[1]);
  FILE *bank = std::fopen(path, "r"), *output = std::fopen(argv[2], "wb");
  if (!bank || !output)
    return 4;
  char line[1024];
  int16_t samples[1024];
  uint32_t frames = 0;
  unsigned cues = 0;
  std::fseek(output, 44, SEEK_SET);
  while (std::fgets(line, sizeof(line), bank)) {
    up::SoundEvent e;
    if (std::sscanf(line, "%39s", e.name) != 1 ||
        !std::strcmp(e.name, "UPBANK1"))
      continue;
    if (!std::strcmp(e.name, "MainSong") || !std::strcmp(e.name, "ShopSong") ||
        !std::strcmp(e.name, "Ambient"))
      continue;
    if (!mixer.play(e))
      return 5;
    ++cues;
    for (unsigned i = 0; i < 22; ++i) {
      mixer.render(samples, 512);
      std::fwrite(samples, 2, 1024, output);
      frames += 512;
    }
  }
  mixer.set_shop(true);
  for (unsigned i = 0; i < 100; ++i) {
    mixer.render(samples, 512);
    std::fwrite(samples, 2, 1024, output);
    frames += 512;
  }
  std::fclose(bank);
  uint32_t bytes = frames * 4, size = bytes + 36, rate = 44100,
           byte_rate = rate * 4;
  uint16_t format = 1, channels = 2, align = 4, bits = 16;
  std::rewind(output);
  std::fwrite("RIFF", 1, 4, output);
  std::fwrite(&size, 4, 1, output);
  std::fwrite("WAVEfmt ", 1, 8, output);
  size = 16;
  std::fwrite(&size, 4, 1, output);
  std::fwrite(&format, 2, 1, output);
  std::fwrite(&channels, 2, 1, output);
  std::fwrite(&rate, 4, 1, output);
  std::fwrite(&byte_rate, 4, 1, output);
  std::fwrite(&align, 2, 1, output);
  std::fwrite(&bits, 2, 1, output);
  std::fwrite("data", 1, 4, output);
  std::fwrite(&bytes, 4, 1, output);
  std::fclose(output);
  std::printf("Bank=%u exercised=%u errors=%u blocks=%u nonzero=%u clipped=%u "
              "mixer_bytes=%u cache_bytes=%u\n",
              mixer.cue_total(), cues, mixer.errors, mixer.blocks,
              mixer.nonzero_blocks, mixer.clipped_samples,
              static_cast<unsigned>(sizeof(mixer)), mixer.cache_bytes);
  return mixer.errors ? 6 : 0;
}
