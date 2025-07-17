#include "nwinstrument.h"
#include "rvl/rwarfile.h"
#include "clefcontext.h"
#include "utility.h"
#include "codec/sampledata.h"
#include "synth/sampler.h"
#include "synth/synthcontext.h"
#include <iomanip>

#define PARAM(name) ((name >= 0) ? name : info->name / 127.0)
static constexpr double SDAT_TICK = 64.0 * 2728.0 / 33513982;
static constexpr double RSEQ_TICK = 1.0 / 192.0;
static constexpr double SDAT_RES = 723;
static constexpr double SDAT_SCALE = SDAT_RES * 128;

enum ParamIndexes {
  I_SampleID,
  F_PitchBend = 0,
};

NWInstrument::TimeParam::TimeParam(double value)
: startLevel(value), startTime(0), endLevel(value), endTime(0)
{
  // initializers only
}

NWInstrument::TimeParam& NWInstrument::TimeParam::operator=(double value)
{
  startLevel = endLevel = value;
  startTime = endTime = 0;
  return *this;
}

double NWInstrument::TimeParam::valueAt(double time) const
{
  if (time <= startTime) {
    return startLevel;
  } else if (time >= endTime) {
    return endLevel;
  } else {
    return lerp(startLevel, endLevel, (time - startTime) / (endTime - startTime));
  }
}

NWInstrument::NWInstrument()
: synth(nullptr), bank(nullptr), war(nullptr), lastPlaybackID(0), lastPlaybackEnd(-1)
{
  // initializers only
}

NWInstrument::NWInstrument(SynthContext* synth, const RBNKFile* bank, const RWARFile* war, int program)
: program(program),
  volume(-1),
  pan(-1),
  pitchBend(0),
  attack(-1),
  hold(-1),
  decay(-1),
  sustain(-1),
  release(-1),
  tie(false),
  synth(synth),
  bank(bank),
  war(war),
  lastPlaybackID(0),
  lastPlaybackEnd(-1)
{
  // initializers only
}

SequenceEvent* NWInstrument::makeEvent(double timestamp, int noteNumber, int velocity, double duration)
{
  auto info = bank->getSample(program, noteNumber, velocity);
  if (!info) {
    return nullptr;
  }

  SampleData* sample = bank->ctx->getSample(info->wave.pointer);
  if (!sample) {
    sample = war->getSample(info->wave.pointer);
    if (!sample) {
      return nullptr;
    }
  }

  if (tie && lastPlaybackEnd >= timestamp) {
    NoteUpdateEvent* event = new NoteUpdateEvent(lastPlaybackID);
    event->params[Sampler::Pitch] = semitonesToFactor(noteNumber - info->baseNote);
    event->params[AudioNode::Gain] = velocity / 127.0;
    event->newDuration = duration;
    event->timestamp = timestamp;
    lastPlaybackEnd = timestamp + duration + .001;
    return event;
  }

  InstrumentNoteEvent* event = new InstrumentNoteEvent;
  event->timestamp = timestamp;
  event->duration = duration;
  event->pitch = semitonesToFactor(noteNumber - info->baseNote);
  event->intParams.push_back(sample->sampleID);
  event->floatParams.push_back(semitonesToFactor(pitchBend.valueAt(timestamp)));
  event->volume = (velocity / 127.0);

  double a = attack < 0 ? attackValue(info->attack) : attack;
  double h = hold < 0 ? holdValue(info->hold) : hold;
  double d = decay < 0 ? decayValue(info->decay) : decay;
  double s = sustain < 0 ? sustainValue(info->sustain) : sustain;
  double r = release < 0 ? releaseValue(info->release) : release;
  if (a < 0) a = attackValue(0);
  if (h < 0) h = holdValue(0);
  if (d < 0) d = decayValue(0);
  if (s < 0) s = sustainValue(1);
  if (r < 0) r = releaseValue(0);
  event->setEnvelope(a, h, d, s, r);

  if (velocity > 0) {
    lastPlaybackID = event->playbackID;
    if (duration > 0) {
      lastPlaybackEnd = timestamp + duration + .001;
    } else {
      lastPlaybackEnd = HUGE_VAL;
    }
  }

  return event;
}

Channel::Note* NWInstrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  auto noteEvent = InstrumentNoteEvent::castShared(event);
  if (!noteEvent) {
    return DefaultInstrument::noteEvent(channel, event);
  }

  SampleData* sampleData = bank->ctx->getSample(noteEvent->intParams[I_SampleID]);
  double pitchBend = noteEvent->floatParams[F_PitchBend];
  double duration = event->duration;

  Sampler* samp = new Sampler(channel->ctx, sampleData, noteEvent->pitch, pitchBend);
  samp->param(AudioNode::Gain)->setConstant(noteEvent->volume);
  samp->param(AudioNode::Pan)->setConstant(noteEvent->pan);
  if (!duration) {
    duration = sampleData->duration();
  }

  int attack = int(event->attack);
  DiscreteEnvelope::Step startGain = attackStep(attack, 0, -SDAT_SCALE);
  DiscreteEnvelope* env = new DiscreteEnvelope(channel->ctx, startGain.nextVolume, startGain.userData);

  if (event->attack < 127) {
    env->addPhase([attack](double last, double user) { return attackStep(attack, last, user); });
  }

  if (event->hold > 0) {
    double hold = event->hold;
    env->addPhase([hold](double last, double user) { return holdStep(hold, last, user); });
  }

  double sustain = event->sustain;
  if (event->sustain < 127) {
    double decay = event->decay;
    env->addPhase([decay, sustain](double last, double user) { return decayStep(decay, sustain, last, user); });
  }
  env->addPhase(sustainStep);

  double release = event->release;
  env->setReleasePhase([release](double last, double user) { return decayStep(release, -SDAT_SCALE, last, user); });

  env->connect(std::shared_ptr<AudioNode>(samp), true);

  return channel->allocNote(event, env, duration);
}

double NWInstrument::scaleVolume(int v)
{
  constexpr double base = std::log(4096) / SDAT_SCALE;
  if (v <= -SDAT_SCALE) {
    return 0;
  }
  return fastExp(base * v);
}

DiscreteEnvelope::Step NWInstrument::attackStep(int attack, double last, double user)
{
  int32_t v = -((-int(user) * attack) >> 8);

  double volume = scaleVolume(v);
  return { volume, SDAT_TICK, v == 0, double(v) };
}

DiscreteEnvelope::Step NWInstrument::holdStep(double hold, double last, double user)
{
  // TODO: units
  return { last, hold, true, user };
}

DiscreteEnvelope::Step NWInstrument::decayStep(double decay, double sustain, double last, double user)
{
  double next = user - decay;
  bool finished = (next <= sustain);
  if (finished) {
    next = sustain;
  }
  return { scaleVolume(next), SDAT_TICK, finished, next };
}

DiscreteEnvelope::Step NWInstrument::sustainStep(double last, double user)
{
  return { last, HUGE_VAL, false, user };
}

double NWInstrument::attackValue(std::int8_t v)
{
  constexpr std::uint8_t lut[] = {
    0x00, 0x01, 0x05, 0x0E, 0x1A, 0x26, 0x33, 0x3F, 0x49, 0x54,
    0x5C, 0x64, 0x6D, 0x74, 0x7B, 0x7F, 0x84, 0x89, 0x8F,
  };
  if (v > 127) {
    return 0;
  } else if (v > 0x06d) {
    return lut[0x7f - v];
  } else {
    return 0xff - v;
  }
}

double NWInstrument::holdValue(std::int8_t v)
{
  // TODO: dummy implementation
  return v * SDAT_TICK;
}

double NWInstrument::decayValue(std::int8_t v)
{
  if (v == 127) {
    return 65535;
  } else if (v == 126) {
    return 15360;
  } else if (v >= 50) {
    return 7680 / (126 - v);
  } else {
    return v * 2 + 1;
  }
}

double NWInstrument::sustainValue(std::int8_t v)
{
  if (v == 0) {
    return -32768;
  } else if (v == 1) {
    return -722;
  } else {
    return int(173.7255 * std::log(double(v)) - 842);
  }
}

double NWInstrument::releaseValue(std::int8_t v)
{
  return decayValue(v);
}
