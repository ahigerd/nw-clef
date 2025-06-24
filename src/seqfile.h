#ifndef NW_SEQFILE_H
#define NW_SEQFILE_H

#include "nwfile.h"
class ISequence;

class SEQFile : public NWFile
{
protected:
  SEQFile(std::istream& is, const ChunkInit& init);

  std::map<std::uint32_t, double> tempos; // seconds per tick

public:
  virtual ISequence* sequence(ClefContext* ctx) = 0;

  double ticksToTimestamp(std::uint32_t ticks) const;

  std::vector<std::int16_t> variables;
};

#endif
