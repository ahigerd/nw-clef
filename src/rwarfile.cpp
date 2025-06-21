#include "rwarfile.h"
#include "rwavfile.h"

RWARFile::RWARFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);

  auto tabl = section('TABL');
  std::uint32_t numWaves = tabl->parseU32(0);
  std::uint32_t pos = 4;
  for (int i = 0; i < numWaves; i++, pos += 12) {
    entries.push_back({ tabl->parseDataRef(pos), tabl->parseU32(pos + 8) });
  }
}

viewstream RWARFile::getFile(int index, bool) const
{
  if (index < 0 || index >= entries.size()) {
    return viewstream();
  }
  auto entry = entries[index];
  auto data = section('DATA');
  std::uint8_t* start = data->rawData.data() + entry.offset.pointer - 8;
  return viewstream(start, start + entry.size);
}

SampleData* RWARFile::getSample(int index) const
{
  if (index < 0 || index >= entries.size()) {
    return nullptr;
  }
  viewstream stream(getFile(index, true));
  RWAVFile* rwav = NWChunk::load<RWAVFile>(stream, nullptr, ctx);
  return rwav->sample(index);
}
