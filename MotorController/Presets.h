#pragma once
#include <stdint.h>

struct Segment { uint32_t duration; int from; int to; };
constexpr Segment MOTOWN[] = {{2000, 0, 125}, {10000, 125, 125}};
constexpr Segment JAPAN[] = {
  {5000, 0, 255}, {5000, 255, 255}, {5000, 255, 200},
  {5000, 200, 200}, {10000, 200, 255}, {5000, 255, 255},
  {5000, 255, 200}, {5000, 200, 255}
};

// Returns zero after the sequence ends. Integer math gives exact ramp endpoints.
inline int presetPWM(const Segment *segments, unsigned count, uint32_t elapsed) {
  for (unsigned i = 0; i < count; ++i) {
    if (elapsed < segments[i].duration) {
      return segments[i].from + (segments[i].to - segments[i].from)
        * static_cast<int32_t>(elapsed) / static_cast<int32_t>(segments[i].duration);
    }
    elapsed -= segments[i].duration;
  }
  return 0;
}
