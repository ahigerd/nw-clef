#include "lablchunk.h"
#include <iostream>

LablChunk::LablChunk(std::istream& is, const ChunkInit& init)
: NWChunk(is, init)
{
  std::uint32_t numLabels = parseU32(0);
  std::uint32_t pos = 4;
  for (int i = 0; i < numLabels; i++, pos += 4) {
    std::uint32_t offset = parseU32(pos);
    std::uint32_t dataOffset = parseU32(offset);
    std::string name = parseLPString(offset + 4);
    labels.push_back({ name, dataOffset });
  }
}
