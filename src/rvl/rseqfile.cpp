#include "rseqfile.h"
#include "lablchunk.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "clefcontext.h"
#include "nwinstrument.h"
#include "utility.h"

RSEQFile::RSEQFile(std::istream& is, const ChunkInit& init)
: SEQFile(is, init), BaseSequence(init.context)
{
  readRHeader(is);

  NWChunk* data = section('DATA');
  for (int i = 0; i < 16; i++) {
    addTrack(new RSEQTrack(this, data, i));
  }
  std::uint32_t startOffset = data->parseU32(0) - 0xC;
  data->rawData.erase(data->rawData.begin(), data->rawData.begin() + 4);
  tracks[0]->parse(startOffset);

  double maxLen = 0;
  for (auto& src : tracks) {
    double len = src->length();
    if (len > maxLen) {
      maxLen = len;
    }
  }
  for (auto& src : tracks) {
    src->maxTimestamp = maxLen;
  }
}

std::string RSEQFile::label(int index) const
{
  auto labels = section<LablChunk>('LABL');
  if (!labels || index < 0 || index >= labels->labels.size()) {
    return std::string();
  }
  return labels->labels.at(index).name;
}

ISequence* RSEQFile::sequence()
{
  return this;
}

void RSEQFile::loadBank(RBNKFile* bank, RWARFile* war)
{
  this->bank = bank;
  this->war = war;
  for (auto& track : tracks) {
    track->inst = NWInstrument(bank, war);
  }
}
