#include "nwinstrument.h"
#include "rvl/rwarfile.h"
#include "clefcontext.h"
#include "utility.h"
#include "codec/sampledata.h"
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
: synth(nullptr), bank(nullptr), war(nullptr)
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
  war(war)
{
  // initializers only
}

BaseNoteEvent* NWInstrument::makeEvent(double timestamp, int noteNumber, int velocity, double duration) const
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

  SampleEvent* event = new SampleEvent();
  event->timestamp = timestamp;
  event->duration = duration;
  event->sampleID = sample->sampleID;
  event->pitchBend = semitonesToFactor(noteNumber - info->baseNote);
  event->modPitchBend = semitonesToFactor(pitchBend.valueAt(timestamp));
  event->volume = (velocity / 127.0); // * PARAM(volume);
  //double pan = this->pan.valueAt(timestamp);
  //event->pan = PARAM(pan);
  //std::cerr << std::setw(8) << event->timestamp << "\t" << std::setw(8) << event->pan << std::endl;
  // TODO: units
  event->setEnvelope(1.0 - PARAM(attack), 1.0 - PARAM(hold), PARAM(decay), PARAM(sustain), 1.0 - PARAM(release));

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
