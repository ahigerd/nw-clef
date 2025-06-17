#include "clefcontext.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "nwfile.h"
#include <fstream>

int main(int argc, char** argv)
{
  std::ifstream is(argv[1]);
  NWFile& nw = *NWChunk::load<NWFile>(is);
  for (auto sound : nw.info->soundDataEntries) {
    if (sound.name == "SEQ_BGM_YUKI_V") {
      auto seqFile = nw.getFile(sound.fileIndex, false);
      std::vector<uint8_t> buffer(0x20);
      seqFile.read((char*)buffer.data(), 0x20);
      seqFile.seekg(0);
      NWFile* seq = NWChunk::load<NWFile>(seqFile);
      break;
    }
  }
  return 0;

  // This sample main() function does nothing but
  // generate a valid but empty wave file.
  ClefContext clef;
  SynthContext context(&clef, 44100, 2);
  int sampleLength = 0;
  RiffWriter riff(context.sampleRate, true, sampleLength * 2);
  riff.open("output.wav");
  context.save(&riff);
  return 0;
}
