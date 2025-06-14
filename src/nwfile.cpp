#include "nwfile.h"
#include "utility.h"
#include "infochunk.h"
#include <stdexcept>
#include <cassert>

NWFile::NWFile(std::istream& is)
: parent(nullptr), strings(stringStore)
{
  fileStartPos = is.tellg();

  char buffer[5];
  buffer[4] = '\0';
  is.read(buffer, 4);
  magic = parseMagic(buffer, 0);
  std::cerr << "Read magic " << buffer << std::endl;

  int bom1 = is.get();
  int bom2 = is.get();
  if (bom1 == 0xFF && bom2 == 0xFE) {
    isLittleEndian = true;
  } else if (bom1 == 0xFE && bom2 == 0xFF) {
    isLittleEndian = false;
  } else {
    throw std::runtime_error("unrecognized file format");
  }

  readHeader(is);

  if (magic == 'RSAR') {
    parseSYMB(section('SYMB'));
    info.reset(new InfoChunk(section('INFO')));
    for (auto file : info->fileEntries) {
      if (file.name.size()) {
        std::cout << "file " << file.name;
      } else {
        std::cout << "embedded file";
      }
      std::cout << ": main=" << file.mainSize << " audio=" << file.audioSize << std::endl;
      for (auto pos : file.positions) {
        auto group = info->groupEntries[pos.group];
        auto item = group.items[pos.index];
        std::cout << "\tgroup=" << group.name << " index=" << pos.index << " main pos=" << (group.fileOffset + item.fileOffset) << " size=" << item.fileSize << " audio pos=" << (group.audioOffset + item.audioOffset) << " size=" << item.audioSize << std::endl;
        if (file.mainSize != item.fileSize || file.audioSize != item.audioSize) {
          std::cout << "\t\tXXX: Size mismatch" << std::endl;
        }
        auto main = getFile(pos.group, pos.index, false);
        auto audio = getFile(pos.group, pos.index, true);
        std::cout << "\t\t" << main.size() << "\t" << audio.size() << std::endl;
      }
    }
  } else if (magic == 'FSAR' || magic == 'CSAR') {
    parseSTRG(section('STRG'));
  }

}

NWFile::NWFile(std::istream& is, NWFile* parent)
: parent(parent), isLittleEndian(parent->isLittleEndian), strings(parent->strings)
{
  fileStartPos = is.tellg();

  char buffer[5];
  buffer[4] = '\0';
  is.read(buffer, 4);
  magic = parseMagic(buffer, 0);
  std::cerr << "Read magic " << buffer << std::endl;

  readHeader(is);
}

void NWFile::readHeader(std::istream& is)
{
  std::uint32_t fileSize, headerSize;
  std::uint16_t sectionCount;

  switch (magic) {
  case 'RSAR':
  case 'RSEQ':
    version = readU16(is);
    fileSize = readU32(is);
    headerSize = readU16(is);
    sectionCount = readU16(is);
    readSections(is, false, sectionCount);
    break;
  case 'FSAR':
    headerSize = readU16(is);
    version = readU32(is);
    fileSize = readU32(is);
    sectionCount = readU16(is);
    is.ignore(2);
    readSections(is, true, sectionCount);
    break;
  case 'CSAR':
    headerSize = readU16(is);
    version = readU32(is);
    fileSize = readU32(is);
    sectionCount = readU32(is);
    readSections(is, true, sectionCount);
  case 'SYMB':
  case 'INFO':
  case 'STRG':
  case 'FILE':
    fileSize = readU32(is) - 8;
    if (parent && parent->magic == 'RSAR' && magic == 'FILE') {
      is.ignore(4);
      fileSize -= 4;
    }
    rawData.resize(fileSize);
    is.read(reinterpret_cast<char*>(rawData.data()), fileSize);
    break;
  default:
    throw std::runtime_error("unrecognized file format");
    break;
  }
}

std::uint16_t NWFile::readU16(std::istream& is) const
{
  char buffer[2];
  is.read(buffer, 2);
  return parseInt<std::uint16_t>(isLittleEndian, buffer, 0);
}

std::uint32_t NWFile::readU32(std::istream& is) const
{
  char buffer[4];
  is.read(buffer, 4);
  return parseInt<std::uint32_t>(isLittleEndian, buffer, 0);
}

std::uint16_t NWFile::parseU16(int offset) const
{
  return parseInt<std::uint16_t>(isLittleEndian, rawData, offset);
}

std::uint32_t NWFile::parseU32(int offset) const
{
  return parseInt<std::uint32_t>(isLittleEndian, rawData, offset);
}

std::int32_t NWFile::parseS32(int offset) const
{
  return parseInt<std::int32_t>(isLittleEndian, rawData, offset);
}

DataRef NWFile::parseDataRef(int offset) const
{
  DataRef ref;
  ref.isOffset = rawData[offset];
  ref.pointer = parseS32(offset + 4);
  return ref;
}

void NWFile::readSections(std::istream& is, bool hasRefID, int sectionCount)
{
  std::vector<std::streamoff> locators;
  for (int i = 0; i < sectionCount; i++) {
    if (hasRefID) {
      is.ignore(4);
    }

    locators.push_back(readU32(is));
    is.ignore(4);
  }

  if (!is) {
    throw std::runtime_error("invalid section table");
  }

  for (int i = 0; i < sectionCount; i++) {
    is.seekg(fileStartPos + locators[i]);
    if (!is) {
      throw std::runtime_error("section offset out of bounds");
    }
    std::unique_ptr<NWFile> section(new NWFile(is, this));
    std::swap(sections[section->magic], section);
  }
}

void NWFile::parseSTRG(NWFile* strg)
{
  std::uint32_t offset = strg->parseU32(4);
  int numStrings = strg->parseU32(offset);
  offset += 4;
  for (int i = 0; i < numStrings; i++) {
    int stringPos = strg->parseU32(offset + 4) + 16;
    int stringSize = strg->parseU32(offset + 8);
    std::string str(strg->rawData.begin() + stringPos, strg->rawData.begin() + stringPos + stringSize);
    if (str[stringSize - 1] == '\0') {
      str.pop_back();
    }
    strings.push_back(str);
    //std::cout << str << std::endl;
    offset += 12;
  }
}

void NWFile::parseSYMB(NWFile* symb)
{
  std::uint32_t offset = symb->parseU32(0);
  int numStrings = symb->parseU32(offset);
  for (int i = 0; i < numStrings; i++) {
    offset += 4;
    std::uint8_t* stringPos = symb->rawData.data() + symb->parseU32(offset);
    std::string str(reinterpret_cast<char*>(stringPos));
    strings.push_back(str);
    //std::cout << str << std::endl;
  }
}

std::string NWFile::string(int index) const
{
  if (index < 0 || index >= strings.size()) {
    return std::string();
  }
  return strings.at(index);
}

NWFile* NWFile::section(std::uint32_t key) const
{
  auto iter = sections.find(key);
  if (iter == sections.end()) {
    return nullptr;
  }
  return iter->second.get();
}

std::vector<std::uint8_t> NWFile::getFile(int index, bool audio) const
{
  if (parent) {
    return parent->getFile(index, audio);
  }
  if (index < 0 || index >= info->fileEntries.size()) {
    return std::vector<std::uint8_t>();
  }
  auto entry = info->fileEntries.at(index);
  if (entry.positions.size()) {
    auto pos = entry.positions.at(0);
    return getFile(pos.group, pos.index, audio);
  }
  // TODO: external
  return std::vector<std::uint8_t>();
}

std::vector<std::uint8_t> NWFile::getFile(int group, int index, bool audio) const
{
  if (parent) {
    return parent->getFile(group, index, audio);
  }
  if (group < 0 || index < 0 || group >= info->groupEntries.size()) {
    return std::vector<std::uint8_t>();
  }
  auto entry = info->groupEntries.at(group);
  if (index >= entry.items.size()) {
    return std::vector<std::uint8_t>();
  }
  auto item = entry.items.at(index);
  int offset = audio ? entry.audioOffset + item.audioOffset : entry.fileOffset + item.fileOffset;
  NWFile* fileSection = section('FILE');
  auto start = fileSection->rawData.begin() + offset;
  return std::vector<std::uint8_t>(start, start + (audio ? item.audioSize : item.fileSize));
}
