#ifndef NW_INFOCHUNK_H
#define NW_INFOCHUNK_H

#include "dataref.h"
#include <vector>
#include <string>

class NWFile;

namespace _SoundType {
enum SoundType
{
  SEQ = 0x1,
  STRM = 0x2,
  WAVE = 0x3,
};
}
using _SoundType::SoundType;

namespace _PanCurve {
enum PanCurve
{
  SQRT,
  SQRT_0DB,
  SQRT_0DB_CLAMP,
  SIN_COS,
  SIN_COS_0DB,
  SIN_COS_0DB_CLAMP,
  LINEAR,
  LINEAR_0DB,
  LINEAR_0DB_CLAMP,
};
}
using _PanCurve::PanCurve;

namespace _DecayCurve {
enum DecayCurve
{
  LINEAR = 1,
  LOGARITHMIC = 2,
};
}
using _DecayCurve::DecayCurve;

class SoundDataEntry
{
public:
  struct Sound3D {
    std::uint32_t flags;
    DecayCurve curve;
    std::uint8_t ratio;
    std::uint8_t doppler;
  };

  struct SeqData {
    std::uint32_t labelEntry;
    std::int32_t bankIndex;
    std::uint32_t trackMask;
    std::uint8_t channelPriority;
    std::uint8_t fixFlag;
  };

  struct StrmData {
    std::uint32_t startPos;
    std::uint16_t channelCount;
    std::uint16_t trackFlags;
  };

  struct WaveData {
    std::uint32_t waveIndex;
    std::uint32_t trackMask;
    std::uint8_t channelPriority;
    std::uint8_t fixFlag;
  };

  SoundDataEntry(NWFile* file, int offset);

  std::string name;
  std::uint32_t fileIndex;
  std::uint32_t playerId;
  Sound3D sound3D;
  std::uint8_t volume;
  std::uint8_t priority;
  SoundType soundType;
  std::uint8_t remoteFilter;
  union {
    SeqData seqData;
    StrmData strmData;
    WaveData waveData;
  };
  std::uint32_t user1;
  std::uint32_t user2;
  bool balancePan;
  PanCurve panCurve;
  std::uint8_t actorPlayerId;
};

class SoundBankEntry
{
public:
  SoundBankEntry(NWFile* file, int offset);

  std::string name;
  std::uint32_t fileIndex;
  std::int32_t bankIndex;
};

class PlayerEntry
{
public:
  PlayerEntry(NWFile* file, int offset);

  std::string name;
  std::uint8_t soundCount;
  std::uint32_t heapSize;
};

class FileEntry
{
public:
  FileEntry(NWFile* file, int offset);

  std::uint32_t mainSize;
  std::uint32_t audioSize;
  std::int32_t entryNumber;
  std::string name;

  struct Position {
    Position(NWFile* file, int offset);
    std::uint32_t group;
    std::uint32_t index;
  };
  std::vector<Position> positions;
};

class GroupEntry
{
public:
  GroupEntry(NWFile* file, int offset);

  std::string name;
  std::uint32_t entryNumber;
  DataRef pathRef;
  std::uint32_t fileOffset;
  std::uint32_t fileSize;
  std::uint32_t audioOffset;
  std::uint32_t audioSize;

  class GroupItem
  {
  public:
    GroupItem(NWFile* file, int offset);

    std::uint32_t fileIndex;
    std::uint32_t fileOffset;
    std::uint32_t fileSize;
    std::uint32_t audioOffset;
    std::uint32_t audioSize;
  };
  std::vector<GroupItem> items;
};

class InfoChunk
{
public:
  InfoChunk(NWFile* file);

  std::vector<SoundDataEntry> soundDataEntries;
  std::vector<SoundBankEntry> soundBankEntries;
  std::vector<PlayerEntry> playerEntries;
  std::vector<FileEntry> fileEntries;
  std::vector<GroupEntry> groupEntries;

private:
  NWFile* file;
};

#endif
