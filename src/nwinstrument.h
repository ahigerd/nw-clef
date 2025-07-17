#ifndef NW_NWINSTRUMENT_H
#define NW_NWINSTRUMENT_H

#include "synth/iinstrument.h"
#include "seq/sequenceevent.h"
#include "rvl/rbnkfile.h"
#include "discreteenvelope.h"
class RWARFile;
class SynthContext;

struct NWInstrument : public DefaultInstrument
{
  NWInstrument();
  NWInstrument(SynthContext* synth, const RBNKFile* bank, const RWARFile* war, int program = 0);
  NWInstrument(const NWInstrument& other) = default;
  NWInstrument& operator=(const NWInstrument& other) = default;
  NWInstrument& operator=(NWInstrument&& other) = default;

  struct TimeParam {
    TimeParam(double value = 0);

    TimeParam& operator=(double value);

    double valueAt(double time) const;

    double startLevel;
    double startTime;
    double endLevel;
    double endTime;
  };

  int trackIndex = 0;
  int program;
  double volume;
  TimeParam pan;
  TimeParam pitchBend;
  double attack, hold, decay, sustain, release;
  bool tie;

  SequenceEvent* makeEvent(double timestamp, int noteNumber, int velocity, double duration);
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event) override;
  //virtual void channelEvent(Channel* channel, std::shared_ptr<ChannelEvent> event);
  //virtual void modulatorEvent(Channel* channel, std::shared_ptr<ModulatorEvent> event);

  //virtual std::vector<int32_t> supportedChannelParams() const;

  static DiscreteEnvelope::Step attackStep(int attack, double last, double user);
  static DiscreteEnvelope::Step holdStep(double hold, double last, double user);
  static DiscreteEnvelope::Step decayStep(double decay, double sustain, double last, double user);
  static DiscreteEnvelope::Step sustainStep(double last, double user);

  static double attackValue(std::int8_t v);
  static double holdValue(std::int8_t v);
  static double decayValue(std::int8_t v);
  static double sustainValue(std::int8_t v);
  static double releaseValue(std::int8_t v);

  static double scaleVolume(std::int32_t v);
private:

  SynthContext* synth;
  const RBNKFile* bank;
  const RWARFile* war;
  std::uint64_t lastPlaybackID;
  double lastPlaybackEnd;
  //BaseOscillator* makeLFO(const LFO& lfo) const;
  //void updatePitchBends(Channel* channel, bool timestamp, bool updateRange = false, double pbrg = 0);
};

#endif
