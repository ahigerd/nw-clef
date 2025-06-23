#ifndef CLEF_METAENUM_H
#define CLEF_METAENUM_H

#include <string>
#include <map>
#include <vector>

class MetaEnumBase {
protected:
  MetaEnumBase(const char* name, const char* defs, int len);

  std::string name;
  std::map<int, std::string> toMap;
  std::map<std::string, int> fromMap;
};

namespace _MetaEnum {
  template <typename T> class Data {};
};

template <typename T>
class MetaEnum : public MetaEnumBase {
private:
  using Data = _MetaEnum::Data<T>;
  MetaEnum() : MetaEnumBase(Data::name, Data::defs, Data::len) {}

  static const MetaEnum& instance() {
    static MetaEnum<T> obj;
    return obj;
  }

public:
  static std::string toString(int val) {
    const MetaEnum<T>& obj = instance();
    return obj.toMap.at(val);
  }

  static T fromString(const std::string& val) {
    const MetaEnum<T>& obj = instance();
    return obj.fromMap.at(val);
  }
};

#define META_ENUM(NAME, ...) \
  namespace _MetaEnum { \
    namespace _ ## NAME { enum NAME { __VA_ARGS__ }; } \
    template<> class Data<_ ## NAME::NAME> { \
      friend class MetaEnum<_ ## NAME::NAME>; \
      inline static const char* const name = #NAME; \
      inline static const char* const defs = #__VA_ARGS__; \
      inline static const int len = sizeof(#__VA_ARGS__); \
    }; \
  } \
  using _MetaEnum::_ ## NAME::NAME;


#endif
