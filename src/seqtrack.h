#ifndef NW_SEQTRACK_H
#define NW_SEQTRACK_H

#include <cstdint>
#include <vector>
#include "seq/itrack.h"

class NWChunk;

class SEQTrack /*: public ITrack*/ {
protected:
  SEQTrack(NWChunk* chunk);

  std::uint8_t readByte();
  std::int16_t readS16();
  std::uint32_t readU24();
  std::int32_t readS24();
  std::uint32_t readU32();
  std::uint32_t readVLQ();

  void parserPush(std::uint32_t offset);
  void parserPop();

  NWChunk* chunk;
  std::uint32_t parseOffset;
  std::vector<std::uint32_t> parseStack;

public:
  std::int32_t loopTimestamp;
};

#endif
