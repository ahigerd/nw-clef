#include "clefcontext.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "seq/isequence.h"
#include "rvl/rsarfile.h"
#include "rvl/rseqfile.h"
#include "rvl/rbnkfile.h"
#include "rvl/rwarfile.h"
#include "rvl/infochunk.h"
#include "rvl/rseqcsv.h"
#include "riffwriter.h"
#include "commandargs.h"
#include "listactions.h"
#include <sstream>
#include <fstream>
#include <filesystem>

int synth(RSEQFile* file, const std::string& filename) {
  file->ctx->purgeSamples();
  SynthContext context(file->ctx, 44100, 2);

  ISequence* seq = file->sequence();
  for (int i = 0; i < seq->numTracks(); i++) {
    context.addChannel(seq->getTrack(i));
  }

  RiffWriter riff(context.sampleRate, true);
  if (!riff.open(filename)) {
    std::cerr << "Unable to open \"" << filename << "\" for writing" << std::endl;
    return 1;
  }
  std::cout << "Writing " << seq->duration() << " seconds to " << filename << "..." << std::endl;
  context.save(&riff);
  return 0;
}

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help",     "h", "", "Show this help text" },
    { "output",   "o", "dir", "Specify the path for saved files (default output/)" },
    { "out-file", "O", "filename", "Only output a single file at the specified path" },
    { "list",     "l", "type", "List entries of the specified type (see below)" },
    { "filter",   "f", "pattern", "Only consider results matching pattern" },
    { "csv",      "c", "", "Generate CSV file(s) for sequences" },
    { "seq",      "s", "pattern", "Read sequence(s) matching pattern" },
    { "verbose",  "v", "", "Include additional information" },
    { "",         "",  "input", "Path(s) to the input file(s)" },
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
#ifndef _WIN32
    std::cerr << std::endl;
    std::cerr << "When used with --synth, using \"--out-file=-\" or \"-O -\" will send the rendered output to" << std::endl;
    std::cerr << "stdout, suitable for piping into another program for playback or transcoding." << std::endl;
#endif
    return 0;
  }

  if (args.hasKey("list")) {
    return listMembers(args);
  }

  if (args.hasKey("output") && args.hasKey("out-file")) {
    std::cerr << argv[0] << ": --output and --out-file cannot be used at the same time" << std::endl;
    return 1;
  }

  Glob glob(args.getString("filter"));
  std::string outPath;
  bool singleFile = args.hasKey("out-file");
  if (singleFile) {
    outPath = args.getString("out-file");
#ifndef _WIN32
    if (outPath == "-") {
      outPath = "/dev/stdout";
    }
#endif
  } else {
    outPath = args.getString("output", "./output/");
    std::filesystem::path fsPath(outPath);
    std::filesystem::create_directories(outPath);
  }
  std::string extension = ".wav";
  if (args.hasKey("csv")) {
    RSEQTrack::parseVerbose = args.hasKey("verbose");
    extension = ".csv";
  }

  for (const std::string& filename : args.positional()) {
    ClefContext clef;
    std::ifstream is(filename, std::ios::in | std::ios::binary);
    std::unique_ptr<RSARFile> nw(NWChunk::load<RSARFile>(is, nullptr, &clef));
    bool didSomething = false;
    for (auto sound : nw->info->soundDataEntries) {
      if (sound.soundType != SoundType::SEQ || !glob.match(sound.name)) {
        continue;
      }
      auto seqFile = nw->getFile(sound.fileIndex, false);
      RSEQFile* seq = NWChunk::load<RSEQFile>(seqFile, nullptr, &clef);

      auto bankEntry = nw->info->soundBankEntries[sound.seqData.bankIndex];
      auto bankFile = nw->getFile(bankEntry.fileIndex, false);
      std::unique_ptr<RBNKFile> bank(NWChunk::load<RBNKFile>(bankFile, nullptr, &clef));
      auto audioFile = nw->getFile(bankEntry.fileIndex, true);
      std::unique_ptr<RWARFile> war(NWChunk::load<RWARFile>(audioFile, nullptr, &clef));
      seq->loadBank(bank.get(), war.get());

      int err;
      std::string outFilename = outPath;
      if (singleFile) {
        if (didSomething) {
          std::cerr << "Can only process one sequence when using --out-file" << std::endl;
          return 1;
        }
      } else {
        outFilename += "/" + sound.name + extension;
      }
      if (args.hasKey("csv")) {
        err = generateCsv(seq, outFilename);
      } else {
        err = synth(seq, outFilename);
      }
      if (err) {
        return err;
      }
      didSomething = true;
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
