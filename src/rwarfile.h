#ifndef NW_RWARFILE_H
#define NW_RWARFILE_H

#include "nwfile.h"

class SampleData;

class RWARFile : public NWFile
{
  friend class NWChunkLoader;

protected:
  RWARFile(std::istream& is, const ChunkInit& init);

public:
  virtual viewstream getFile(int index, bool audio) const override;
  inline int numSamples() const { return entries.size(); }
  SampleData* getSample(int index) const;

private:
  struct Entry {
    DataRef offset;
    std::uint32_t size;
  };
  std::vector<Entry> entries;
};

#endif
