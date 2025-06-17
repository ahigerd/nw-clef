#include "nwfile.h"
#include "utility.h"
#include "infochunk.h"
#include <stdexcept>
#include <cassert>
#include <sstream>

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
: NWChunk(is, init), version(0)
{
  // initializers only
}

#if 0
  readHeader(is);

  if (magic == 'RSAR') {
    parseSYMB(section('SYMB'));
    //info.reset(new InfoChunk(section('INFO')));
    info = dynamic_cast<InfoChunk*>(section('INFO'));
    info->parse();
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
#endif

void NWFile::readRHeader(std::istream& is)
{
  version = readU16(is);
  is.ignore(6); // file size, header size
  std::uint32_t sectionCount = readU16(is);
  readSections(is, false, sectionCount);
}

void NWFile::readFHeader(std::istream& is)
{
  is.ignore(2); // header size
  version = readU32(is);
  is.ignore(4); // file size
  std::uint32_t sectionCount = readU16(is);
  readSections(is, true, sectionCount);
}

void NWFile::readCHeader(std::istream& is)
{
  is.ignore(2); // header size
  version = readU32(is);
  is.ignore(4); // header size
  std::uint32_t sectionCount = readU16(is);
  readSections(is, true, sectionCount);
}

void NWFile::readSections(std::istream& is, bool hasRefID, int sectionCount)
{
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
  if (parent) {
    return parent->getFile(index, audio);
  }
  return viewstream();
}

viewstream NWFile::getFile(int group, int index, bool audio) const
{
  if (parent) {
    return parent->getFile(group, index, audio);
  }
  return viewstream();
}
