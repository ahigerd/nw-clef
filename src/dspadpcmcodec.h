#ifndef NW_DSPADPCMCODEC_H
#define NW_DSPADPCMCODEC_H

#include "codec/icodec.h"
#include "codec/sampledata.h"

// TODO: interleaved sterep
class DspAdpcmCodec : public ICodec
{
public:
  struct Params {
    std::uint32_t sampleRate;
    std::int32_t loopStart;
    std::uint32_t loopEnd;
    float gain;
    std::uint16_t history[2];
    std::uint16_t coefs[16];
  };

  DspAdpcmCodec(ClefContext* ctx, const Params& params);

  virtual SampleData* decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID = SampleData::Uncached);

private:
  std::int16_t getNextSample(std::int8_t state);

  Params params;
  std::int32_t scale, coef1, coef2;
};

#endif
