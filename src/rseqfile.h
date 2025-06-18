#ifndef NW_RSEQFILE_H
#define NW_RSEQFILE_H

#include "nwfile.h"

class ClefContext;
class ISequence;
class ITrack;

namespace _RSEQCmd {
enum RSEQCmd {
  Rest = 0x80,
  ProgramChange = 0x81,
  AddTrack = 0x88,
  Goto = 0x89,
  Gosub = 0x8A,
  Ppqn = 0x8B,
  Hold = 0xB1,
  Mono = 0xB2,
  VelocityRange = 0xB3,
  PrefixRand = 0xA0,
  PrefixVar = 0xA1,
  PrefixIf = 0xA2,
  PrefixTime = 0xA3,
  PrefixTimeRand = 0xA4,
  PrefixTimeVar = 0xA5,
  Pan = 0xC0,
  Volume = 0xC1,
  MainVolume = 0xC2,
  Transpose = 0xC3,
  Bend = 0xC4,
  BendRange = 0xC5,
  Priority = 0xC6,
  WaitEnable = 0xC7,
  Tie = 0xC8,
  Portamento = 0xC9,
  ModDepth = 0xCA,
  ModSpeed = 0xCB,
  ModType = 0xCC,
  ModRange = 0xCD,
  PortaSpeed = 0xCE,
  PortaTime = 0xCF,
  Attack = 0xD0,
  Decay = 0xD1,
  Sustain = 0xD2,
  Release = 0xD3,
  LoopStart = 0xD4,
  Expression = 0xD5,
  DebugPrint = 0xD6,
  Surround = 0xD7,
  Cutoff = 0xD8,
  setSendA = 0xD9,
  SendB = 0xDA,
  MainSend = 0xDB,
  InitialPan = 0xDC,
  Mute = 0xDD,
  SendC = 0xDE,
  Damper = 0xDF,
  ModDelay = 0xE0,
  Tempo = 0xE1,
  Sweep = 0xE2,
  Extended = 0xF0,
  LoopEnd = 0xFC,
  Return = 0xFD,
  AllocTracks = 0xFE,
  EOT = 0xFF,

  ExtendedBase = 0x100,
  VarSet = 0x180,
  VarAdd = 0x181,
  VarSub = 0x182,
  VarMul = 0x183,
  VarDiv = 0x184,
  VarShift = 0x185,
  VarRand = 0x186,
  VarAnd = 0x187,
  VarOr = 0x188,
  VarXor = 0x189,
  VarSetInverse = 0x18A,
  VarMod = 0x18B,
  VarEQ = 0x1C0,
  VarGE = 0x1C1,
  VarGT = 0x1C2,
  VarLE = 0x1C3,
  VarLT = 0x1C4,
  VarNE = 0x1C5,
};
};
using _RSEQCmd::RSEQCmd;

class RSEQFile : public NWFile
{
  friend class NWChunkLoader;

public:
  std::string label(int index) const;

  ISequence* sequence(ClefContext* ctx) const;

protected:
  RSEQFile(std::istream& is, const ChunkInit& init);

private:
  struct RSEQPrefix {
    std::uint8_t cmd;
    std::int16_t param1 = 0;
    std::int16_t param2 = 0;
  };
  struct RSEQEvent {
    std::uint32_t offset;
    std::vector<RSEQPrefix> prefix;
    std::uint16_t cmd;
    std::int32_t param1 = 0;
    std::int32_t param2 = 0;
    std::int32_t param3 = 0;
  };
  using RSEQTrack = std::vector<RSEQEvent>;

  friend class TrackParser;

  double tempo;
  std::vector<RSEQTrack> tracks;
  std::map<std::uint32_t, RSEQTrack> subs;

  ITrack* createTrack(const RSEQTrack& src) const;
  static int findOffset(const RSEQTrack* src, std::uint32_t offset);
};

#endif
