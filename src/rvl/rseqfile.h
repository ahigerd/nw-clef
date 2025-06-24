#ifndef NW_RSEQFILE_H
#define NW_RSEQFILE_H

#include "seqfile.h"
#include "rseqtrack.h"

class ClefContext;
class ISequence;
class ITrack;
class RBNKFile;
class RWARFile;

class RSEQFile : public SEQFile
{
  friend class NWChunkLoader;
  friend class RSEQTrack;

protected:
  RSEQFile(std::istream& is, const ChunkInit& init);

public:
  std::string label(int index) const;

  void loadBank(RBNKFile* bank, RWARFile* war);

  ISequence* sequence(ClefContext* ctx) override;

private:
  std::vector<RSEQTrack> tracks;
  RBNKFile* bank;
  RWARFile* war;
};

#endif
