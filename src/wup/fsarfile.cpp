#include "fsarfile.h"

FSARFile::FSARFile(std::istream& is, const ChunkInit& init)
: SARFile(is, init)
{
  readFHeader(is);
  parseSTRG(section('STRG'));
}

