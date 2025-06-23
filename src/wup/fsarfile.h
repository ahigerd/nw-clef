#ifndef NW_FSARFILE_H
#define NW_FSARFILE_H

#include "../sarfile.h"

class FSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  FSARFile(std::istream& is, const ChunkInit& init);
};


#endif
