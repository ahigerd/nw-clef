#include "seqtrack.h"
#include "seqfile.h"
#include "nwchunk.h"

SEQTrack::SEQTrack(SEQFile* file, NWChunk* chunk, int trackIndex)
: seqFile(file),
  chunk(chunk),
  parseOffset(0),
  playbackIndex(0),
  lastTimestamp(0),
  bend(0),
  bendRange(2),
  transpose(0),
  loopStartTicks(-1),
  loopEndTicks(-1),
  loopStartIndex(-1),
  trackEndIndex(-1),
  trackIndex(trackIndex),
  maxTimestamp(-1)
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

std::shared_ptr<SequenceEvent> SEQTrack::readNextEvent()
{
  if (lastTimestamp > maxTimestamp) {
    return nullptr;
  }
  int loopCount = 0;
  int loopLength = loopEndIndex - loopStartIndex;
  while (true) {
    std::int32_t index = playbackIndex;
    if (loopStartTicks >= 0 && index >= loopEndIndex) {
      index = ((index - loopStartIndex) % loopLength) + loopStartIndex;
      loopCount = (playbackIndex - loopStartIndex) / loopLength;
    } else if (loopStartTicks < 0 && index > trackEndIndex) {
      return nullptr;
    }
    SequenceEvent* event = translateEvent(index, loopCount);
    playbackIndex = index + loopCount * loopLength + 1;
    if (event) {
      lastTimestamp = event->timestamp;
      if (lastTimestamp > maxTimestamp) {
        delete event;
        return nullptr;
      }
      return std::shared_ptr<SequenceEvent>(event);
    }
  }
}

void SEQTrack::internalReset()
{
  playbackIndex = 0;
  lastTimestamp = 0;
  bend = 0;
  bendRange = 2;
  transpose = 0;
  int numVars = seqFile->variables.size();
  for (int i = 0; i < numVars; i++) {
    seqFile->variables[i] = 0;
  }
}

bool SEQTrack::isFinished() const
{
  if (loopEndTicks < 0) {
    return true;
  } else if (loopStartTicks >= 0) {
    std::int32_t loopLength = loopEndIndex - loopStartIndex;
    // TODO: coda?
    return playbackIndex > loopEndIndex + loopLength;
  } else {
    return playbackIndex > trackEndIndex;
  }
  // return loopEndTicks < 0 || lastTimestamp >= maxTimestamp;
}

double SEQTrack::length() const
{
  if (loopEndTicks < 0) {
    return 0;
  } else if (maxTimestamp >= 0) {
    return maxTimestamp;
  } else if (loopStartTicks >= 0) {
    std::int32_t loopLength = loopEndTicks - loopStartTicks;
    // TODO: coda?
    return seqFile->ticksToTimestamp(loopEndTicks + loopLength);
  } else {
    return seqFile->ticksToTimestamp(loopEndTicks);
  }
}
