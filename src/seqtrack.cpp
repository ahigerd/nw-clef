#include "seqtrack.h"
#include "nwchunk.h"

SEQTrack::SEQTrack(NWChunk* chunk)
: chunk(chunk), parseOffset(0), loopTimestamp(-1)
{
  // initializers only
}

std::uint8_t SEQTrack::readByte()
{
  return chunk->parseU8(parseOffset++);
}

std::int16_t SEQTrack::readS16()
{
  std::int16_t value = chunk->parseS16(parseOffset);
  parseOffset += 2;
  return value;
}

std::uint32_t SEQTrack::readU24()
{
  std::uint32_t value;
  if (chunk->isLittleEndian) {
    value = readByte();
    value |= readByte() << 8;
    value |= readByte() << 16;
  } else {
    value = readByte() << 16;
    value |= readByte() << 8;
    value |= readByte();
  }
  return value;
}

std::int32_t SEQTrack::readS24()
{
  std::uint32_t value = readU24();
  return std::int32_t(value << 8) >> 8; // sign-extend
}

std::uint32_t SEQTrack::readU32()
{
  std::uint32_t value = chunk->parseU32(parseOffset);
  parseOffset += 4;
  return value;
}

std::uint32_t SEQTrack::readVLQ()
{
  std::uint32_t value = 0;
  std::uint8_t b = 0;
  do {
    b = readByte();
    value = (value << 7) | (b & 0x7F);
  } while (b & 0x80);
  return value;
}

void SEQTrack::parserPush(std::uint32_t offset)
{
  parseStack.push_back(parseOffset);
  parseOffset = offset;
}

void SEQTrack::parserPop()
{
  parseOffset = parseStack.back();
  parseStack.pop_back();
}
