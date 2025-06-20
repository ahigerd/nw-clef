#include "rseqfile.h"
#include "lablchunk.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "clefcontext.h"
#include "seq/isequence.h"
#include "seq/itrack.h"
#include "utility.h"
#include <iostream>

static const double expPitch = M_LN2 / 12.0;

struct TrackParser {
  using RSEQPrefix = RSEQFile::RSEQPrefix;
  using RSEQEvent = RSEQFile::RSEQEvent;
  using RSEQTrack = RSEQFile::RSEQTrack;

  RSEQFile* file;
  NWChunk* chunk;
  std::uint32_t offset;
  std::uint32_t elapsed;
  RSEQTrack* track;

  TrackParser(RSEQFile* file, RSEQTrack* track, NWChunk* chunk, std::uint32_t start, std::uint32_t startTime = 0, bool isSub = false)
  : file(file), track(track), chunk(chunk), offset(start + 4), elapsed(0)
  {
    if (startTime > 0) {
      RSEQEvent event;
      event.cmd = RSEQCmd::Rest;
      event.param1 = startTime;
      std::cerr << "initial " << startTime << std::endl;
      track->push_back(event);
    }
    do {
      RSEQEvent event = readEvent();
      if (event.cmd == RSEQCmd::EOT) {
        break;
      } else if (event.cmd == RSEQCmd::AddTrack) {
        TrackParser(file, &file->tracks[event.param1], chunk, event.param2, elapsed);
      } else if (event.cmd == RSEQCmd::AllocTracks) {
        // Ignore
      } else if (event.cmd == RSEQCmd::Tempo) {
        file->tempo = event.param1;
      } else {
        track->push_back(event);
        if (event.cmd < 0x80) {
          elapsed += event.param2;
        } else if (event.cmd == RSEQCmd::Rest) {
          elapsed += event.param1;
        } else if (event.cmd == RSEQCmd::Gosub) {
          if (!file->subs.count(event.param1)) {
            file->subs[event.param1] = RSEQTrack();
            TrackParser(file, &file->subs[event.param1], chunk, event.param1, 0, true);
          }
        } else if (event.cmd == RSEQCmd::Return && isSub) {
          break;
        }
      }
    } while (offset < chunk->rawData.size());
  }

  inline std::uint8_t readByte()
  {
    return chunk->parseU8(offset++);
  }

  inline std::int16_t readS16()
  {
    std::int16_t value = chunk->parseS16(offset);
    offset += 2;
    return value;
  }

  inline std::uint32_t readU24()
  {
    std::uint32_t value = readByte() << 16;
    value |= readByte() << 8;
    value |= readByte();
    return value;
  }

  inline std::int32_t readS24()
  {
    std::uint32_t value = readU24();
    return std::int32_t(value << 8) >> 8; // sign-extend
  }

  inline std::uint32_t readU32()
  {
    std::uint32_t value = chunk->parseU32(offset);
    offset += 4;
    return value;
  }

  std::uint32_t readVLQ()
  {
    std::uint32_t value = 0;
    std::uint8_t b = 0;
    do {
      b = readByte();
      value = (value << 7) | (b & 0x7F);
    } while (b & 0x80);
    return value;
  }

  RSEQEvent readEvent()
  {
    RSEQEvent event;
    event.offset = offset - 4;
    event.cmd = readByte();

    if (event.cmd < 0x80) {
      event.param1 = readByte();
      event.param2 = readVLQ();
    } else {
      switch (event.cmd) {
      case RSEQCmd::Rest:
      case RSEQCmd::ProgramChange:
        event.param1 = readVLQ();
        break;
      case RSEQCmd::AddTrack:
        event.param1 = readByte();
        event.param2 = readU24();
        break;
      case RSEQCmd::Goto:
      case RSEQCmd::Gosub:
        event.param1 = readU24();
        break;
      case RSEQCmd::ModDelay:
      case RSEQCmd::Tempo:
      case RSEQCmd::Sweep:
      case RSEQCmd::AllocTracks:
        event.param1 = readS16();
        break;
      case RSEQCmd::Extended:
        event.cmd = RSEQCmd::ExtendedBase + readByte();
        event.param1 = readByte();
        event.param2 = readS16();
        break;
      case RSEQCmd::LoopEnd:
      case RSEQCmd::Return:
      case RSEQCmd::EOT:
        break;
      default:
        event.param1 = readByte();
        break;
      }
    }

    return event;
  }
};

RSEQFile::RSEQFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init), tracks(16)
{
  readRHeader(is);

  NWChunk* data = section('DATA');
#if 0
  for (auto label : section<LablChunk>('LABL')->labels) {
    std::cout << label.name << "\t" << (label.dataOffset) << std::endl;
  }
#endif
  TrackParser(this, &tracks[0], data, data->parseU32(0) - 0xC);
}

std::string RSEQFile::label(int index) const
{
  auto labels = section<LablChunk>('LABL');
  if (!labels || index < 0 || index >= labels->labels.size()) {
    return std::string();
  }
  return labels->labels.at(index).name;
}

int RSEQFile::findOffset(const RSEQTrack* src, std::uint32_t offset)
{
  int len = src->size();
  for (int j = 0; j < len; j++) {
    const RSEQEvent& target = src->at(j);
    if (target.offset == offset) {
      return j;
    }
  }
  return -1;
}

ITrack* RSEQFile::createTrack(const RSEQTrack& src) const
{
  double tempo = this->tempo;
  double ppqn = 48;
  double timestamp = 0;
  double volume = 1.0, pan = 0.5;
  double attack = .0015, hold = 0, decay = 0.1, sustain = 0.5, release = 0.01;
  double bend = 0, bendRange = 2;
  int oscID = 0;
  bool noteWait = true;

  int program = 0;

  BasicTrack* out = new BasicTrack();
  struct Frame {
    const RSEQTrack* track;
    int index;
  };
  std::vector<Frame> stack;
  Frame loopPoint;
  int loopPointCount = 0;
  const RSEQTrack* srcTrack = &src;

  int i = 0;
  int len = src.size();
  int loopCount = 0;
  for (int i = 0; i < len && loopCount < 2; i++) {
    const RSEQEvent& event = srcTrack->at(i);
    if (event.cmd == RSEQCmd::Goto) {
      int j = findOffset(srcTrack, event.param1);
      if (j < 0) {
        std::cerr << event.offset << " Goto " << event.param1 << std::endl;
        continue;
      } else if (j < i) {
        loopCount++;
      }
      i = j - 1;
    } else if (event.cmd == RSEQCmd::Gosub) {
      if (!subs.count(event.param1)) {
        std::cerr << event.offset << " Gosub " << event.param1 << std::endl;
        continue;
      }
      stack.push_back({ srcTrack, i });
      srcTrack = &subs.at(event.param1);
      i = -1;
    } else if (event.cmd == RSEQCmd::Return) {
      if (!stack.size()) {
        std::cerr << event.offset << " XXX Bad return" << std::endl;
        continue;
      }
      auto frame = stack.back();
      stack.pop_back();
      srcTrack = frame.track;
      i = frame.index;
    } else if (event.cmd == RSEQCmd::LoopStart) {
      std::cerr << event.offset << " loop start " << event.param1 << std::endl;
      loopPoint.track = srcTrack;
      loopPoint.index = i;
      loopPointCount = event.param1;
    } else if (event.cmd == RSEQCmd::LoopEnd) {
      std::cerr << event.offset << " loop end " << loopPointCount << std::endl;
      if (loopPointCount > 0) {
        //loopPoint.track = srcTrack;
        i = loopPoint.index;
      }
      --loopPointCount;
    } else if (event.cmd == RSEQCmd::Ppqn) {
      std::cerr << "PPQN: " << event.param1 << std::endl;
    } else if (event.cmd == RSEQCmd::Tempo) {
      std::cerr << "Tempo: " << event.param1 << std::endl;
    } else if (event.cmd == RSEQCmd::Attack) {
      attack = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Hold) {
      hold = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Decay) {
      decay = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Sustain) {
      sustain = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Release) {
      release = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Volume) {
      volume = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Pan) {
      if (event.param1 == 64) {
        pan = 0.5;
      } else {
        pan = (event.param1 / 127.0);
      }
    } else if (event.cmd == RSEQCmd::Bend) {
      bend = std::int8_t(event.param1) / 127.0;
    } else if (event.cmd == RSEQCmd::BendRange) {
      bendRange = event.param1;
    } else if (event.cmd == RSEQCmd::WaitEnable) {
      noteWait = event.param1;
    } else if (event.cmd == RSEQCmd::ProgramChange) {
      program = event.param1;
    } else if (event.cmd < 0x80) {
      auto sample = bank->getSample(program, event.cmd, event.param1);
      if (!sample) {
        continue;
      }
      SampleData* sd = ctx->getSample(sample->wave.pointer);
      if (!sd) {
        sd = war->getSample(sample->wave.pointer);
      }
      BaseNoteEvent* e;
      if (sd) {
        SampleEvent* se = new SampleEvent;
        se->sampleID = sample->wave.pointer;
        se->pitchBend = fastExp(event.cmd - sample->baseNote + bend * bendRange, expPitch);
        e = se;
      } else {
        continue;
        OscillatorEvent* oe = new OscillatorEvent;
        oe->waveformID = oscID;
        oe->frequency = noteToFreq(event.cmd + bend * bendRange);
        e = oe;
      }
      e->timestamp = timestamp;
      e->volume = volume * (event.param1 / 127.0);
      e->setEnvelope(attack, hold, decay, sustain, release);
      e->duration = event.param2 / ppqn / tempo * 60.0;
      e->pan = pan;
      if (noteWait) {
        timestamp += e->duration;
      }
      out->addEvent(e);
    } else if (event.cmd == RSEQCmd::Rest) {
      timestamp += event.param1 / ppqn / tempo * 60.0;
    } else if (event.cmd >= 0xCA && event.cmd <= 0xE0) {
      // no-op for now
    } else {
      std::cerr << event.offset << " event " << std::hex << event.cmd << std::dec << " " << event.param1 << " " << event.param2 << " " << event.param3 << std::endl;
    }
  }

  return out;
}

ISequence* RSEQFile::sequence(ClefContext* ctx) const
{
  BaseSequence<ITrack>* seq = new BaseSequence<ITrack>(ctx);

  int i = 0;
  for (const RSEQTrack& src : tracks) {
    if (!src.size()) {
      continue;
    }
    seq->addTrack(createTrack(src));
    ++i;
  }

  return seq;
}

void RSEQFile::loadBank(RBNKFile* bank, RWARFile* war)
{
  this->bank = bank;
  this->war = war;
}
