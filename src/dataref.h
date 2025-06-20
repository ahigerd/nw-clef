#ifndef NW_DATAREF_H
#define NW_DATAREF_H

#include <cstdint>
#include <ostream>

struct DataRef
{
  bool isOffset;
  std::uint8_t dataType;
  std::uint32_t pointer;

  inline operator bool() const { return pointer; }
};

inline std::ostream& operator<<(std::ostream& os, const DataRef& ref)
{
  return os << std::hex << ref.isOffset << ":" << int(ref.dataType) << "(" << ref.pointer << std::dec << ")";
}

#endif
