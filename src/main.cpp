#include "clefcontext.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "nwfile.h"
#include <fstream>

int main(int argc, char** argv)
{
  std::ifstream is(argv[1]);
  NWFile nw(is);
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
