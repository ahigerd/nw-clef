#ifndef NW_RSEQTRACK_H
#define NW_RSEQTRACK_H

#include <ostream>
#include "seqtrack.h"
#include "metaenum.h"

class RSEQFile;

META_ENUM(RSEQCmd,
  Rest = 0x80,
  ProgramChange = 0x81, // 7 bits program, 7 bits bank
  AddTrack = 0x88,
  Goto = 0x89,
  Gosub = 0x8A,
  SSEQ_AddTrack = 0x93,
  SSEQ_Goto = 0x94,
  SSEQ_Gosub = 0x95,
  PrefixRand = 0xA0,
  PrefixVar = 0xA1,
  PrefixIf = 0xA2,
  PrefixTime = 0xA3,
  PrefixTimeRand = 0xA4,
  PrefixTimeVar = 0xA5,
  Ppqn = 0xB0,
  Hold = 0xB1,
  Mono = 0xB2,
  VelocityRange = 0xB3,
  BiquadType = 0xB4, // none, LPF, HPF, BPF 512, BPF 1024, BPF 2048
  BiquadValue = 0xB5,
  BankSelect = 0xB6,
  ModPhase = 0xBD,
  ModCurve = 0xBE,
  FrontBypass = 0xBF,
  Pan = 0xC0,
  Volume = 0xC1,
  MainVolume = 0xC2,
  Transpose = 0xC3,
  Bend = 0xC4,
  BendRange = 0xC5,
  Priority = 0xC6,
  WaitEnable = 0xC7,
  Tie = 0xC8,
  Portamento = 0xC9, // param = start key
  ModDepth = 0xCA,
  ModSpeed = 0xCB,
  ModType = 0xCC,
  ModRange = 0xCD,
  PortaEnable = 0xCE,
  PortaTime = 0xCF,
  Attack = 0xD0,
  Decay = 0xD1,
  Sustain = 0xD2,
  Release = 0xD3,
  LoopStart = 0xD4, // 0 = inf
  Expression = 0xD5,
  DebugPrint = 0xD6,
  Surround = 0xD7, // CSEQ: Span?
  Cutoff = 0xD8, // 0 = max, 64 = none
  SendA = 0xD9,
  SendB = 0xDA,
  MainSend = 0xDB,
  InitialPan = 0xDC,
  Mute = 0xDD, // 0 = off, 1 = mute future notes, 2 = mute after release, 3 = mute now
  SendC = 0xDE,
  Damper = 0xDF,
  ModDelay = 0xE0,
  Tempo = 0xE1,
  Sweep = 0xE3, // start detuned (64 = 1 semitone) and use PortaTime to get to pitch
  ModPeriod = 0xE4,
  Extended = 0xF0,
  EnvReset = 0xFB,
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
  VarEQ = 0x190,
  VarGE = 0x191,
  VarGT = 0x192,
  VarLE = 0x193,
  VarLT = 0x194,
  VarNE = 0x195,
  Mod2Curve = 0x1A0,
  Mod2Phase = 0x1A1,
  Mod2Depth = 0x1A2,
  Mod2Speed = 0x1A3,
  Mod2Type = 0x1A4,
  Mod2Range = 0x1A5,
  Mod3Curve = 0x1A6,
  Mod3Phase = 0x1A7,
  Mod3Depth = 0x1A8,
  Mod3Speed = 0x1A9,
  Mod3Type = 0x1AA,
  Mod3Range = 0x1AB,
  Mod4Curve = 0x1AC,
  Mod4Phase = 0x1AD,
  Mod4Depth = 0x1AE,
  Mod4Speed = 0x1AF,
  Mod4Type = 0x1B0,
  Mod4Range = 0x1B1,
  UserProc = 0x1E0,
  Mod2Delay = 0x1E1,
  Mod2Period = 0x1E2,
  Mod3Delay = 0x1E3,
  Mod3Period = 0x1E4,
  Mod4Delay = 0x1E5,
  Mod4Period = 0x1E6,
);

class RSEQTrack : public SEQTrack {
public:
  static bool parseVerbose;

  struct RSEQPrefix {
    std::uint8_t cmd;
    std::int16_t param1 = 0;
    std::int16_t param2 = 0;
  };

  struct RSEQEvent {
    std::uint32_t offset;
    std::uint32_t timestamp;
    std::vector<RSEQPrefix> prefix;
    std::uint16_t cmd;
    std::int32_t param1 = 0;
    std::int32_t param2 = 0;
    std::int32_t param3 = 0;

    std::string format() const;
  };

  RSEQTrack(RSEQFile* file, NWChunk* chunk, int trackIndex);

  std::vector<RSEQEvent> events;

  void parse(std::uint32_t offset);
  void addEvent(const RSEQEvent& event);
  int findEvent(std::uint32_t offset) const;

protected:
  virtual SequenceEvent* translateEvent(std::int32_t& index, int loopCount) override;

private:
  RSEQEvent readEvent();

  RSEQFile* file;
  std::uint32_t tickPos;
  bool noteWait;
  bool didInitInstrument;
};

std::ostream& operator<<(std::ostream& os, const RSEQTrack::RSEQEvent& event);

#endif
