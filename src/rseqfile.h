#ifndef NW_RSEQFILE_H
#define NW_RSEQFILE_H

#include "nwfile.h"
#include "rseqtrack.h"

class ClefContext;
class ISequence;
class ITrack;
class RBNKFile;
class RWARFile;

class RSEQFile : public NWFile
{
  friend class NWChunkLoader;
  friend class RSEQTrack;
  friend class TrackParser;

protected:
  RSEQFile(std::istream& is, const ChunkInit& init);

public:
  std::string label(int index) const;

  void loadBank(RBNKFile* bank, RWARFile* war);

  ISequence* sequence(ClefContext* ctx) const;

private:
  std::vector<RSEQTrack> tracks;
  std::map<std::uint32_t, double> tempos; // seconds per tick
  std::map<std::uint32_t, RSEQTrack> subs;
  RBNKFile* bank;
  RWARFile* war;

  ITrack* createTrack(const RSEQTrack& src) const;
  double ticksToTimestamp(std::uint32_t ticks) const;
  static int findOffset(const RSEQTrack* src, std::uint32_t offset);
};

#endif
