#include "metaenum.h"
#include <iostream>
#include <stdexcept>

enum ParseState {
  WantKey,
  ReadKey,
  WantEquals,
  WantValue,
  ReadValue,
  WantComma,
  Done,
};

static bool validateIdentifier(const std::string& key)
{
  for (char ch : key) {
    if (ch < '0') {
      return false;
    }
    if (ch > '9' && ch < 'A') {
      return false;
    }
    if (ch > 'Z' && ch < 'a' && ch != '_') {
      return false;
    }
    if (ch > 'z') {
      return false;
    }
  }
  return true;
}

MetaEnumBase::MetaEnumBase(const char* _name, const char* defs, int len)
: name(_name)
{
  const char* keyStart = nullptr;
  const char* valStart = nullptr;
  std::string key;
  int val = -1;
  bool hasVal = false;
  ParseState state = WantKey;
  for (int i = 0; i < len; i++) {
    char ch = defs[i];
    if (state == WantKey) {
      if (ch != ' ') {
        keyStart = defs + i;
        state = ReadKey;
      }
    } else if (state == ReadKey) {
      if (ch == ' ' || ch == '=' || ch == ',') {
        key = std::string(keyStart, defs + i);
        if (!validateIdentifier(key)) {
          throw std::invalid_argument(name + ": invalid MetaEnum key '" + key + "'");
        }
        if (ch == ' ') {
          state = WantEquals;
        } else if (ch == '=') {
          state = WantValue;
        } else {
          state = Done;
        }
      }
    } else if (state == WantEquals) {
      if (ch == '=') {
        state = WantValue;
      } else if (ch == ',') {
        state = Done;
      }
    } else if (state == WantValue) {
      hasVal = true;
      if (ch != ' ') {
        valStart = defs + i;
        state = ReadValue;
      }
    } else if (state == ReadValue) {
      if (ch == ' ' || ch == ',') {
        std::size_t end = 0;
        std::string valStr(valStart, defs + i);
        val = std::stoi(valStr, &end, 0);
        if (end != valStr.size()) {
          throw std::invalid_argument(name + ": invalid MetaEnum value '" + valStr + "'");
        }
        state = ch == ',' ? Done : WantComma;
      }
    } else if (state == WantComma) {
      if (ch == ',') {
        state = Done;
      }
    }
    if (state == Done) {
      if (!hasVal) {
        ++val;
      }
      toMap[val] = key;
      fromMap[key] = val;
      key.clear();
      hasVal = false;
      state = WantKey;
    }
  }
  if (state == ReadKey) {
    key = std::string(keyStart, defs + len);
  }
  if (hasVal && state == ReadValue) {
    val = std::stoi(std::string(valStart, defs + len), nullptr, 0);
  }
  if (key.size()) {
    if (!hasVal) {
      ++val;
    }
    toMap[val] = key;
    fromMap[key] = val;
  }
}
