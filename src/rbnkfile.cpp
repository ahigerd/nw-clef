#include "rbnkfile.h"

RBNKFile::Program::Program(NWChunk* file, int offset)
: waveIndex(file->parseU32(0)),
  attack(file->parseS8(4)),
  decay(file->parseS8(5)),
  sustain(file->parseS8(6)),
  release(file->parseS8(7)),
  hold(file->parseS8(8)),
  locationType(LocationType(file->parseU8(9))),
  ignoreRelease(file->parseU8(10)),
  alternate(file->parseU8(11)),
  baseNote(file->parseU8(12)),
  volume(file->parseU8(13)),
  pan(file->parseU8(14)),
  surround(file->parseU8(15)),
  pitch(file->parseU32(16)),
  lfoTable(file->parseDataRef(20)),
  envTable(file->parseDataRef(28)),
  randTable(file->parseDataRef(34))
{
  // initializers only
}

RBNKFile::RBNKFile(std::istream& is, const NWChunk::ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);

  section('DATA')->readDataRefTable(0, programs);
}

void RBNKFile::linkAudio(RWARFile* rwar)
{
}
