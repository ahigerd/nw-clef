#ifndef NW_CSARFILE_H
#define NW_CSARFILE_H

#include "../sarfile.h"

class CSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  CSARFile(std::istream& is, const ChunkInit& init);
};


#endif
