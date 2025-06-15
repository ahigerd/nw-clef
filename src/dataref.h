#ifndef NW_DATAREF_H
#define NW_DATAREF_H

#include <cstdint>

struct DataRef
{
  bool isOffset;
  std::uint8_t dataType;
  std::uint32_t pointer;

  inline operator bool() const { return pointer; }
};

#endif
