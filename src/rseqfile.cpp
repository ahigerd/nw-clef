#include "rseqfile.h"
#include "lablchunk.h"
#include "rbnkfile.h"
#include "rwarfile.h"
#include "clefcontext.h"
#include "seq/isequence.h"
#include "seq/itrack.h"
#include "synth/sampler.h"
#include "nwinstrument.h"
#include "utility.h"

RSEQFile::RSEQFile(std::istream& is, const ChunkInit& init)
: NWFile(is, init)
{
  readRHeader(is);

  NWChunk* data = section('DATA');
#if 0
  for (auto label : section<LablChunk>('LABL')->labels) {
    std::cout << label.name << "\t" << (label.dataOffset) << std::endl;
  }
#endif
  for (int i = 0; i < 16; i++) {
    tracks.emplace_back(this, data, i);
  }
  tempos[0] = 60.0 / (120 * 48); // 120 beats/minute = 2 ticks/second * 48 ticks/beat
  std::uint32_t startOffset = data->parseU32(0) - 0xC;
  data->rawData.erase(data->rawData.begin(), data->rawData.begin() + 4);
  tracks[0].parse(startOffset);
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
  int len = src->events.size();
  for (int j = 0; j < len; j++) {
    const RSEQTrack::RSEQEvent& target = src->events.at(j);
    if (target.offset == offset) {
      return j;
    }
  }
  return -1;
}

double RSEQFile::ticksToTimestamp(std::uint32_t ticks) const
{
  double timestamp = 0;
  double tempo = 0;
  for (auto pair : tempos) {
    if (pair.first > ticks) {
      timestamp += pair.second * pair.first;
    } else {
      timestamp += pair.second * (ticks - pair.first);
      break;
    }
  }
  return timestamp;
}

ITrack* RSEQFile::createTrack(const RSEQTrack& src) const
{
  double bend = 0, bendRange = 2;

  NWInstrument inst(bank, war);

  BasicTrack* out = new BasicTrack();
  int loopPoint = 0;
  int loopPointCount = 0;
  const RSEQTrack* srcTrack = &src;

  int i = 0;
  int len = src.events.size();
  int loopCount = 0;
  std::uint64_t lastID = 0;
  for (int i = 0; i < len && loopCount < 2; i++) {
    const RSEQTrack::RSEQEvent& event = srcTrack->events.at(i);
    if (event.cmd == RSEQCmd::Goto) {
      int j = findOffset(srcTrack, event.param1);
      if (j < 0) {
        std::cerr << event.offset << " Goto " << event.param1 << std::endl;
        continue;
      } else if (j < i) {
        loopCount++;
      }
      i = j - 1;
    } else if (event.cmd == RSEQCmd::LoopStart) {
      std::cerr << event.offset << " loop start " << event.param1 << std::endl;
      loopPoint = i;
      loopPointCount = event.param1;
    } else if (event.cmd == RSEQCmd::LoopEnd) {
      std::cerr << event.offset << " loop end " << loopPointCount << std::endl;
      if (loopPointCount > 0) {
        i = loopPoint;
      }
      --loopPointCount;
    } else if (event.cmd == RSEQCmd::Attack) {
      inst.attack = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Hold) {
      inst.hold = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Decay) {
      inst.decay = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Sustain) {
      inst.sustain = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Release) {
      inst.release = event.param1 / 127.0;
    } else if (event.cmd == RSEQCmd::Volume) {
      ChannelEvent* e = new ChannelEvent(AudioNode::Gain, event.param1 / 127.0);
      e->timestamp = ticksToTimestamp(event.timestamp);
      out->addEvent(e);
    } else if (event.cmd == RSEQCmd::Pan) {
      ChannelEvent* e = new ChannelEvent(AudioNode::Pan, event.param1 / 128.0);
      e->timestamp = ticksToTimestamp(event.timestamp);
      out->addEvent(e);
    } else if (event.cmd == RSEQCmd::Bend) {
      bend = std::int8_t(event.param1) / 127.0;
      inst.pitchBend = bend * bendRange;
      ModulatorEvent* e = new ModulatorEvent(Sampler::PitchBend, inst.pitchBend);
      e->timestamp = ticksToTimestamp(event.timestamp);
      out->addEvent(e);
    } else if (event.cmd == RSEQCmd::BendRange) {
      bendRange = event.param1;
      inst.pitchBend = bend * bendRange;
    } else if (event.cmd == RSEQCmd::WaitEnable) {
      // ignore, already handled
    } else if (event.cmd == RSEQCmd::ProgramChange) {
      inst.program = event.param1;
    } else if (event.cmd < 0x80) {
      double start = ticksToTimestamp(event.timestamp);
      double end = ticksToTimestamp(event.timestamp + event.param2);
      double duration = end - start;
      auto e = inst.makeEvent(event.cmd, event.param1, duration);
      if (e) {
        lastID = e->playbackID;
        e->timestamp = start;
        out->addEvent(e);
      }
    } else if (event.cmd == RSEQCmd::Rest) {
      // ignore, already handled
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
    if (!src.events.size()) {
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
