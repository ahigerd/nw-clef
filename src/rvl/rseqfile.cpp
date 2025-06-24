#include "rseqfile.h"
#include "lablchunk.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "clefcontext.h"
#include "seq/isequence.h"
#include "seq/itrack.h"
#include "nwinstrument.h"
#include "utility.h"

RSEQFile::RSEQFile(std::istream& is, const ChunkInit& init)
: SEQFile(is, init)
{
  readRHeader(is);

  NWChunk* data = section('DATA');
  for (int i = 0; i < 16; i++) {
    tracks.emplace_back(this, data, i);
  }
  std::uint32_t startOffset = data->parseU32(0) - 0xC;
  data->rawData.erase(data->rawData.begin(), data->rawData.begin() + 4);
  tracks[0].parse(startOffset);
}

std::string RSEQFile::label(int index) const
{
  auto labels = section<LablChunk>('LABL');
  if (!labels || index < 0 || index >= labels->labels.size()) {
    return std::string();
  }
  return labels->labels.at(index).name;
}

ISequence* RSEQFile::sequence(ClefContext* ctx)
{
  BaseSequence<RSEQTrack>* seq = new BaseSequence<RSEQTrack>(ctx);

  int i = 0;
  double len = 0;
  for (RSEQTrack& src : tracks) {
    if (!src.events.size()) {
      continue;
    }
    if (src.length() > len) {
      len = src.length();
    }
    // XXX: seq takes ownership
    seq->addTrack(&src);
    ++i;
  }

  for (RSEQTrack& src : tracks) {
    src.maxTimestamp = len;
  }

  return seq;
}

void RSEQFile::loadBank(RBNKFile* bank, RWARFile* war)
{
  this->bank = bank;
  this->war = war;
  for (auto& track : tracks) {
    track.inst = NWInstrument(bank, war);
  }
}
