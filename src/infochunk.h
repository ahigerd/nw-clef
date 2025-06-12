#ifndef NW_INFOCHUNK_H
#define NW_INFOCHUNK_H

class NWFile;

class InfoChunk
{
public:
  InfoChunk(NWFile* file);

private:
  NWFile* file;
};

#endif
