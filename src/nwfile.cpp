#include "nwfile.h"
#include "utility.h"
#include <stdexcept>

NWFile::NWFile(std::istream& is)
: parentMagic(0)
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
    parseSYMB(sections.at('SYMB').get());
    NWFile* info = sections.at('INFO').get();
    std::uint32_t tableOffset = info->parseU32(12);
    int numFiles = info->parseU32(tableOffset);
    std::cout << numFiles << std::endl;
    tableOffset += 4;
    std::cout << "start @ 0x" << std::hex << int(info->fileStartPos) << std::dec << std::endl;
    for (int i = 0; i < numFiles; i++) {
      DataRef fileOffset = info->parseDataRef(tableOffset);
      std::cout << "offset: " << fileOffset.isOffset << " 0x" << std::hex << fileOffset.pointer << std::dec << std::endl;
      std::uint32_t mainSize = info->parseU32(fileOffset.pointer);
      std::uint32_t audioSize = info->parseU32(fileOffset.pointer + 4);
      DataRef nameRef = info->parseDataRef(fileOffset.pointer + 8);
      if (nameRef) {
        std::cout << "name ref: " << nameRef.isOffset << " 0x" << std::hex << nameRef.pointer << std::dec;
        if (!nameRef.isOffset) {
          std::cout << ": " << strings[nameRef.pointer];
        }
        std::cout << std::endl;
      } else {
        std::cout << "embedded" << std::endl;
      }
      DataRef groupsRef = info->parseDataRef(fileOffset.pointer + 20);
      if (groupsRef) {
        std::cout << "groups ref: " << groupsRef.isOffset << " 0x" << std::hex << groupsRef.pointer << std::dec;
        std::cout << std::endl;
      }
      tableOffset += 8;
    }
  } else if (magic == 'FSAR' || magic == 'CSAR') {
    parseSTRG(sections.at('STRG').get());
  }

}

NWFile::NWFile(std::istream& is, std::uint32_t parentMagic, bool isLittleEndian)
: parentMagic(parentMagic), isLittleEndian(isLittleEndian)
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
    if (parentMagic == 'RSAR' && magic == 'FILE') {
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
    std::unique_ptr<NWFile> section(new NWFile(is, magic, isLittleEndian));
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
    std::cout << str << std::endl;
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
    std::cout << str << std::endl;
  }
}
