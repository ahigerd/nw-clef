#include "dspadpcmcodec.h"
#include "utility.h"

static std::int8_t signedNibble(std::uint8_t byte, bool low)
{
  if (low) {
    byte <<= 4;
  }
  return std::int8_t(byte) >> 4;
}

DspAdpcmCodec::DspAdpcmCodec(ClefContext* ctx, const DspAdpcmCodec::Params& params)
: ICodec(ctx), params(params)
{
  // initializers only
}

static inline std::uint32_t convertSampleCount(std::int32_t n)
{
  n = (n / 16 * 14) + (n % 16) - 2;
  return (n < 0) ? 0 : n;
}

SampleData* DspAdpcmCodec::decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID)
{
  SampleData* sample = new SampleData(context(), sampleID);
  sample->sampleRate = params.sampleRate;
  sample->loopStart = convertSampleCount(params.loopStart);
  sample->loopEnd = convertSampleCount(params.loopEnd);
  sample->channels.emplace_back();
  auto& buffer = sample->channels[0];
  buffer.reserve(sample->loopEnd);

  auto iter = start;
  while (iter != end) {
    // frame header
    scale = 1 << (*iter & 0x0F);
    int index = (*iter >> 4) * 2;
    coef1 = params.coefs[index];
    coef2 = params.coefs[index + 1];
    ++iter;

    for (int i = 0; i < 7 && iter != end; i++) {
      std::int8_t high = signedNibble(*iter, false);
      std::int8_t low = signedNibble(*iter, true);

      buffer.push_back(getNextSample(high));
      buffer.push_back(getNextSample(low));

      ++iter;
    }
  }

  return sample;
}

std::int16_t DspAdpcmCodec::getNextSample(std::int32_t state)
{
  std::int32_t c1 = coef1 * params.history[0];
  std::int32_t c2 = coef2 * params.history[1];
  std::int32_t sample = (state * scale) << 11;
  sample = (sample + 1024 + c1 + c2) >> 11;
  std::int16_t result = clamp<std::int16_t>(sample, -0x8000, 0x7FFF);
  params.history[1] = params.history[0];
  params.history[0] = result;
  return result * params.gain;
}
