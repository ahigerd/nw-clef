#include "listactions.h"
#include "commandargs.h"
#include "clefcontext.h"
#include "rvl/rsarfile.h"
#include "rvl/rwarfile.h"
#include "rvl/rwavfile.h"
#include "rvl/rseqfile.h"
#include "rvl/infochunk.h"
#include <iostream>
#include <fstream>
#include <sstream>

Glob::Glob(const std::string& pattern)
{
  if (!pattern.size()) {
    return;
  }

  std::string section;
  for (char ch : pattern) {
    if (ch == '*') {
      sections.push_back(section);
      section.clear();
    } else {
      section += ch;
    }
  }
  sections.push_back(section);
}

bool Glob::match(const std::string& subject) const
{
  int numSections = sections.size();
  if (!numSections) {
    return true;
  }
  int lastPos = 0;
  for (int i = 0; i < numSections; i++) {
    if (!sections[i].size()) {
      continue;
    }
    int pos = subject.find(sections[i], lastPos);
    if (pos == std::string::npos) {
      return false;
    }
    if (i == 0 && pos != 0) {
      return false;
    }
    lastPos = pos + sections[i].size();
    if (i == numSections - 1 && lastPos != subject.size()) {
      return false;
    }
  }
  return true;
}

struct RSARContext {
  ClefContext clef;
  std::unique_ptr<RSARFile> nw;
};

static RSARContext loadRSAR(const std::string& filename)
{
  ClefContext clef;
  std::ifstream is(filename, std::ios::in | std::ios::binary);
  if (!is) {
    std::cerr << "Unable to open file " << filename << std::endl;
    return {};
  }
  std::unique_ptr<RSARFile> nw(NWChunk::load<RSARFile>(is, nullptr, &clef));
  if (!nw) {
    std::cerr << "Unable to load file " << filename << std::endl;
    return {};
  }
  return { std::move(clef), std::move(nw) };
}

struct ListResult
{
  ListResult(const std::string& listType, const std::string& listTypeLC, bool namedOnly = false)
  : listType(listType), listTypeLC(listTypeLC), namedOnly(namedOnly) {}
  ListResult(ListResult&& other) = default;

  std::string listType;
  std::string listTypeLC;
  bool namedOnly;
  std::vector<std::string> matches;
};

static ListResult listBySoundType(RSARFile* nw, const Glob& glob, SoundType soundType)
{
  static const std::map<SoundType, std::string> typeNames = {
    { SoundType::SEQ, "Sequences" },
    { SoundType::STRM, "Streams" },
    { SoundType::WAVE, "Waves" },
  };
  static const std::map<SoundType, std::string> typeNamesLC = {
    { SoundType::SEQ, "sequences" },
    { SoundType::STRM, "streams" },
    { SoundType::WAVE, "waves" },
  };

  ListResult rv(typeNames.at(soundType), typeNamesLC.at(soundType));
  for (auto sound : nw->info->soundDataEntries) {
    if (sound.soundType == soundType && glob.match(sound.name)) {
      rv.matches.push_back(sound.name);
      if (soundType != SoundType::SEQ) {
        continue;
      }
      if (sound.seqData.labelEntry) {
        auto seqFile = nw->getFile(sound.fileIndex, false);
        if (seqFile) {
          RSEQFile* seq = NWChunk::load<RSEQFile>(seqFile, nullptr, nw->ctx);
          if (seq) {
            std::string label = seq->label(sound.seqData.labelEntry);
            if (label.size()) {
              rv.matches.push_back("\tEntrypoint: " + label);
            }
          }
        }
      }
      if (sound.seqData.bankIndex >= 0 && sound.seqData.bankIndex < nw->info->soundBankEntries.size()) {
        auto bankEntry = nw->info->soundBankEntries[sound.seqData.bankIndex];
        rv.matches.push_back("\tBank: " + bankEntry.name);
      }
    }
  }
  return rv;
}

static ListResult listFiles(RSARFile* nw, const Glob& glob)
{
  ListResult rv("Files", "files", true);
  int numEntries = nw->info->fileEntries.size();
  for (int index = 0; index < numEntries; index++) {
    const auto& file = nw->info->fileEntries[index];
    if (file.name.size()) {
      rv.matches.push_back(file.name + " (external)");
      continue;
    } else if (file.positions.size()) {
      bool first = true;
      for (auto pos : file.positions) {
        auto group = nw->info->groupEntries[pos.group];
        std::ostringstream ss;
        if (first) {
          first = false;
        } else {
          ss << "  + ";
        }
        if (!group.name.size()) {
          ss << "nameless_group";
        } else {
          ss << group.name;
        }
        ss << "#" << pos.index;
        rv.matches.push_back(ss.str());
      }
    } else if (file.mainSize || file.audioSize) {
      std::ostringstream ss;
      ss << "nameless_file#" << index;
      rv.matches.push_back(ss.str());
    }

    if (file.mainSize) {
      auto data = nw->getFile(index, false);
      char magic[5] = "\0\0\0\0";
      data.read(magic, 4);
      bool isValid = data.gcount() == 4;
      for (int i = 0; i < 4 && isValid; i++) {
        if (magic[i] < 'A' || magic[i] > 'Z') {
          isValid = false;
        }
      }
      if (isValid) {
        rv.matches.push_back("\tMain Type: " + std::string(magic, 4));
      } else {
        rv.matches.push_back("\tMain Type: unknown");
      }
    }

    if (file.audioSize) {
      auto data = nw->getFile(index, true);
      char magic[5] = "\0\0\0\0";
      data.read(magic, 4);
      bool isValid = data.gcount() == 4;
      for (int i = 0; i < 4 && isValid; i++) {
        if (magic[i] < 'A' || magic[i] > 'Z') {
          isValid = false;
        }
      }
      if (isValid) {
        rv.matches.push_back("\tAudio Type: " + std::string(magic, 4));
      } else {
        rv.matches.push_back("\tAudio Type: unknown");
      }
    }
  }
  return rv;
}

static ListResult listGroups(RSARFile* nw, const Glob& glob)
{
  ListResult rv("Groups", "groups");
  for (auto group : nw->info->groupEntries) {
    std::string match = (group.name.size() ? group.name : "nameless_group") +
      " (" + std::to_string(group.items.size()) + " items)";
    rv.matches.push_back(match);

    if (group.pathRef) {
      rv.matches.push_back("\tExternal file: " + group.externalPath);
    }
  }
  return rv;
}

static ListResult listPlayers(RSARFile* nw, const Glob& glob)
{
  ListResult rv("Players", "players");
  for (auto player : nw->info->playerEntries) {
    rv.matches.push_back(player.name);
  }
  return rv;
}

static ListResult listBanks(RSARFile* nw, const Glob& glob)
{
  ListResult rv("Banks", "banks");
  for (auto bank : nw->info->soundBankEntries) {
    if (!glob.match(bank.name)) {
      continue;
    }
    std::string bankFile;
    bool hasFile = false;
    if (bank.fileIndex >= 0 && nw->info->fileEntries.size() > bank.fileIndex) {
      hasFile = true;
      const auto& file = nw->info->fileEntries[bank.fileIndex];
      bankFile = nw->info->fileEntries[bank.fileIndex].name;
      if (bankFile.size()) {
        bankFile = " (in " + bankFile + ")";
      }
    }
    rv.matches.push_back(bank.name + bankFile);
    if (RSEQTrack::parseVerbose && hasFile) {
      auto file = nw->getFile(bank.fileIndex, false);
      std::unique_ptr<RBNKFile> b(NWChunk::load<RBNKFile>(file, nullptr, nw->ctx));
      auto audioFile = nw->getFile(bank.fileIndex, true);
      std::unique_ptr<RWARFile> war;
      try {
        war.reset(NWChunk::load<RWARFile>(audioFile, nullptr, nw->ctx));
      } catch (...) {
        // ignore
      }
      int numProgs = b->programs.size();
      for (int i = 0; i < numProgs; i++) {
        rv.matches.push_back("\tProgram " + std::to_string(i) + ":");
        const auto& prog = b->programs[i];
        for (const auto& ks : prog.keySplits) {
          for (const auto& vs : ks.velSplits) {
            auto sample = b->programs[i].keySplits[0].velSplits[0].sample;
            std::ostringstream ss;
            ss << "\t  Wave " << sample.wave.pointer
              << " [" << int(ks.minKey) << "-" << int(ks.maxKey) << "]"
              << " (" << int(vs.minVel) << "-" << int(vs.maxVel) << ")"
              << " A=" << int(sample.attack)
              << " H=" << int(sample.hold)
              << " D=" << int(sample.decay)
              << " S=" << int(sample.sustain)
              << " R=" << int(sample.release);
            rv.matches.push_back(ss.str());
            if (war) {
              auto wav = war->getRWAV(sample.wave.pointer);
              if (wav) {
                ss.str("");
                ss << "\t\t ";
                if (wav->format == RWAVFile::PCM8) {
                  ss << "PCM8";
                } else if (wav->format == RWAVFile::ADPCM) {
                  ss << "ADPCM";
                } else {
                  ss << "PCM16";
                }
                ss << " " << wav->sampleRate << "Hz " << wav->channels.size() << "ch ";
                if (wav->looped) {
                  auto sd = war->getSample(sample.wave.pointer);
                  if (sd) {
                    ss << "loop (" << sd->loopStart << "-" << sd->loopEnd <<")";
                  } else {
                    ss << " (decoding error)";
                  }
                }
                rv.matches.push_back(ss.str());
              }
            }
          }
        }
      }
    }
  }
  return rv;
}

static ListResult listLabels(RSARFile* nw, const Glob& glob, const std::string& seq)
{
  ListResult rv("Labels", "labels");
  Glob seqGlob(seq);
  for (auto sound : nw->info->soundDataEntries) {
    if (sound.soundType != SoundType::SEQ || !seqGlob.match(sound.name)) {
      continue;
    }
    auto seqFile = nw->getFile(sound.fileIndex, false);
    RSEQFile* seq = NWChunk::load<RSEQFile>(seqFile, nullptr, nw->ctx);
    bool found = false;
    for (int i = 0; ; i++) {
      std::string label = seq->label(i);
      if (!label.size()) {
        break;
      }
      found = true;
      rv.matches.push_back(sound.name + "\\" + label);
    }
    if (!found) {
      rv.matches.push_back(sound.name);
    }
  }
  return rv;
}

int listMembers(const CommandArgs& args)
{
  std::string listType = args.getString("list");
  std::string seq = args.getString("seq");
  Glob glob(args.getString("filter"));
  std::function<ListResult(RSARFile*, const Glob&)> listFn;
  bool wantsSeq = false;
  if (listType == "seq") {
    listFn = [](RSARFile* nw, const Glob& glob){
      return listBySoundType(nw, glob, SoundType::SEQ);
    };
  } else if (listType == "strm") {
    listFn = [](RSARFile* nw, const Glob& glob){
      return listBySoundType(nw, glob, SoundType::STRM);
    };
  } else if (listType == "wave") {
    listFn = [](RSARFile* nw, const Glob& glob){
      return listBySoundType(nw, glob, SoundType::WAVE);
    };
  } else if (listType == "file") {
    listFn = listFiles;
  } else if (listType == "group") {
    listFn = listGroups;
  } else if (listType == "player") {
    listFn = listPlayers;
  } else if (listType == "bank") {
    listFn = listBanks;
  } else if (listType == "label") {
    if (!seq.size()) {
      std::cerr << "Listing labels requires --seq" << std::endl;
      return 1;
    }
    wantsSeq = true;
    listFn = [seq](RSARFile* nw, const Glob& glob){
      return listLabels(nw, glob, seq);
    };
  } else {
    std::cerr << "Unknown list type: " << listType << std::endl;
    return 1;
  }

  for (const std::string& filename : args.positional()) {
    try {
      auto nwctx(loadRSAR(filename));
      ListResult rv(listFn(nwctx.nw.get(), glob));
      if (!rv.matches.size()) {
        std::cout << "No ";
        if (args.hasKey("filter")) {
          std::cout << "matching ";
        } else if (rv.namedOnly) {
          std::cout << "named ";
        }
        std::cout << rv.listTypeLC << " found in " << filename << std::endl;
      } else {
        if (!wantsSeq) {
          std::cout << rv.listType << " in " << filename << ":" << std::endl;
        }
        std::string lastSeq;
        for (const std::string& match : rv.matches) {
          if (wantsSeq) {
            int slash = match.find('\\');
            if (slash == std::string::npos) {
              std::cout << "No ";
              if (args.hasKey("filter")) {
                std::cout << "matching ";
              }
              std::cout << rv.listTypeLC << " found in " << filename << " - " << match << std::endl;
              continue;
            }
            std::string seqName = match.substr(0, slash);
            std::string matchName = match.substr(slash + 1);
            if (seqName != lastSeq) {
              std::cout << rv.listType << " in " << filename << " - " << seqName << ":" << std::endl;
              lastSeq = seqName;
            }
            std::cout << "\t" << matchName << std::endl;
          } else {
            std::cout << "\t" << match << std::endl;
          }
        }
      }
    } catch (std::exception& e) {
      std::cerr << "Error while reading " << filename << ": " << e.what() << std::endl;
      return 1;
    }
  }
  return 0;
}
