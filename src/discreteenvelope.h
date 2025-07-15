#ifndef NW_DISCREENVELOPE_H
#define NW_DISCREENVELOPE_H

#include "synth/audionode.h"
#include "synth/audioparam.h"
#include <functional>

class DiscreteEnvelope : public FilterNode
{
public:
  struct Step {
    double nextVolume = 0;
    double nextTime = 0;
    bool finished = false;
    double userData = 0;
  };
  using PhaseFn = std::function<Step(double, double)>;

  DiscreteEnvelope(const SynthContext* ctx, double startLevel = 0.0, double startUser = 0.0);

  virtual bool isActive() const;

  void addPhase(PhaseFn phase);
  void setReleasePhase(PhaseFn phase);

  virtual int16_t filterSample(double time, int channel, int16_t sample);
protected:

  double lastLevel;
  double stepAt, stepVolume;
  Step currentStep;
  int step;
  std::vector<PhaseFn> phases;
  PhaseFn releasePhase;
};

#endif

