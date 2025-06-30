#include "rseqcsv.h"
#include "rseqtrack.h"
#include <iostream>
#include <fstream>
#include <iomanip>

int generateCsv(RSEQFile* file, const std::string& filename)
{
  std::ofstream o(filename, std::ios::out | std::ios::trunc);
  if (!o) {
    std::cerr << "Unable to open \"" << filename << "\" for writing" << std::endl;
    return 1;
  }

  std::uint32_t now = 0;
  std::uint32_t next = 0xFFFFFFFF;

  ISequence* seq = file->sequence();
  std::vector<RSEQTrack*> tracks;
  std::vector<int> eventIndex;

  o << "\"Ticks\"";
  for (int i = 0; i < 16; i++) {
    RSEQTrack* track = dynamic_cast<RSEQTrack*>(seq->getTrack(i));
    if (track && track->events.size()) {
      tracks.push_back(track);
      eventIndex.push_back(i == 0 || !RSEQTrack::parseVerbose ? 0 : -1);
      o << ",\"Track " << i << "\"";
    }
  }
  o << std::endl;

  int numTracks = tracks.size();
  int unfinished = numTracks;
  while (unfinished > 0 && now != 0xFFFFFFFF) {
    unfinished = 0;
    o << now;
    for (int i = 0; i < numTracks; i++) {
      RSEQTrack* track = tracks[i];
      int& idx = eventIndex[i];
      if (idx < 0 || idx >= track->events.size()) {
        o << ",\"\"";
        continue;
      }
      unfinished++;
      auto event = track->events[eventIndex[i]];
      std::uint32_t eventNext = next;
      if (event.timestamp == now) {
        if (event.cmd == RSEQCmd::AddTrack) {
          for (int i = 0; i < tracks.size(); i++) {
            if (tracks[i]->trackIndex == event.param1) {
              eventIndex[i] = 0;
            }
          }
        }
        if (eventIndex[i] == 0 && event.cmd == RSEQCmd::Rest) {
          // Don't output synthetic rest event
          o << ",\"\"";
        } else {
          o << ",\"" << std::hex << std::setw(4) << std::setfill('0') << event.offset << std::dec << ": " << event << "\"";
        }
        do {
          ++idx;
        } while (idx < track->events.size() && !RSEQTrack::parseVerbose && track->events[idx].cmd == RSEQCmd::Rest);
        if (idx < track->events.size()) {
          eventNext = track->events[idx].timestamp;
        }
      } else {
        o << ",\"\"";
        eventNext = event.timestamp;
      }
      if (eventNext < next) {
        next = eventNext;
      }
    }
    now = next;
    next = 0xFFFFFFFF;
    o << std::endl;
  }

  return 0;
}
