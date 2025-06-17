#ifndef NW_SARFILE_H
#define NW_SARFILE_H

#include "nwfile.h"

class SARFile : public NWFile
{
protected:
  SARFile(std::istream& is, const ChunkInit& init);

  void parseSTRG(NWChunk* strg);
};

class RSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  RSARFile(std::istream& is, const ChunkInit& init);

public:
  virtual viewstream getFile(int index, bool audio) const;
  virtual viewstream getFile(int group, int index, bool audio) const;

  InfoChunk* info;

private:
  void parseSYMB(NWChunk* symb);
};

class FSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  FSARFile(std::istream& is, const ChunkInit& init);
};

class CSARFile : public SARFile
{
  friend class NWChunkLoader;

protected:
  CSARFile(std::istream& is, const ChunkInit& init);
};

#endif
