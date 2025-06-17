#include "nwfile.h"
#include "utility.h"
#include "infochunk.h"
#include <stdexcept>
#include <cassert>
#include <sstream>
#include <fstream>

static std::string formatBoundsException(int base, int offset, int size, int limit)
{
  std::ostringstream ss;
  ss << "file range out of bounds: ";
  if (base >= 0) {
    ss << base << " + ";
  } else {
    base = 0;
  }
  ss << offset << " + " << size << " (0x" << std::hex << (base + offset + size) << std::dec;
  ss << ") exceeds " << limit << " (0x" << std::hex << limit << std::dec << ")";
  return ss.str();
}

FileBoundsException::FileBoundsException(int base, int offset, int size, int limit)
: std::range_error(formatBoundsException(base, offset, size, limit))
{
  // initializers only
}

FileBoundsException::FileBoundsException(int offset, int size, int limit)
: std::range_error(formatBoundsException(-1, offset, size, limit))
{
  // initializers only
}

NWFile::NWFile(std::istream& is, const NWChunk::ChunkInit& init)
: NWChunk(is, init)
{
  readHeader(is);

  if (magic == 'RSAR') {
    parseSYMB(section('SYMB'));
    //info.reset(new InfoChunk(section('INFO')));
    info = dynamic_cast<InfoChunk*>(section('INFO'));
    info->parse();
#if 0
    for (auto file : info->fileEntries) {
      if (file.name.size()) {
        std::cout << "file " << file.name;
      } else {
        std::cout << "embedded file";
      }
      std::cout << ": main=" << file.mainSize << " audio=" << file.audioSize << std::endl;
      /*
      for (auto pos : file.positions) {
        auto group = info->groupEntries[pos.group];
        auto item = group.items[pos.index];
        std::cout << "\t" << pos.group << " group=" << group.name << " " << item.fileIndex << " index=" << pos.index << " main pos=" << (group.fileOffset + item.fileOffset) << " size=" << item.fileSize << " audio pos=" << (group.audioOffset + item.audioOffset) << " size=" << item.audioSize << std::endl;
        if (file.mainSize != item.fileSize || file.audioSize != item.audioSize) {
          std::cout << "\t\tXXX: Size mismatch" << std::endl;
        }
        auto main = getFile(pos.group, pos.index, false);
        auto audio = getFile(pos.group, pos.index, true);
        std::cout << "\t\t" << main.size() << "\t" << audio.size() << std::endl;
      }
      */
    }
#endif
    auto test = getFile(0, 0, false);
    std::string tt;
    test >> tt;
    std::cout << test.size() << " [" << tt.substr(0, 4) << "]" << std::endl;
    for (auto sound : info->soundDataEntries) {
      if (sound.soundType == SoundType::SEQ) {
        auto bank = info->soundBankEntries.at(sound.seqData.bankIndex);
        test = getFile(sound.fileIndex, false);
        test >> tt;
        std::cout << sound.name << "\t" << bank.name << "\t" << tt.substr(0, 4) << "\t" << test.size() << std::endl;
      } else {
        FileEntry f = info->fileEntries[sound.fileIndex];
        test = getFile(sound.fileIndex, false);
        std::cout << sound.name << "\t" << f.name << "\t" << test.size() << std::endl;
      }
    }
    /*
    for (auto bank : info->soundBankEntries) {
      std::cout << bank.name << "\t" << bank.fileIndex << "\t" << bank.bankIndex << std::endl;
    }
    */
  } else if (magic == 'FSAR' || magic == 'CSAR') {
    parseSTRG(section('STRG'));
  }

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
  default:
    throw std::runtime_error("unrecognized file format");
    break;
  }
}

void NWFile::readSections(std::istream& is, bool hasRefID, int sectionCount)
{
  std::cerr << "readSections " << sectionCount << " " << int(is.tellg()) << std::endl;
  if (sectionCount == 0) {
    return;
  }

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
    std::unique_ptr<NWChunk> section(NWChunk::load(is, this));
    std::swap(sections[section->magic], section);
  }
}

void NWFile::parseSTRG(NWChunk* strg)
{
  std::uint32_t offset = strg->parseU32(4);
  int numStrings = strg->parseU32(offset);
  offset += 4;
  for (int i = 0; i < numStrings; i++) {
    int stringPos = strg->parseU32(offset + 4) + 16;
    int stringSize = strg->parseU32(offset + 8);
    std::string str = strg->parseString(stringPos, stringSize);
    if (str[stringSize - 1] == '\0') {
      str.pop_back();
    }
    strings.push_back(str);
    //std::cout << str << std::endl;
    offset += 12;
  }
}

void NWFile::parseSYMB(NWChunk* symb)
{
  std::uint32_t offset = symb->parseU32(0);
  int numStrings = symb->parseU32(offset);
  for (int i = 0; i < numStrings; i++) {
    offset += 4;
    std::uint32_t stringPos = symb->parseU32(offset);
    std::string str = symb->parseCString(stringPos);
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

NWChunk* NWFile::section(std::uint32_t key) const
{
  auto iter = sections.find(key);
  if (iter == sections.end()) {
    return nullptr;
  }
  return iter->second.get();
}

viewstream NWFile::getFile(int index, bool audio) const
{
  if (index < 0 || index >= info->fileEntries.size()) {
    return viewstream();
  }
  auto entry = info->fileEntries.at(index);
  if (entry.positions.size()) {
    auto pos = entry.positions.at(0);
    return getFile(pos.group, pos.index, audio);
  }

  // External file
  std::unique_ptr<std::istream> f(new std::ifstream(entry.name, std::ios::in | std::ios::binary));
  return viewstream(std::move(f));
}

viewstream NWFile::getFile(int group, int index, bool audio) const
{
  if (group < 0 || index < 0 || group >= info->groupEntries.size()) {
    return viewstream();
  }
  auto entry = info->groupEntries.at(group);
  if (index >= entry.items.size()) {
    return viewstream();
  }
  auto item = entry.items.at(index);
  NWChunk* fileSection = section('FILE');
  int base = (audio ? entry.audioOffset : entry.fileOffset) - int(fileSection->fileStartPos) - 0xC;
  int offset = audio ? item.audioOffset : item.fileOffset;
  int size = audio ? item.audioSize : item.fileSize;
  int maxSize = audio ? entry.audioSize : entry.fileSize;
  if (offset + size > maxSize) {
    throw FileBoundsException(offset, size, maxSize);
  }
  if (base + offset + size > fileSection->rawData.size()) {
    throw FileBoundsException(base, offset, size, fileSection->rawData.size());
  }
  auto start = fileSection->rawData.data() + base + offset;
  return viewstream(start, start + size);
}
