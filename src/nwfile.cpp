#include "nwfile.h"
#include "utility.h"
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
