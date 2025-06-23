#include "csarfile.h"

CSARFile::CSARFile(std::istream& is, const ChunkInit& init)
: SARFile(is, init)
{
  readCHeader(is);
  parseSTRG(section('STRG'));
}
