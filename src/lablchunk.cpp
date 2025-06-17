#include "lablchunk.h"
#include <iostream>

static void hexdump(const std::vector<uint8_t>& buffer, int limit)
{
  int size = buffer.size();
  int offset = 0;
  while (offset < size && (limit < 0 || offset < limit)) {
    int lineStart = offset;
    int i;
    std::string printable;
    std::printf("%04x: ", uint32_t(offset));
    for (i = 0; i < 16; i++) {
      if (offset < size) {
        std::printf("%02x ", uint32_t(uint8_t(buffer[offset])));
        printable += char((buffer[offset] >= 0x20 && buffer[offset] < 0x7F) ? buffer[offset] : '.');
      } else {
        std::printf("   ");
      }
      ++offset;
    }
    std::printf("%s\n", printable.c_str());
  }
}

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
