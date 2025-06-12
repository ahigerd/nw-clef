#ifndef NW_NWFILE_H
#define NW_NWFILE_H

#include <string>
#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

struct DataRef
{
  bool isOffset;
  std::uint32_t pointer;

  inline operator bool() const { return pointer; }
};

class NWFile
{
public:
  NWFile(std::istream& is);
  NWFile(std::istream& is, std::uint32_t parentMagic, bool isLittleEndian);

  std::uint32_t magic, parentMagic;
  bool isLittleEndian;
  std::uint32_t version;

  std::map<std::uint32_t, std::unique_ptr<NWFile>> sections;
  std::vector<std::string> strings;
  std::vector<std::uint8_t> rawData;

private:
  std::uint16_t readU16(std::istream& is) const;
  std::uint32_t readU32(std::istream& is) const;
  std::uint16_t parseU16(int offset) const;
  std::uint32_t parseU32(int offset) const;
  std::int32_t parseS32(int offset) const;
  DataRef parseDataRef(int offset) const;

  void readHeader(std::istream& is);
  void readSections(std::istream& is, bool hasRefID, int sectionCount);
  void parseSTRG(NWFile* strg);
  void parseSYMB(NWFile* symb);

  std::streampos fileStartPos;
};

#endif
