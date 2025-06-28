#include "clefcontext.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "seq/isequence.h"
#include "rvl/rsarfile.h"
#include "rvl/rseqfile.h"
#include "rvl/rbnkfile.h"
#include "rvl/rwarfile.h"
#include "rvl/infochunk.h"
#include "riffwriter.h"
#include "commandargs.h"
#include "listactions.h"
#include <sstream>
#include <fstream>

int synth(RSEQFile* file, const std::string& name, const std::string& outPath) {
  file->ctx->purgeSamples();
  SynthContext context(file->ctx, 44100, 2);

  ISequence* seq = file->sequence();
  for (int i = 0; i < seq->numTracks(); i++) {
    context.addChannel(seq->getTrack(i));
  }

  RiffWriter riff(context.sampleRate, true);
  riff.open(outPath + "/" + name + ".wav");
  context.save(&riff);
  return 0;
}

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "output", "o", "dir", "Specify the path for saved files (default output/)" },
    { "list", "l", "type", "List entries of the specified type (see below)" },
    { "filter", "f", "pattern", "Only consider results matching pattern" },
    { "seq", "s", "pattern", "Read sequence(s) matching pattern" },
    { "", "", "input", "Path(s) to the input file(s)" },
  });

  std::string argError = args.parse(argc, argv);
  if (!argError.empty()) {
    std::cerr << argError << std::endl;
    return 1;
  }

  if (args.hasKey("help") || !args.positional().size()) {
    std::cout << args.usageText(argv[0]) << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options for --list:" << std::endl;
    std::cerr << "    seq     List sequences" << std::endl;
    std::cerr << "    strm    List streams" << std::endl;
    std::cerr << "    wave    List named waves" << std::endl;
    std::cerr << "    bank    List banks" << std::endl;
    std::cerr << "    file    List named files" << std::endl;
    std::cerr << "    group   List groups" << std::endl;
    std::cerr << "    player  List players" << std::endl;
    std::cerr << "    label   List labels on sequence" << std::endl;
    /*
#ifndef _WIN32
    std::cerr << std::endl;
    std::cerr << "When used with --synth, specifying - as an output path will send the rendered output" << std::endl;
    std::cerr << "to stdout, suitable for piping into another program for playback or transcoding." << std::endl;
#endif
    */
    return 0;
  }

  if (args.hasKey("list")) {
    return listMembers(args);
  }

  Glob glob(args.getString("filter"));
  for (const std::string& filename : args.positional()) {
    ClefContext clef;
    std::ifstream is(filename, std::ios::in | std::ios::binary);
    std::unique_ptr<RSARFile> nw(NWChunk::load<RSARFile>(is, nullptr, &clef));
    bool didSomething = false;
    for (auto sound : nw->info->soundDataEntries) {
      if (sound.soundType != SoundType::SEQ || !glob.match(sound.name)) {
        continue;
      }
      didSomething = true;
      auto seqFile = nw->getFile(sound.fileIndex, false);
      RSEQFile* seq = NWChunk::load<RSEQFile>(seqFile, nullptr, &clef);

      auto bankEntry = nw->info->soundBankEntries[sound.seqData.bankIndex];
      auto bankFile = nw->getFile(bankEntry.fileIndex, false);
      std::unique_ptr<RBNKFile> bank(NWChunk::load<RBNKFile>(bankFile, nullptr, &clef));
      auto audioFile = nw->getFile(bankEntry.fileIndex, true);
      std::unique_ptr<RWARFile> war(NWChunk::load<RWARFile>(audioFile, nullptr, &clef));
      seq->loadBank(bank.get(), war.get());

      synth(seq, sound.name, args.getString("output", "./output"));
    }
    if (!didSomething) {
      if (args.hasKey("filter")) {
        std::cerr << "Nothing found matching \"" << args.getString("filter") << "\" in " << filename << std::endl;
      } else {
        std::cerr << "No sequences found in " << filename << std::endl;
      }
    }
  }
  return 0;
}
