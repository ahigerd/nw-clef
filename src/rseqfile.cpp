#include "rseqfile.h"
#include "lablchunk.h"

RSEQFile::RSEQFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);
}
