#include "rsarfile.h"
#include "infochunk.h"
#include "clefcontext.h"

RSARFile::RSARFile(std::istream& is, const ChunkInit& init)
: SARFile(is, init)
{
  readRHeader(is);
  parseSYMB(section('SYMB'));

  info = dynamic_cast<InfoChunk*>(section('INFO'));
  info->parse();
}

void RSARFile::parseSYMB(NWChunk* symb)
{
  std::uint32_t offset = symb->parseU32(0);
  int numStrings = symb->parseU32(offset);
  for (int i = 0; i < numStrings; i++) {
    offset += 4;
    std::uint32_t stringPos = symb->parseU32(offset);
    std::string str = symb->parseCString(stringPos);
    strings.push_back(str);
  }
}


viewstream RSARFile::getFile(int index, bool audio) const
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
  std::unique_ptr<std::istream> f(ctx->openFile(entry.name));
  return viewstream(std::move(f));
}

viewstream RSARFile::getFile(int group, int index, bool audio) const
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
