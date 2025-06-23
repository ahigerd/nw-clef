#include "rseqtrack.h"
#include "rseqfile.h"
#include "utility.h"
#include <sstream>

static int paramCount(int cmd)
{
  if (cmd < 0x80) {
    return 2;
  } else if (cmd > RSEQCmd::ExtendedBase) {
    return 3;
  }
  switch (cmd) {
  case RSEQCmd::AddTrack:
    return 2;
  case RSEQCmd::LoopEnd:
  case RSEQCmd::Return:
  case RSEQCmd::EOT:
    return 0;
  default:
    return 1;
  }
}

std::string RSEQTrack::RSEQEvent::format() const
{
  std::ostringstream ss;

  for (const auto& prefix : this->prefix) {
    ss << "[" << MetaEnum<RSEQCmd>::toString(prefix.cmd) << "(";
    if (prefix.cmd != RSEQCmd::PrefixIf) {
      ss << prefix.param1;
    }
    if (prefix.cmd == RSEQCmd::PrefixRand || prefix.cmd == RSEQCmd::PrefixTimeRand) {
      ss << "," << prefix.param2;
    }
    ss << ")] ";
  }

  if (cmd < 0x80) {
    ss << midiNoteSymbol(cmd);
  } else {
    try {
      ss << MetaEnum<RSEQCmd>::toString(cmd);
    } catch (...) {
      ss << "Unknown" << std::hex << int(cmd) << std::dec;
    }
  }

  int pc = paramCount(cmd);
  if (!pc) {
    return ss.str();
  }
  ss << "(";
  if (pc >= 1) {
    ss << param1;
  }
  if (pc >= 2) {
    ss << "," << param2;
  }
  ss << ")";

  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const RSEQTrack::RSEQEvent& event)
{
  return os << event.format();
}

static int eventDuration(const RSEQTrack::RSEQEvent& event)
{
  if (event.cmd < 0x80) {
    return event.param2;
  } else if (event.cmd == RSEQCmd::Rest) {
    return event.param1;
  }
  return 0;
}

RSEQTrack::RSEQTrack(RSEQFile* file, NWChunk* chunk, int trackIndex)
: SEQTrack(chunk), file(file), tickPos(0), trackIndex(trackIndex), noteWait(true)
{
  // initializers only
}

void RSEQTrack::addEvent(const RSEQEvent& event)
{
  if (event.cmd >= 0x80 || noteWait) {
    tickPos += eventDuration(event);
  }
  events.push_back(event);
}

int RSEQTrack::findEvent(std::uint32_t offset) const
{
  int len = events.size();
  for (int i = 0; i < len; i++) {
    if (events[i].offset == offset) {
      return i;
    }
  }
  return -1;
}

void RSEQTrack::parse(std::uint32_t offset)
{
  double tempo = 120;
  double ppqn = 48;

  parseOffset = offset;
  std::string indent;
  while (parseOffset < chunk->rawData.size()) {
    RSEQEvent event = readEvent();
    // std::cout << "[" << trackIndex << "] " << indent << event.offset << " " << event.timestamp << ": " << event << std::endl;
    if (event.cmd == RSEQCmd::EOT) {
      break;
    } else if (event.cmd == RSEQCmd::AddTrack) {
      RSEQTrack& track = file->tracks[event.param1];
      if (tickPos > 0) {
        RSEQEvent rest;
        rest.offset = 0xFFFF;
        rest.timestamp = 0;
        rest.cmd = RSEQCmd::Rest;
        rest.param1 = tickPos;
        track.addEvent(rest);
      }
      track.parse(event.param2);
    } else if (event.cmd == RSEQCmd::Gosub) {
      parserPush(event.param1);
      indent = indent + "  ";
    } else if (event.cmd == RSEQCmd::Return) {
      if (!parseStack.size()) {
        std::cerr << "XXX: unexpected Return" << std::endl;
        break;
      }
      parserPop();
      indent = indent.substr(0, indent.size() - 2);
    } else if (event.cmd == RSEQCmd::Tempo) {
      tempo = event.param1;
      file->tempos[tickPos] = 60.0 / (tempo * ppqn);
    } else if (event.cmd == RSEQCmd::Ppqn) {
      ppqn = event.param1;
      file->tempos[tickPos] = 60.0 / (tempo * ppqn);
    } else if (event.cmd == RSEQCmd::WaitEnable) {
      noteWait = event.param1;
    } else if (event.cmd == RSEQCmd::AllocTracks) {
      // Ignore
    } else {
      addEvent(event);
      if (event.cmd == RSEQCmd::Goto) {
        int target = findEvent(event.param1);
        if (target >= 0) {
          // TODO: non-loop backwards gotos
          loopTimestamp = events[target].timestamp;
          break;
        }
        parseOffset = event.param1;
      }
    }
  }
}

RSEQTrack::RSEQEvent RSEQTrack::readEvent()
{
  RSEQEvent event;
  event.timestamp = tickPos;
  event.offset = parseOffset;
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
    case RSEQCmd::PrefixRand:
      event.param1 = readS16();
      event.param2 = readS16();
      break;
    case RSEQCmd::PrefixVar:
      event.param1 = readByte();
      break;
    case RSEQCmd::PrefixTime:
    case RSEQCmd::PrefixTimeRand:
    case RSEQCmd::PrefixTimeVar:
    case RSEQCmd::LoopEnd:
    case RSEQCmd::Return:
    case RSEQCmd::EOT:
      break;
    default:
      event.param1 = readByte();
      break;
    }
  }

  if (event.cmd >= RSEQCmd::PrefixRand && event.cmd <= RSEQCmd::PrefixTimeVar) {
    RSEQEvent nextEvent = readEvent();
    nextEvent.prefix.insert(nextEvent.prefix.begin(), { std::uint8_t(event.cmd), std::int16_t(event.param1), std::int16_t(event.param2) });
    if (event.cmd >= RSEQCmd::PrefixTime) {
      nextEvent.prefix[0].param1 = readS16();
      if (event.cmd == RSEQCmd::PrefixTimeRand) {
        nextEvent.prefix[0].param2 = readS16();
      }
    }
    return nextEvent;
  }

  return event;
}
