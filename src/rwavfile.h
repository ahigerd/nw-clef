#ifndef NW_RWAVFILE_H
#define NW_RWAVFILE_H

#include "nwfile.h"

class SampleData;

class RWAVFile : public NWFile
{
  friend class NWChunkLoader;

protected:
  RWAVFile(std::istream& is, const ChunkInit& init);

public:
  SampleData* sample(std::uint64_t sampleID);

  enum Format {
    PCM8,
    PCM16,
    ADPCM,
  };
  Format format;
  bool looped;
  std::uint32_t sampleRate;
  std::uint32_t loopStart;
  std::uint32_t loopEnd;
  DataRef dataLocation;

  struct ADPCMInfo {
    std::uint16_t coef[16];
    std::uint16_t gain;
    std::uint16_t initialPred;
    std::uint16_t history1;
    std::uint16_t history2;
    std::uint16_t loopPred;
    std::uint16_t loopHistory1;
    std::uint16_t loopHistory2;
  };

  struct ChannelInfo {
    std::uint32_t sampleOffset;
    ADPCMInfo adpcm;
    std::uint32_t leftVolume;
    std::uint32_t rightVolume;
    std::uint32_t surroundLeftVolume;
    std::uint32_t surroundRightVolume;
  };
  std::vector<ChannelInfo> channels;
};

#endif
