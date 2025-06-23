#include "sarfile.h"

SARFile::SARFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init)
{
  // initializers only
}

void SARFile::parseSTRG(NWChunk* strg)
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
    offset += 12;
  }
}
