#ifndef NW_NWCHUNK_H
#define NW_NWCHUNK_H

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include "viewstream.h"
#include "dataref.h"

class NWFile;
class ClefContext;

class NWChunk
{
  friend class NWChunkLoader;

protected:
  struct ChunkInit {
    std::streampos fileStartPos;
    std::uint32_t magic;
    NWFile* parent;
    ClefContext* context;
  };
  NWChunk(std::istream& is, const ChunkInit& init);

  const NWFile* parent;
  std::uint32_t version;

  std::uint16_t readU16(std::istream& is) const;
  std::uint32_t readU32(std::istream& is) const;

public:
  static NWChunk* load(std::istream& is, NWFile* parent = nullptr, ClefContext* ctx = nullptr);

  template <typename T>
  static T* load(std::istream& is, NWFile* parent = nullptr, ClefContext* ctx = nullptr)
  {
    NWChunk* chunk = load(is, parent, ctx);
    T* cast = dynamic_cast<T*>(chunk);
    if (cast) {
      return cast;
    }
    delete chunk;
    throw std::runtime_error("load returned wrong chunk type");
  }

  virtual ~NWChunk() {}

  ClefContext* ctx;
  bool isLittleEndian;
  const std::streampos fileStartPos;
  const std::uint32_t magic;
  std::vector<std::uint8_t> rawData;

  std::uint8_t parseU8(int offset) const;
  std::int8_t parseS8(int offset) const;
  std::uint16_t parseU16(int offset) const;
  std::int16_t parseS16(int offset) const;
  std::uint32_t parseU32(int offset) const;
  std::int32_t parseS32(int offset) const;
  std::string parseCString(int offset) const;
  std::string parseLPString(int offset) const;
  std::string parseString(int offset, int length) const;
  DataRef parseDataRef(int offset) const;

  template <typename T>
  void readDataRefTable(int base, std::vector<T>& table)
  {
    int n = parseU32(base);
    base += 4;
    DataRef ref;
    for (int i = 0; i < n; i++, base += 8) {
      ref = parseDataRef(base);
      table.emplace_back(this, ref.pointer);
    }
  }

  virtual std::string string(int index) const;
};

#endif
