#include "seqfile.h"

SEQFile::SEQFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init), variables(256, 0)
{
  tempos[0] = 60.0 / (120 * 48); // 120 beats/minute = 2 ticks/second * 48 ticks/beat
}

double SEQFile::ticksToTimestamp(std::uint32_t ticks) const
{
  double timestamp = 0;
  double tempo = 0;
  for (auto pair : tempos) {
    if (pair.first > ticks) {
      timestamp += pair.second * pair.first;
    } else {
      timestamp += pair.second * (ticks - pair.first);
      break;
    }
  }
  return timestamp;
}
