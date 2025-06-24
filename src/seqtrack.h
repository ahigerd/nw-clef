#ifndef NW_SEQTRACK_H
#define NW_SEQTRACK_H

#include <cstdint>
#include <vector>
#include "nwinstrument.h"
#include "seq/itrack.h"

class NWChunk;
class SEQFile;

class SEQTrack : public ITrack {
  friend class RSEQFile;
protected:
  SEQTrack(SEQFile* file, NWChunk* chunk, int trackIndex);

  std::uint8_t readByte();
  std::int16_t readS16();
  std::uint32_t readU24();
  std::int32_t readS24();
  std::uint32_t readU32();
  std::uint32_t readVLQ();

  void parserPush(std::uint32_t offset);
  void parserPop();

  SEQFile* seqFile;
  NWChunk* chunk;
  std::uint32_t parseOffset;
  std::vector<std::uint32_t> parseStack;
  std::vector<std::pair<std::uint32_t, int>> loopStack;

  std::int32_t playbackIndex;
  double lastTimestamp;

  double bend, bendRange;
  int transpose;
  NWInstrument inst;

  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  virtual void internalReset();
  virtual SequenceEvent* translateEvent(std::int32_t& index, int loopCount) = 0;

public:
  virtual bool isFinished() const;
  virtual double length() const;

  std::int32_t loopStartTicks;
  std::int32_t loopEndTicks;
  std::int32_t loopStartIndex;
  std::int32_t loopEndIndex;
  std::int32_t trackEndIndex;
  int trackIndex;
  double maxTimestamp;
  bool finishedOnce;
};

#endif
