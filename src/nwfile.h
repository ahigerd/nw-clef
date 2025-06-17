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
  std::vector<std::string> strings;
  InfoChunk* info;

  NWChunk* section(std::uint32_t key) const;
  std::string string(int index) const override;
  viewstream getFile(int index, bool audio) const;
  viewstream getFile(int group, int index, bool audio) const;

private:
  void readHeader(std::istream& is);
  void readSections(std::istream& is, bool hasRefID, int sectionCount);
  void parseSTRG(NWChunk* strg);
  void parseSYMB(NWChunk* symb);

  std::vector<std::string> stringStore;
  std::map<std::uint32_t, std::unique_ptr<NWChunk>> sections;
};

#endif
