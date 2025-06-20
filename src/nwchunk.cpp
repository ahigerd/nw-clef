#include "nwchunk.h"
#include "nwfile.h"
#include "sarfile.h"
#include "rseqfile.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "rwavfile.h"
#include "lablchunk.h"
#include "infochunk.h"
#include "utility.h"
#include <map>

class NWChunkLoader
{
public:
  using Loader = NWChunk*(*)(std::istream&, const NWChunk::ChunkInit&);

  template <typename T>
  static NWChunk* load(std::istream& is, const NWChunk::ChunkInit& init)
  {
    return new T(is, init);
  }

  std::uint32_t key;
  std::uint32_t parentKey;
  Loader ctor;
};

static NWChunkLoader loaders[] = {
  { 'RSAR', 0, NWChunkLoader::load<RSARFile> },
  { 'FSAR', 0, NWChunkLoader::load<FSARFile> },
  { 'CSAR', 0, NWChunkLoader::load<CSARFile> },
  { 'RSEQ', 0, NWChunkLoader::load<RSEQFile> },
  { 'RBNK', 0, NWChunkLoader::load<RBNKFile> },
  { 'RWAR', 0, NWChunkLoader::load<RWARFile> },
  { 'RWAV', 0, NWChunkLoader::load<RWAVFile> },
  { 'INFO', 'RSAR', NWChunkLoader::load<InfoChunk> },
  { 'LABL', 'RSEQ', NWChunkLoader::load<LablChunk> },
};

static std::uint32_t readMagic(std::istream& is)
{
  char buffer[5];
  buffer[4] = '\0';
  is.read(buffer, 4);
  std::cerr << "Read magic " << buffer << std::endl;
  return parseMagic(buffer, 0);
}

NWChunk* NWChunk::load(std::istream& is, NWFile* parent, ClefContext* ctx)
{
  std::streampos start = is.tellg();
  std::uint32_t magic = readMagic(is);
  std::uint32_t parentMagic = parent ? parent->magic : 0;
  if (!ctx && parent) {
    ctx = parent->ctx;
  }
  if (!ctx) {
    throw std::runtime_error("no ctx");
  }
  ChunkInit init{ start, magic, parent, ctx };

  for (auto loader : loaders) {
    if (loader.key == magic && loader.parentKey == parentMagic) {
      return loader.ctor(is, init);
    }
  }
  return new NWChunk(is, init);
}

NWChunk::NWChunk(std::istream& is, const NWChunk::ChunkInit& init)
: parent(init.parent),
  ctx(init.context),
  fileStartPos(init.fileStartPos),
  magic(init.magic)
{
  if (parent) {
    isLittleEndian = parent->isLittleEndian;

    std::uint32_t fileSize = readU32(is) - 8;
    // XXX: Is there a cleaner way to do this?
    if (parent && parent->magic == 'RSAR' && magic == 'FILE') {
      is.ignore(4);
      fileSize -= 4;
    }
    rawData.resize(fileSize);
    is.read(reinterpret_cast<char*>(rawData.data()), fileSize);
  } else {
    int bom1 = is.get();
    int bom2 = is.get();
    if (bom1 == 0xFF && bom2 == 0xFE) {
      isLittleEndian = true;
    } else if (bom1 == 0xFE && bom2 == 0xFF) {
      isLittleEndian = false;
    } else {
      throw std::runtime_error("unrecognized file format");
    }
  }
}

std::uint16_t NWChunk::readU16(std::istream& is) const
{
  char buffer[2];
  is.read(buffer, 2);
  return parseInt<std::uint16_t>(isLittleEndian, buffer, 0);
}

std::uint32_t NWChunk::readU32(std::istream& is) const
{
  char buffer[4];
  is.read(buffer, 4);
  return parseInt<std::uint32_t>(isLittleEndian, buffer, 0);
}

std::uint8_t NWChunk::parseU8(int offset) const
{
  return rawData[offset];
}

std::int8_t NWChunk::parseS8(int offset) const
{
  return rawData[offset];
}

std::uint16_t NWChunk::parseU16(int offset) const
{
  return parseInt<std::uint16_t>(isLittleEndian, rawData, offset);
}

std::int16_t NWChunk::parseS16(int offset) const
{
  return parseInt<std::int16_t>(isLittleEndian, rawData, offset);
}

std::uint32_t NWChunk::parseU32(int offset) const
{
  return parseInt<std::uint32_t>(isLittleEndian, rawData, offset);
}

std::int32_t NWChunk::parseS32(int offset) const
{
  return parseInt<std::int32_t>(isLittleEndian, rawData, offset);
}

std::string NWChunk::parseCString(int offset) const
{
  // TODO: bounds checking
  return std::string((char*)&*(rawData.begin() + offset));
}

std::string NWChunk::parseLPString(int offset) const
{
  // TODO: bounds checking
  int stringSize = parseU32(offset);
  return parseString(offset + 4, stringSize);
}

std::string NWChunk::parseString(int offset, int length) const
{
  // TODO: bounds checking
  return std::string(rawData.begin() + offset, rawData.begin() + offset + length);
}

DataRef NWChunk::parseDataRef(int offset) const
{
  DataRef ref;
  ref.isOffset = rawData[offset];
  ref.dataType = rawData[offset + 1];
  ref.pointer = parseS32(offset + 4);
  return ref;
}

std::string NWChunk::string(int index) const {
  if (parent) {
    return parent->string(index);
  }
  return std::string();
}
