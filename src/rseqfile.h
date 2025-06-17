#ifndef NW_RSEQFILE_H
#define NW_RSEQFILE_H

#include "nwfile.h"

namespace _RSEQCmd {
enum RSEQCmd {
  Rest = 0x80,
  ProgramChange = 0x81,
  TrackStart = 0x88,
  Goto = 0x89,
  Gosub = 0x8A,
  Ppqn = 0x8B,
  Hold = 0xB1,
  Mono = 0xB2,
  VelocityRange = 0xB3,
  Pan = 0xC0,
  Volume = 0xC1,
  MainVolume = 0xC2,
  Transpose = 0xC3,
  Bend = 0xC4,
  BendRange = 0xC5,
  Priority = 0xC6,
  Wait = 0xC7,
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
};
};
using _RSEQCmd::RSEQCmd;

class RSEQFile : public NWFile
{
  friend class NWChunkLoader;

protected:
  RSEQFile(std::istream& is, const ChunkInit& init);
};

#endif
