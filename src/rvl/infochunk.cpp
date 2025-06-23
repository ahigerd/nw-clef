#include "infochunk.h"
#include "nwchunk.h"
#include <iostream>

SoundDataEntry::SoundDataEntry(NWChunk* file, int offset)
: name(file->string(file->parseU32(offset))),
  fileIndex(file->parseU32(offset + 0x4)),
  playerId(file->parseU32(offset + 0x8)),
  volume(file->parseU8(offset + 0x14)),
  priority(file->parseU8(offset + 0x15)),
  soundType(SoundType(file->parseU8(offset + 0x16))),
  remoteFilter(file->parseU8(offset + 0x17)),
  user1(file->parseU32(offset + 0x20)),
  user2(file->parseU32(offset + 0x24)),
  balancePan(file->parseU8(offset + 0x28) != 0),
  panCurve(PanCurve(file->parseU8(offset + 0x29))),
  actorPlayerId(file->parseU8(offset + 0x2A))
{
  std::uint32_t sound3dRef = file->parseDataRef(offset + 0xC).pointer;
  sound3D.flags = file->parseU32(offset + sound3dRef);
  sound3D.curve = DecayCurve(file->parseU8(offset + sound3dRef + 0x4));
  sound3D.ratio = file->parseU8(offset + sound3dRef + 0x5);
  sound3D.doppler = file->parseU8(offset + sound3dRef + 0x6);

  std::uint32_t dataRef = file->parseDataRef(offset + 0x18).pointer;
  if (soundType == SoundType::SEQ) {
    seqData.labelEntry = file->parseU32(dataRef);
    seqData.bankIndex = file->parseS32(dataRef + 4);
    seqData.trackMask = file->parseU32(dataRef + 8);
    seqData.channelPriority = file->parseU8(dataRef + 12);
    seqData.fixFlag = file->parseU8(dataRef + 13);
  } else if (soundType == SoundType::STRM) {
    strmData.startPos = file->parseU32(dataRef);
    strmData.channelCount = file->parseU16(dataRef + 4);
    strmData.trackFlags = file->parseU16(dataRef + 6);
  } else if (soundType == SoundType::WAVE) {
    waveData.waveIndex = file->parseU32(dataRef);
    waveData.trackMask = file->parseU32(dataRef + 4);
    waveData.channelPriority = file->parseU8(dataRef + 8);
    waveData.fixFlag = file->parseU8(dataRef + 9);
  } else {
    throw std::exception();
  }
}

SoundBankEntry::SoundBankEntry(NWChunk* file, int offset)
: name(file->string(file->parseU32(offset))),
  fileIndex(file->parseU32(offset + 4)),
  bankIndex(file->parseS32(offset + 8))
{
  // initializers only
}

PlayerEntry::PlayerEntry(NWChunk* file, int offset)
: name(file->string(file->parseU32(offset))),
  soundCount(file->parseU8(offset + 4)),
  heapSize(file->parseU32(offset + 8))
{
  // initializers only
}

FileEntry::FileEntry(NWChunk* file, int offset)
: mainSize(file->parseU32(offset)),
  audioSize(file->parseU32(offset + 0x4)),
  entryNumber(file->parseS32(offset + 0x8))
{
  auto nameRef = file->parseDataRef(offset + 0xC);
  if (nameRef && nameRef.isOffset) {
    name = file->parseCString(nameRef.pointer);
  }
  auto locationStart = file->parseDataRef(offset + 0x14);
  file->readDataRefTable(locationStart.pointer, positions);
}

FileEntry::Position::Position(NWChunk* file, int offset)
: group(file->parseU32(offset)),
  index(file->parseU32(offset + 4))
{
  // initializers only
}

GroupEntry::GroupEntry(NWChunk* file, int offset)
: name(file->string(file->parseU32(offset))),
  entryNumber(file->parseU32(offset + 0x4)),
  pathRef(file->parseDataRef(offset + 0x8)),
  fileOffset(file->parseU32(offset + 0x10)),
  fileSize(file->parseU32(offset + 0x14)),
  audioOffset(file->parseU32(offset + 0x18)),
  audioSize(file->parseU32(offset + 0x1C))
{
  auto itemsRef = file->parseDataRef(offset + 0x20);
  file->readDataRefTable(itemsRef.pointer, items);
}

GroupEntry::GroupItem::GroupItem(NWChunk* file, int offset)
: fileIndex(file->parseU32(offset)),
  fileOffset(file->parseU32(offset + 0x4)),
  fileSize(file->parseU32(offset + 0x8)),
  audioOffset(file->parseU32(offset + 0xC)),
  audioSize(file->parseU32(offset + 0x10))
{
  // initializers only
}

InfoChunk::InfoChunk(std::istream& is, const NWChunk::ChunkInit& init)
: NWChunk(is, init)
{
  // initializers only
}

void InfoChunk::parse()
{
  readDataRefTable(parseDataRef(0x00).pointer, soundDataEntries);
  readDataRefTable(parseDataRef(0x08).pointer, soundBankEntries);
  readDataRefTable(parseDataRef(0x10).pointer, playerEntries);
  readDataRefTable(parseDataRef(0x18).pointer, fileEntries);
  readDataRefTable(parseDataRef(0x20).pointer, groupEntries);
}
