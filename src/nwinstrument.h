#ifndef NW_NWINSTRUMENT_H
#define NW_NWINSTRUMENT_H

#include "synth/iinstrument.h"
#include "seq/sequenceevent.h"
#include "rvl/rbnkfile.h"
class RWARFile;

struct NWInstrument : public DefaultInstrument
{
  NWInstrument();
  NWInstrument(const RBNKFile* bank, const RWARFile* war);
  NWInstrument(const NWInstrument& other) = default;
  NWInstrument& operator=(const NWInstrument& other) = default;
  NWInstrument& operator=(NWInstrument&& other) = default;

  int program;
  double volume, pan, pitchBend;
  double attack, hold, decay, sustain, release;

  BaseNoteEvent* makeEvent(int noteNumber, int velocity, double duration) const;
  //virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
  //virtual void channelEvent(Channel* channel, std::shared_ptr<ChannelEvent> event);
  //virtual void modulatorEvent(Channel* channel, std::shared_ptr<ModulatorEvent> event);

  //virtual std::vector<int32_t> supportedChannelParams() const;

private:
  const RBNKFile* bank;
  const RWARFile* war;
  //BaseOscillator* makeLFO(const LFO& lfo) const;
  //void updatePitchBends(Channel* channel, bool timestamp, bool updateRange = false, double pbrg = 0);
};

#endif
