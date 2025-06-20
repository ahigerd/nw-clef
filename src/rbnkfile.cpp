#include "rbnkfile.h"

void hexdump(const std::vector<uint8_t>& buffer, int limit);

RBNKFile::Sample::Sample(NWChunk* file, int offset)
: attack(file->parseS8(offset + 4)),
  decay(file->parseS8(offset + 5)),
  sustain(file->parseS8(offset + 6)),
  release(file->parseS8(offset + 7)),
  hold(file->parseS8(offset + 8)),
  ignoreRelease(file->parseU8(offset + 10)),
  alternate(file->parseU8(offset + 11)),
  baseNote(file->parseU8(offset + 12)),
  volume(file->parseU8(offset + 13)),
  pan(file->parseU8(offset + 14)),
  surround(file->parseU8(offset + 15)),
  pitch(file->parseU32(offset + 16)),
  lfoTable(file->parseDataRef(offset + 20)),
  envTable(file->parseDataRef(offset + 28)),
  randTable(file->parseDataRef(offset + 34))
{
  wave.isOffset = file->parseU8(offset + 9);
  wave.dataType = 1;
  wave.pointer = file->parseU32(offset + 0);
}

RBNKFile::RBNKFile(std::istream& is, const NWChunk::ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);

  auto data = section('DATA');
  std::uint32_t numPrograms = data->parseU32(0) - 1;
  std::uint32_t offset = 4;
  for (int i = 0; i < numPrograms; i++, offset += 8) {
    programs.push_back({ readKeySplits(data, ref) });
  }
  std::cout << "Bank: " << programs.size() << " programs" << std::endl;
  hexdump(data->rawData, data->rawData.size());
}

std::vector<RBNKFile::VelSplit> RBNKFile::readVelSplits(NWChunk* data, DataRef ref)
{
  std::uint32_t offset = ref.pointer;
  if (ref.dataType != 3) {
    return { VelSplit{ 0, 128, Sample(data, offset) } };
  }
  std::vector<VelSplit> splits;
  std::uint8_t minVel = data->parseU8(offset);
  std::uint8_t maxVel = data->parseU8(offset + 1);
  offset += 4;
  std::uint32_t prev = 0xFFFFFFFF;
  for (std::uint8_t i = minVel; i <= maxVel; i++, offset += 8) {
    DataRef ref = data->parseDataRef(offset);
    if (ref.pointer != prev) {
      splits.push_back({ i, i, Sample(data, ref.pointer) });
      prev = ref.pointer;
    } else {
      splits.back().maxVel = i;
    }
  }
  return splits;
}

std::vector<RBNKFile::KeySplit> RBNKFile::readKeySplits(NWChunk* data, DataRef ref)
{
  if (ref.dataType != 2) {
    return { KeySplit{ 0, 127, readVelSplits(data, ref) } };
  }
  std::uint32_t offset = ref.pointer;
  std::uint8_t minKey = 0;
  std::uint8_t numSplits = data->parseU8(offset++);
  std::vector<KeySplit> splits;
  for (int i = 0; i < numSplits; i++) {
    KeySplit split;
    split.minKey = minKey;
    split.maxKey = data->parseU8(offset++);
    splits.push_back(split);
    minKey = split.maxKey + 1;
  }
  for (int i = 0; i < numSplits; i++) {
    KeySplit& split = splits[i];
    DataRef ref2 = data->parseDataRef(offset);
    split.velSplits = readVelSplits(data, ref);
    offset += 8;
  }
  return splits;
}

const RBNKFile::Sample* RBNKFile::getSample(int program, int key, int vel) const
{
  if (program < 0 || program >= programs.size()) {
    return nullptr;
  }
  const Program& p = programs[program];
  for (const KeySplit& k : p.keySplits) {
    if (k.minKey <= key && key <= k.maxKey) {
      for (const VelSplit& v : k.velSplits) {
        if (v.minVel <= vel && vel <= v.maxVel) {
          return &v.sample;
        }
      }
    }
  }
  return nullptr;
}
