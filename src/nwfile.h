#ifndef NW_NWFILE_H
#define NW_NWFILE_H

#include <string>
#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>
#include "dataref.h"
#include "infochunk.h"
#include "viewstream.h"

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

class NWFile
{
public:
  NWFile(std::istream& is);
  NWFile(std::istream& is, NWFile* parent);

  std::uint32_t magic;
  bool isLittleEndian;
  std::uint32_t version;

  std::vector<std::string>& strings;
  std::vector<std::uint8_t> rawData;
  std::unique_ptr<const InfoChunk> info;

  NWFile* section(std::uint32_t key) const;
  std::string string(int index) const;
  viewstream getFile(int index, bool audio) const;
  viewstream getFile(int group, int index, bool audio) const;

  std::uint16_t parseU16(int offset) const;
  std::uint32_t parseU32(int offset) const;
  std::int32_t parseS32(int offset) const;
  DataRef parseDataRef(int offset) const;

private:
  std::uint16_t readU16(std::istream& is) const;
  std::uint32_t readU32(std::istream& is) const;

  void readHeader(std::istream& is);
  void readSections(std::istream& is, bool hasRefID, int sectionCount);
  void parseSTRG(NWFile* strg);
  void parseSYMB(NWFile* symb);

  NWFile* parent;
  std::streampos fileStartPos;
  std::vector<std::string> stringStore;
  std::map<std::uint32_t, std::unique_ptr<NWFile>> sections;
};

#endif
