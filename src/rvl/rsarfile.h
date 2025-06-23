#ifndef NW_RSARFILE_H
#define NW_RSARFILE_H

#include "../sarfile.h"

class InfoChunk;

class RSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  RSARFile(std::istream& is, const ChunkInit& init);

public:
  virtual viewstream getFile(int index, bool audio) const override;
  virtual viewstream getFile(int group, int index, bool audio) const override;

  InfoChunk* info;

private:
  void parseSYMB(NWChunk* symb);
};

#endif
