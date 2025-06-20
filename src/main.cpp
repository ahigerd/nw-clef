#include "clefcontext.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "seq/isequence.h"
#include "sarfile.h"
#include "rseqfile.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "lablchunk.h"
#include <fstream>

ClefContext clef;

void hexdump(const std::vector<uint8_t>& buffer, int limit);

int synth(RSEQFile* file, const std::string& name) {
  SynthContext context(&clef, 44100, 2);

  ISequence* seq = file->sequence(&clef);
  for (int i = 0; i < seq->numTracks(); i++) {
    context.addChannel(seq->getTrack(i));
  }

  RiffWriter riff(context.sampleRate, true);
  riff.open(name + ".wav");
  context.save(&riff);
  return 0;
}

int main(int argc, char** argv)
{
  std::ifstream is(argv[1]);
  std::unique_ptr<RSARFile> nw(NWChunk::load<RSARFile>(is, nullptr, &clef));
  /*
  for (auto sound : nw->info->soundDataEntries) {
    if (sound.soundType == SoundType::SEQ && sound.name.substr(0, 3) == "SEQ") {
      std::cout << "=====================" << std::endl << sound.name << std::endl;
      auto seqFile = nw->getFile(sound.fileIndex, false);
      RSEQFile* seq = NWChunk::load<RSEQFile>(seqFile);
      std::string label = seq->label(sound.seqData.labelEntry);
      if (!label.size()) continue;
      std::cout << "Label: " << label << std::endl;
      auto bank = nw->info->soundBankEntries[sound.seqData.bankIndex];
      std::cout << "Bank:  " << bank.name << std::endl;

      synth(seq, sound.name);
    }
  }
  */
  for (auto bankEntry : nw->info->soundBankEntries) {
    auto fileEntry = nw->info->fileEntries[bankEntry.fileIndex];
    std::cout << bankEntry.name << " " << fileEntry.mainSize << " / " << fileEntry.audioSize << std::endl;
    auto bankFile = nw->getFile(bankEntry.fileIndex, false);
    std::unique_ptr<RBNKFile> bank(NWChunk::load<RBNKFile>(bankFile, nullptr, &clef));
    auto audioFile = nw->getFile(bankEntry.fileIndex, true);
    std::unique_ptr<RWARFile> war(NWChunk::load<RWARFile>(audioFile, nullptr, &clef));
    int i = 0;
    while (true) {
      auto s = war->getSample(i);
      if (!s) break;
      ++i;
    }
    //bank->loadAudio(audioFile);
  }
  return 0;
}
