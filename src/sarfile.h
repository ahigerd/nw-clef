#ifndef NW_SARFILE_H
#define NW_SARFILE_H

#include "nwfile.h"

class SARFile : public NWFile
{
protected:
  SARFile(std::istream& is, const ChunkInit& init);

  void parseSTRG(NWChunk* strg);
};

#endif
