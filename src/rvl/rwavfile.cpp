#include "rwavfile.h"
#include "dspadpcmcodec.h"
#include "codec/sampledata.h"
#include "codec/pcmcodec.h"

RWAVFile::RWAVFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);

  auto info = section('INFO');
  format = Format(info->parseU8(0));
  looped = info->parseU8(1);
  sampleRate = info->parseU32(2) & 0x00FFFFFF;
  dataLocation.isOffset = info->parseU8(6);
  loopStart = info->parseU32(8);
  loopEnd = info->parseU32(12);
  dataLocation.pointer = info->parseU32(20);

  std::uint8_t numChannels = info->parseU8(2);
  std::uint32_t table = info->parseU32(16);
  for (int i = 0; i < numChannels; i++, table += 4) {
    std::uint32_t offset = info->parseU32(table);
    ADPCMInfo adpcm{};
    if (format == ADPCM) {
      std::uint32_t adpcmOffset = info->parseU32(offset + 4);
      for (int j = 0; j < 16; j++) {
        adpcm.coef[j] = info->parseU16(adpcmOffset + j * 2);
      }
      adpcm.gain = info->parseS16(adpcmOffset + 0x2C);
      adpcm.initialPred = info->parseU16(adpcmOffset + 0x2E);
      adpcm.history1 = info->parseS16(adpcmOffset + 0x30);
      adpcm.history2 = info->parseS16(adpcmOffset + 0x32);
      adpcm.loopPred = info->parseU16(adpcmOffset + 0x34);
      adpcm.loopHistory1 = info->parseS16(adpcmOffset + 0x36);
      adpcm.loopHistory2 = info->parseS16(adpcmOffset + 0x38);
    }

    channels.push_back({
      info->parseU32(offset),
      adpcm,
      info->parseU32(offset + 8),
      info->parseU32(offset + 12),
      info->parseU32(offset + 16),
      info->parseU32(offset + 20),
    });
  }
}

SampleData* RWAVFile::sample(std::uint64_t sampleID)
{
  int numChannels = channels.size();
  SampleData* combined = nullptr;
  NWChunk* data = section('DATA');
  std::uint64_t setSampleID = sampleID;
  for (int i = 0; i < numChannels; i++) {
    SampleData* decoded = nullptr;
    const ChannelInfo& ch = channels[i];
    if (format == ADPCM) {
      DspAdpcmCodec::Params params{
        sampleRate,
        looped ? std::int32_t(loopStart) : -1,
        loopEnd,
        1.0f, // + (ch.adpcm.gain / 32767.0f),
        { ch.adpcm.history1, ch.adpcm.history2 },
      };
      for (int i = 0; i < 16; i++) {
        params.coefs[i] = ch.adpcm.coef[i];
      }
      DspAdpcmCodec codec(ctx, params);
      std::uint32_t dataLength = loopEnd / 2;
      auto begin = data->rawData.begin() + ch.sampleOffset;
      decoded = codec.decodeRange(begin, begin + dataLength, setSampleID);
    } else {
      PcmCodec codec(ctx, format == PCM8 ? 8 : 16, 1, !isLittleEndian);
      auto begin = data->rawData.begin() + ch.sampleOffset;
      std::uint32_t dataLength = loopEnd;
      if (format == PCM16) {
        dataLength *= 2;
      }
      decoded = codec.decodeRange(begin, begin + dataLength, setSampleID);
    }
    if (!combined) {
      combined = decoded;
      setSampleID = SampleData::Uncached;
    } else {
      combined->channels.emplace_back(std::move(decoded->channels[0]));
      delete decoded;
    }
  }
  if (combined && !looped) {
    combined->loopStart = -1;
    combined->loopEnd = -1;
  }
  return combined;
}
