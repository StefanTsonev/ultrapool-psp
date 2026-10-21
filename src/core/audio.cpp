#include "ultrapool/audio.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace up {
bool SoundQueue::push(const char *name, float gain, float pitch) {
  if (!name || std::strlen(name) >= 40 || !std::isfinite(gain) ||
      !std::isfinite(pitch) || gain < 0 || pitch <= 0)
    return false;
  if (count == entries.size()) {
    ++dropped;
    return false;
  }
  SoundEvent &e = entries[(first + count++) % entries.size()];
  std::strcpy(e.name, name);
  e.gain = gain;
  e.pitch = pitch;
  return true;
}
bool SoundQueue::pop(SoundEvent &e) {
  if (!count)
    return false;
  e = entries[first];
  first = (first + 1) % entries.size();
  --count;
  return true;
}
void AudioMixer::Stream::close() {
  if (file)
    std::fclose(file);
  file = nullptr;
  memory = nullptr;
  cache_count = 0;
}
bool AudioMixer::Stream::open(const char *path, bool looping) {
  close();
  file = std::fopen(path, "rb");
  if (!file)
    return false;
  uint32_t header[4];
  if (std::fread(header, 4, 4, file) != 4 || header[0] != 0x31415055 ||
      header[1] != 22050 || (header[2] != 1 && header[2] != 2) || !header[3] ||
      header[3] > 22050u * 3600u) {
    close();
    return false;
  }
  channels = header[2];
  frames = header[3];
  if (std::fseek(file, 0, SEEK_END) != 0 ||
      std::ftell(file) !=
          16L + static_cast<long>(frames) * static_cast<long>(channels) * 2) {
    close();
    return false;
  }
  position = fraction = 0;
  step = 32768;
  loop = looping;
  return true;
}
int AudioMixer::Stream::sample(unsigned channel) {
  if (!opened())
    return 0;
  unsigned index = position;
  if (index >= frames)
    return 0;
  auto read = [&](unsigned frame) -> int {
    if (frame >= frames)
      frame = loop ? 0 : frames - 1;
    if (memory)
      return memory[frame * channels + (channels == 1 ? 0 : channel)];
    if (!cache_count || frame < cache_start ||
        frame >= cache_start + cache_count) {
      cache_start = frame ? frame - 1 : 0;
      std::fseek(file,
                 16L + static_cast<long>(cache_start) *
                           static_cast<long>(channels) * 2,
                 SEEK_SET);
      cache_count = static_cast<unsigned>(
                        std::fread(cache.data(), 2, cache.size(), file)) /
                    channels;
    }
    if (frame >= cache_start + cache_count)
      return 0;
    return cache[(frame - cache_start) * channels +
                 (channels == 1 ? 0 : channel)];
  };
  int left = read(index), right = read(index + 1),
      weight = static_cast<int>(fraction >> 8);
  return (left * (256 - weight) + right * weight) / 256;
}
void AudioMixer::Stream::advance() {
  if (!opened())
    return;
  fraction += step;
  position += fraction >> 16;
  fraction &= 65535;
  if (position >= frames) {
    if (loop)
      position %= frames;
    else
      close();
  }
}
void AudioMixer::Stream::skip(unsigned count) {
  if (!opened())
    return;
  uint64_t phase = static_cast<uint64_t>(step) * count + fraction;
  position += static_cast<uint32_t>(phase >> 16);
  fraction = static_cast<uint32_t>(phase) & 65535;
  if (position >= frames) {
    if (loop)
      position %= frames;
    else
      close();
  }
}
float AudioMixer::random() {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng / 4294967296.0f;
}
bool AudioMixer::open_file(Stream &s, const char *name, bool loop) {
  if (std::strstr(name, "..") || std::strchr(name, '/') ||
      std::strchr(name, '\\'))
    return false;
  for (unsigned i = 0; i < cached_count; ++i)
    if (!std::strcmp(cached[i].name, name)) {
      s.close();
      s.memory = cached[i].data;
      s.frames = cached[i].frames;
      s.channels = cached[i].channels;
      s.position = s.fraction = 0;
      s.step = 32768;
      s.loop = loop;
      return true;
    }
  char path[240];
  std::snprintf(path, sizeof(path), "%s/%s", root, name);
  if (!s.open(path, loop)) {
    ++errors;
    return false;
  }
  return true;
}
AudioMixer::~AudioMixer() { shutdown(); }
void AudioMixer::shutdown() {
  for (auto &s : voices)
    s.close();
  for (auto &s : music)
    s.close();
  ambient.close();
  cue_count = 0;volume_ready=false;unmute_gain=1;
  cached_count = cache_bytes = 0;
}
bool AudioMixer::load(const char *directory) {
  shutdown();
  if (!directory || std::strlen(directory) >= sizeof(root))
    return false;
  std::strcpy(root, directory);
  char path[240];
  std::snprintf(path, sizeof(path), "%s/bank.txt", root);
  FILE *f = std::fopen(path, "r");
  if (!f) {
    ++errors;
    return false;
  }
  char line[1024];
  if (!std::fgets(line, sizeof(line), f) || std::strncmp(line, "UPBANK1", 7)) {
    std::fclose(f);
    ++errors;
    return false;
  }
  while (std::fgets(line, sizeof(line), f)) {
    if (cue_count == cues.size()) {
      ++errors;
      break;
    }
    Cue &c = cues[cue_count];
    c = Cue();
    int offset = 0;
    if (std::sscanf(line, "%39s %f %f %f %f %u %u %u %n", c.name, &c.gain,
                    &c.pitch, &c.random_pitch, &c.random_db, &c.mode,
                    &c.polyphony, &c.count, &offset) != 8 ||
        !c.count || c.count > 12 || !std::isfinite(c.gain) ||
        !std::isfinite(c.pitch) || c.gain < 0 || c.pitch <= 0 ||
        c.random_pitch < 1 || c.random_db < 0) {
      ++errors;
      continue;
    }
    char *token = std::strtok(line + offset, " \r\n");
    unsigned n = 0;
    while (token && n < c.count) {
      if (std::strlen(token) >= 20)
        break;
      std::strcpy(c.files[n++], token);
      token = std::strtok(nullptr, " \r\n");
    }
    if (n != c.count) {
      ++errors;
      continue;
    }
    ++cue_count;
  }
  std::fclose(f);
  for (unsigned i = 0; i < cue_count; ++i) {
    Cue &c = cues[i];
    if (!std::strcmp(c.name, "MainSong"))
      open_file(music[0], c.files[0], true);
    if (!std::strcmp(c.name, "ShopSong"))
      open_file(music[1], c.files[0], true);
    if (!std::strcmp(c.name, "Ambient"))
      open_file(ambient, c.files[0], true);
  }
  return cue_count > 0;
}
void AudioMixer::preload(void *storage, unsigned bytes) {
  if (!storage)
    return;
  const char *names[] = {
      "player_ball_shoot", "BallOnBall",         "BallOnWall",
      "ball_pocket",       "ball_pocket_rattle", "ball_spawn",
      "buff_spawn",        "buff_land",          "score",
      "button_pos",        "button_neg",         "enter_shop"};
  for (const char *name : names)
    for (unsigned i = 0; i < cue_count; ++i)
      if (!std::strcmp(cues[i].name, name))
        for (unsigned j = 0; j < cues[i].count; ++j) {
          bool present = false;
          for (unsigned k = 0; k < cached_count; ++k)
            present |= !std::strcmp(cached[k].name, cues[i].files[j]);
          if (present || cached_count == cached.size())
            continue;
          Stream source;
          if (!open_file(source, cues[i].files[j]))
            continue;
          unsigned needed = source.frames * source.channels * 2;
          if (needed <= bytes - cache_bytes) {
            int16_t *destination = reinterpret_cast<int16_t *>(
                static_cast<unsigned char *>(storage) + cache_bytes);
            std::fseek(source.file, 16, SEEK_SET);
            if (std::fread(destination, 1, needed, source.file) == needed) {
              Cached &entry = cached[cached_count++];
              std::strcpy(entry.name, cues[i].files[j]);
              entry.data = destination;
              entry.frames = source.frames;
              entry.channels = source.channels;
              cache_bytes += needed;
            } else
              ++errors;
          }
          source.close();
        }
}
bool AudioMixer::play(const SoundEvent &event) {
  if (!std::isfinite(event.gain) || !std::isfinite(event.pitch) ||
      event.gain < 0 || event.pitch <= 0)
    return false;
  unsigned owner = 0;
  while (owner < cue_count && std::strcmp(cues[owner].name, event.name))
    ++owner;
  if (owner == cue_count) {
    ++errors;
    return false;
  }
  Cue &c = cues[owner];
  unsigned variant = static_cast<unsigned>(random() * c.count);
  if (c.mode == 0 && c.count > 1 && variant == c.last)
    variant = (variant + 1 + static_cast<unsigned>(random() * (c.count - 1))) %
              c.count;
  c.last = variant;
  unsigned slot = 16, active = 0, oldest = 16;
  for (unsigned i = 0; i < 16; ++i) {
    if (!voices[i].opened()) {
      if (slot == 16)
        slot = i;
    } else if (owners[i] == owner) {
      ++active;
      if (oldest == 16 || ages[i] < ages[oldest])
        oldest = i;
    }
  }
  if (active >= std::max(1u, c.polyphony))
    slot = oldest;
  if (slot == 16) {
    slot = 0;
    for (unsigned i = 1; i < 16; ++i)
      if (ages[i] < ages[slot])
        slot = i;
  }
  Stream &s = voices[slot];
  if (!open_file(s, c.files[variant]))
    return false;
  s.gain = c.gain * event.gain *
           std::pow(10.0f, ((random() * 2 - 1) * c.random_db) / 20);
  s.pitch =
      event.pitch * c.pitch *
      (1 / c.random_pitch + random() * (c.random_pitch - 1 / c.random_pitch));
  s.step = static_cast<uint32_t>(
      std::max(.1f, std::min(s.pitch, 8.0f)) * 32768.0f + .5f);
  owners[slot] = owner;
  ages[slot] = ++serial;
  ++played;
  return true;
}
void AudioMixer::render(int16_t *output, unsigned frames) {
  // Render in bounded chunks. Only active streams enter the sample loop.
  bool audible = false;
  while (frames) {
    unsigned count = std::min(frames, 1024u);
    static_assert(sizeof(int)==4,"Audio accumulation requires 32-bit int");
    int sum[2048]{};
    float previous = shop_blend;
    float travel = static_cast<float>(count) / (44100.0f * .2f);
    shop_blend = shop ? std::min(1.0f, shop_blend + travel)
                      : std::max(0.0f, shop_blend - travel);
    float music_start=music_volume,music_end=music_volume,sfx_start=sfx_volume,sfx_end=sfx_volume;
    if(smooth_volume){
      if(!volume_ready){current_music=music_volume;current_sfx=sfx_volume;volume_ready=true;}
      float ramp=std::min(1.0f,float(count)/(44100*.02f));
      music_start=current_music;sfx_start=current_sfx;
      current_music+=(music_volume-current_music)*ramp;current_sfx+=(sfx_volume-current_sfx)*ramp;
      float before=unmute_gain;unmute_gain=muted?0:std::min(1.0f,unmute_gain+ramp);
      music_start*=before;sfx_start*=before;music_end=current_music*unmute_gain;sfx_end=current_sfx*unmute_gain;
    }
    auto mix = [&](Stream &stream, float start_gain, float end_gain) {
      if (!stream.opened())
        return;
      if (muted || (start_gain == 0 && end_gain == 0)) {
        stream.skip(count);
        return;
      }
      int gain = static_cast<int>(
          std::max(0.0f, std::min(8.0f, start_gain)) * 4096 + .5f);
      int target = static_cast<int>(
          std::max(0.0f, std::min(8.0f, end_gain)) * 4096 + .5f);
      int delta = target - gain, carry = 0;
      for (unsigned frame = 0; frame < count && stream.opened(); ++frame) {
        int left = stream.sample(0),
            right = stream.channels == 1 ? left : stream.sample(1);
        sum[2 * frame] += (left * gain) / 4096;
        sum[2 * frame + 1] += (right * gain) / 4096;
        stream.advance();
        carry += delta;
        if (carry >= static_cast<int>(count) ||
            carry <= -static_cast<int>(count)) {
          int change = carry / static_cast<int>(count);
          gain += change;
          carry -= change * static_cast<int>(count);
        }
      }
    };
    mix(music[0], music_start * (1 - previous) * 1.2589254f,
        music_end * (1 - shop_blend) * 1.2589254f);
    mix(music[1], music_start * previous, music_end * shop_blend);
    mix(ambient, ambient_enabled ? sfx_start * .4466836f : 0,
        ambient_enabled ? sfx_end * .4466836f : 0);
    for (auto &stream : voices)
      if (stream.opened())
        mix(stream, stream.gain * sfx_start, stream.gain * sfx_end);
    int peak=1;for(unsigned i=0;i<count*2;++i)peak=std::max(peak,std::abs(sum[i]));
    int limiter=peak>32700?(32700*256)/peak:256;if(limiter<256)++limited_blocks;
    for (unsigned i = 0; i < count * 2; ++i) {
      sum[i]=(sum[i]*limiter)/256;
      if (sum[i] > 32767 || sum[i] < -32768)
        ++clipped_samples;
      int value = std::max(-32768, std::min(32767, sum[i]));
      output[i] = static_cast<int16_t>(value);
      audible |= value != 0;
    }
    output += count * 2;
    frames -= count;
  }
  ++blocks;
  if (audible)
    ++nonzero_blocks;
}
} // namespace up
