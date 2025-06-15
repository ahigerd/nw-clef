#include "infochunk.h"
#include "nwfile.h"
#include <iostream>

template <typename T>
static void readTable(NWFile* file, int base, std::vector<T>& table)
{
  int n = file->parseU32(base);
  base += 4;
  DataRef ref;
  for (int i = 0; i < n; i++, base += 8) {
    ref = file->parseDataRef(base);
    table.emplace_back(file, ref.pointer);
  }
}

SoundDataEntry::SoundDataEntry(NWFile* file, int offset)
: name(file->string(file->parseU32(offset))),
  fileIndex(file->parseU32(offset + 0x4)),
  playerId(file->parseU32(offset + 0x8)),
  volume(file->rawData[offset + 0x14]),
  priority(file->rawData[offset + 0x15]),
  soundType(SoundType(file->rawData[offset + 0x16])),
  remoteFilter(file->rawData[offset + 0x17]),
  user1(file->parseU32(offset + 0x20)),
  user2(file->parseU32(offset + 0x24)),
  balancePan(file->rawData[offset + 0x28] != 0),
  panCurve(PanCurve(file->rawData[offset + 0x29])),
  actorPlayerId(file->rawData[offset + 0x2A])
{
  std::uint32_t sound3dRef = file->parseDataRef(offset + 0xC).pointer;
  sound3D.flags = file->parseU32(offset + sound3dRef);
  sound3D.curve = DecayCurve(file->rawData[offset + sound3dRef + 0x4]);
  sound3D.ratio = file->rawData[offset + sound3dRef + 0x5];
  sound3D.doppler = file->rawData[offset + sound3dRef + 0x6];

  std::uint32_t dataRef = file->parseDataRef(offset + 0x18).pointer;
  if (soundType == SoundType::SEQ) {
    seqData.labelEntry = file->parseU32(dataRef);
    seqData.bankIndex = file->parseS32(dataRef + 4);
    seqData.trackMask = file->parseU32(dataRef + 8);
    seqData.channelPriority = file->rawData[dataRef + 12];
    seqData.fixFlag = file->rawData[dataRef + 13];
  } else if (soundType == SoundType::STRM) {
    strmData.startPos = file->parseU32(dataRef);
    strmData.channelCount = file->parseU16(dataRef + 4);
    strmData.trackFlags = file->parseU16(dataRef + 6);
  } else if (soundType == SoundType::WAVE) {
    waveData.waveIndex = file->parseU32(dataRef);
    waveData.trackMask = file->parseU32(dataRef + 4);
    waveData.channelPriority = file->rawData[dataRef + 8];
    waveData.fixFlag = file->rawData[dataRef + 9];
  } else {
    throw std::exception();
  }
}

SoundBankEntry::SoundBankEntry(NWFile* file, int offset)
: name(file->string(file->parseU32(offset))),
  fileIndex(file->parseU32(offset + 4)),
  bankIndex(file->parseS32(offset + 8))
{
  // initializers only
}

PlayerEntry::PlayerEntry(NWFile* file, int offset)
: name(file->string(file->parseU32(offset))),
  soundCount(file->rawData[offset + 4]),
  heapSize(file->parseU32(offset + 8))
{
  // initializers only
}

FileEntry::FileEntry(NWFile* file, int offset)
: mainSize(file->parseU32(offset)),
  audioSize(file->parseU32(offset + 0x4)),
  entryNumber(file->parseS32(offset + 0x8))
{
  auto nameRef = file->parseDataRef(offset + 0xC);
  if (nameRef && nameRef.isOffset) {
    name = std::string((char*)&*(file->rawData.begin() + nameRef.pointer));
  }
  auto locationStart = file->parseDataRef(offset + 0x14);
  readTable(file, locationStart.pointer, positions);
}

FileEntry::Position::Position(NWFile* file, int offset)
: group(file->parseU32(offset)),
  index(file->parseU32(offset + 4))
{
  // initializers only
}

GroupEntry::GroupEntry(NWFile* file, int offset)
: name(file->string(file->parseU32(offset))),
  entryNumber(file->parseU32(offset + 0x4)),
  pathRef(file->parseDataRef(offset + 0x8)),
  fileOffset(file->parseU32(offset + 0x10)),
  fileSize(file->parseU32(offset + 0x14)),
  audioOffset(file->parseU32(offset + 0x18)),
  audioSize(file->parseU32(offset + 0x1C))
{
  auto itemsRef = file->parseDataRef(offset + 0x20);
  readTable(file, itemsRef.pointer, items);
}

GroupEntry::GroupItem::GroupItem(NWFile* file, int offset)
: fileIndex(file->parseU32(offset)),
  fileOffset(file->parseU32(offset + 0x4)),
  fileSize(file->parseU32(offset + 0x8)),
  audioOffset(file->parseU32(offset + 0xC)),
  audioSize(file->parseU32(offset + 0x10))
{
  // initializers only
}

InfoChunk::InfoChunk(NWFile* file)
: file(file)
{
  if (!file) {
    return;
  }
  readTable(file, file->parseDataRef(0x00).pointer, soundDataEntries);
  readTable(file, file->parseDataRef(0x08).pointer, soundBankEntries);
  readTable(file, file->parseDataRef(0x10).pointer, playerEntries);
  readTable(file, file->parseDataRef(0x18).pointer, fileEntries);
  readTable(file, file->parseDataRef(0x20).pointer, groupEntries);
}

