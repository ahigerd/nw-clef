#ifndef NW_RSEQFILE_H
#define NW_RSEQFILE_H

#include "seqfile.h"
#include "rseqtrack.h"
#include "seq/isequence.h"

class ClefContext;
class ISequence;
class ITrack;
class RBNKFile;
class RWARFile;

class RSEQFile : public SEQFile, private BaseSequence<RSEQTrack>
{
  friend class NWChunkLoader;
  friend class RSEQTrack;

protected:
  RSEQFile(std::istream& is, const ChunkInit& init);

public:
  using SEQFile::ctx;

  std::string label(int index) const;

  void loadBank(RBNKFile* bank, RWARFile* war);

  ISequence* sequence() override;

private:
  RBNKFile* bank;
  RWARFile* war;
};

#endif
