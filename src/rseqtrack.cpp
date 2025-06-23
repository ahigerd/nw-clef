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

static std::string formatEvent(const RSEQTrack::RSEQEvent& event)
{
  std::ostringstream ss;

  if (event.cmd < 0x80) {
    ss << midiNoteSymbol(event.cmd);
  } else {
    try {
      ss << MetaEnum<RSEQCmd>::toString(event.cmd);
    } catch (...) {
      ss << "Unknown" << std::hex << int(event.cmd) << std::dec;
    }
  }

  int pc = paramCount(event.cmd);
  if (!pc) {
    return ss.str();
  }
  ss << "(";
  if (pc >= 1) {
    ss << event.param1;
  }
  if (pc >= 2) {
    ss << "," << event.param2;
  }
  ss << ")";

  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const RSEQTrack::RSEQEvent& event)
{
  return os << formatEvent(event);
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

  std::cout << "T" << trackIndex << " " << tickPos << std::endl;

  parseOffset = offset;
  hexdump(chunk->rawData.data() + offset, 32);
  do {
    std::uint32_t o = parseOffset;
    RSEQEvent event = readEvent();
    if (trackIndex == 12) {
      for (int indent = 0; indent < parseStack.size(); indent++) std::cout << "  ";
      std::cout << o << " / " << tickPos <<  ": " << event << std::endl;
    }
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
      std::cout << "->T" << event.param1 << " " << tickPos << std::endl;
      track.parse(event.param2);
    } else if (event.cmd == RSEQCmd::Gosub) {
      if (trackIndex == 12) {
        std::cout << "gosub " << parseOffset << " -> " << event.param1 << std::endl;
        hexdump(chunk->rawData.data() + event.param1, 16);
      }
      parserPush(event.param1);
    } else if (event.cmd == RSEQCmd::Return) {
      parserPop();
      if (trackIndex == 12) std::cout << "return " << parseOffset << std::endl;
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
  } while (parseOffset < chunk->rawData.size());
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
    // TODO: prefix
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
