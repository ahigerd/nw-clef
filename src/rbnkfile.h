#ifndef NW_RBNKFILE_H
#define NW_RBNKFILE_H

#include "nwfile.h"

class RWARFile;

namespace _LocationType {
enum LocationType {
  INDEX,
  ADDRESS,
  CALLBACK,
};
}
using _LocationType::LocationType;

class RBNKFile : public NWFile
{
  friend class NWChunkLoader;

protected:
  RBNKFile(std::istream& is, const ChunkInit& init);

public:
  struct Program {
    Program(NWChunk* file, int offset);

    std::uint32_t waveIndex;
    std::int8_t attack;
    std::int8_t decay;
    std::int8_t sustain;
    std::int8_t release;
    std::int8_t hold;
    LocationType locationType;
    bool ignoreRelease;
    std::uint8_t alternate;
    std::uint8_t baseNote;
    std::uint8_t volume;
    std::uint8_t pan;
    std::uint8_t surround;
    std::uint32_t pitch; // TODO: ???
    DataRef lfoTable;
    DataRef envTable;
    DataRef randTable;
  };

  std::vector<Program> programs;

  void linkAudio(RWARFile* rwar);
};

#endif
