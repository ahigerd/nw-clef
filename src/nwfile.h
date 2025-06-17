#ifndef NW_NWFILE_H
#define NW_NWFILE_H

#include <map>
#include <memory>
#include <stdexcept>
#include "nwchunk.h"
#include "infochunk.h"

class InfoChunk;

class FileBoundsException : public std::range_error
{
public:
  FileBoundsException(int base, int offset, int size, int limit);
  FileBoundsException(int offset, int size, int limit);

  int base;
  int offset;
  int size;
  int limit;
};

class NWFile : public NWChunk
{
  friend class NWChunkLoader;

protected:
  NWFile(std::istream& is, const ChunkInit& init);

public:
  std::uint32_t version;

  NWChunk* section(std::uint32_t key) const;
  template <typename T>
  T* section(std::uint32_t key) const
  {
    return dynamic_cast<T*>(section(key));
  }

  std::string string(int index) const override;
  virtual viewstream getFile(int index, bool audio) const;
  virtual viewstream getFile(int group, int index, bool audio) const;

protected:
  void readRHeader(std::istream& is);
  void readFHeader(std::istream& is);
  void readCHeader(std::istream& is);

  std::vector<std::string> strings;

private:
  void readSections(std::istream& is, bool hasRefID, int sectionCount);

  std::map<std::uint32_t, std::unique_ptr<NWChunk>> sections;
};

#endif
