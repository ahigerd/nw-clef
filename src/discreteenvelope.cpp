#include "discreteenvelope.h"
#include "utility.h"
#include <cmath>

DiscreteEnvelope::DiscreteEnvelope(const SynthContext* ctx, double startLevel, double startUser)
: FilterNode(ctx), lastLevel(startLevel), stepAt(0), stepVolume(startLevel),
  currentStep({ startLevel, 0, false, startUser }), step(0), allDone(false)
{
  // initializers only
}

bool DiscreteEnvelope::isActive() const
{
  return !allDone && FilterNode::isActive();
}

void DiscreteEnvelope::addPhase(PhaseFn phase)
{
  phases.push_back(phase);
}

void DiscreteEnvelope::setReleasePhase(PhaseFn phase)
{
  releasePhase = phase;
}

int16_t DiscreteEnvelope::filterSample(double time, int channel, int16_t sample)
{
  if (step > 0 && paramValue(Trigger, time) <= 0) {
    stepAt = time;
    step = -1;
    currentStep.nextVolume = lastLevel;
    currentStep.nextTime = 0;
    currentStep.finished = false;
  }
  while (true) {
    double dt = time - stepAt;
    bool shouldStep = dt >= currentStep.nextTime;
    if (shouldStep && currentStep.finished) {
      ++step;
    }
    if ((step == 0 && shouldStep && currentStep.finished) || step >= (int)phases.size()) {
      allDone = true;
      return 0;
    }
    const PhaseFn& phase = step < 0 ? releasePhase : phases[step];
    if (!phase) {
      step = 0;
      currentStep.finished = true;
      allDone = true;
      return 0;
    }
    if (shouldStep) {
      stepAt += currentStep.nextTime;
      stepVolume = currentStep.nextVolume;
      currentStep = phase(stepVolume, currentStep.userData);
      continue;
    }
    lastLevel = lerp(stepVolume, currentStep.nextVolume, dt / currentStep.nextTime);
    return lastLevel * sample;
  }
}
