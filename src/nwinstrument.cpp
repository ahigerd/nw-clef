#include "nwinstrument.h"
#include "rwarfile.h"
#include "clefcontext.h"
#include "utility.h"
#include "codec/sampledata.h"

#define PARAM(name) ((name >= 0) ? name : info->name / 127.0)

NWInstrument::NWInstrument(const RBNKFile* bank, const RWARFile* war)
: program(0),
  volume(-1),
  pan(-1),
  pitchBend(1),
  attack(-1),
  hold(-1),
  decay(-1),
  sustain(-1),
  release(-1),
  bank(bank),
  war(war)
{
  // initializers only
}

BaseNoteEvent* NWInstrument::makeEvent(int noteNumber, int velocity, double duration) const
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
  event->duration = duration;
  event->sampleID = sample->sampleID;
  event->pitchBend = semitonesToFactor(noteNumber - info->baseNote);
  event->modPitchBend + semitonesToFactor(pitchBend);
  event->volume = (velocity / 127.0) * PARAM(volume);
  event->pan = PARAM(pan);
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
