#ifndef NW_LISTACTIONS_H
#define NW_LISTACTIONS_H

#include <string>
#include <vector>
class CommandArgs;

class Glob {
public:
  Glob(const std::string& pattern);

  bool match(const std::string& subject) const;

private:
  std::vector<std::string> sections;
};

int listMembers(const CommandArgs& args);

#endif
