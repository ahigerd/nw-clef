#include "nwinstrument.h"
#include "rvl/rwarfile.h"
#include "clefcontext.h"
#include "utility.h"
#include "codec/sampledata.h"
#include "synth/sampler.h"
#include <iomanip>

#define PARAM(name) ((name >= 0) ? name : info->name / 127.0)

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
    // std::cerr << int(noteNumber) << " tie " << pitchBend.valueAt(timestamp) << std::endl;
    NoteUpdateEvent* event = new NoteUpdateEvent(lastPlaybackID);
    event->params[Sampler::Pitch] = semitonesToFactor(noteNumber - info->baseNote);
    event->params[AudioNode::Gain] = velocity / 127.0;
    event->newDuration = duration;
    event->timestamp = timestamp;
    lastPlaybackEnd = timestamp + duration + .001;
    return event;
  }

  SampleEvent* event = new SampleEvent();
  event->timestamp = timestamp;
  event->duration = duration;
  event->sampleID = sample->sampleID;
  event->pitchBend = semitonesToFactor(noteNumber - info->baseNote);
  event->modPitchBend = semitonesToFactor(pitchBend.valueAt(timestamp));
  event->volume = (velocity / 127.0);

  double a = attack < 0 ? attackValue(info->attack) : attack;
  double h = hold < 0 ? holdValue(info->hold) : hold;
  double d = decay < 0 ? decayValue(info->decay) : decay;
  double s = sustain < 0 ? sustainValue(info->sustain) : sustain;
  double r = release < 0 ? releaseValue(info->release) : release;
  if (a < 0) a = 0;
  if (h < 0) h = 0;
  if (d < 0) d = 0;
  if (s < 0) s = 1;
  if (r < 0) r = 0;
  d = d * (1 - s);
  r = r * s;
  // std::cerr << int(noteNumber) << " ENV: " << a << " " << h << " " << d << " " << s << " " << r << "\t" << pitchBend.valueAt(timestamp) << std::endl;
  // TODO: units
  event->setEnvelope(a, h, d, s, r);
  //event->setEnvelope(1.0 - PARAM(attack), 1.0 - PARAM(hold), PARAM(decay), PARAM(sustain), 1.0 - PARAM(release));
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

/*
Channel::Note* NWInstrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  auto noteEvent = InstrumentNoteEvent::castShared(event);
  if (!noteEvent) {
    return DefaultInstrument::noteEvent(channel, event);
  }

  auto sd = bank->getSample(program, event->pitch, event->volume);
  if (!sd) {
    return nullptr;
  }

  SampleEvent* newEvent = new SampleEvent();
  newEvent->duration = event->duration;
  newEvent->volume = (volume < 0 ?


  return nullptr;
}
*/

double NWInstrument::attackValue(std::int8_t v)
{
  // TODO: values over 0x6D scale differently
  return 1 - (v / 127.0);
}

double NWInstrument::holdValue(std::int8_t v)
{
  // TODO: dummy implementation
  return 1 - attackValue(v);
}

double NWInstrument::decayValue(std::int8_t v)
{
  if (v < 0) {
    return -1;
  } else if (v >= 0x7F) {
    return 0.001;
  } else if (v == 0x7E) {
    return 482.0 / 15360.0;
  } else if (v >= 0x32) {
    return 482.0 / (7680.0 / (0x7E - v));
  } else {
    return 482.0 / (v * 2 + 1);
  }
}

double NWInstrument::sustainValue(std::int8_t v)
{
  // TODO: dummy implementation;
  return v / 127.0;
}

double NWInstrument::releaseValue(std::int8_t v)
{
  return decayValue(v);
}
