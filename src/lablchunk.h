#ifndef NW_LABLCHUNK_H
#define NW_LABLCHUNK_H

#include "nwchunk.h"

class LablChunk : public NWChunk
{
  friend class NWChunkLoader;

protected:
  LablChunk(std::istream& is, const ChunkInit& init);

public:
  struct Label {
    std::string name;
    std::uint32_t dataOffset;
  };
  std::vector<Label> labels;
};

#endif
