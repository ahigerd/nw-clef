#ifndef NW_RBNKFILE_H
#define NW_RBNKFILE_H

#include "nwfile.h"

class RWARFile;

class RBNKFile : public NWFile
{
  friend class NWChunkLoader;

protected:
  RBNKFile(std::istream& is, const ChunkInit& init);

public:
  struct Sample {
    Sample(NWChunk* file, int offset);

    DataRef wave;
    std::int8_t attack;
    std::int8_t decay;
    std::int8_t sustain;
    std::int8_t release;
    std::int8_t hold;
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

  const Sample* getSample(int program, int key, int vel) const;

private:
  struct VelSplit {
    std::uint8_t minVel;
    std::uint8_t maxVel;
    Sample sample;
  };

  struct KeySplit {
    std::uint8_t minKey;
    std::uint8_t maxKey;
    std::vector<VelSplit> velSplits;
  };

  struct Program {
    std::vector<KeySplit> keySplits;
  };

  std::vector<Program> programs;

  static std::vector<VelSplit> readVelSplits(NWChunk* data, DataRef ref);
  static std::vector<KeySplit> readKeySplits(NWChunk* data, DataRef ref);
};

#endif
