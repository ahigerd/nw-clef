#ifndef NW_VIEWSTREAM_H
#define NW_VIEWSTREAM_H

#include <istream>
#include <streambuf>
#include <cstdint>
#include <memory>

class viewstreambuf : public std::streambuf
{
public:
  viewstreambuf(const std::uint8_t* start, const std::uint8_t* end);

  inline int size() const { return end - start; }

protected:
  virtual std::streamsize xsgetn(char* s, std::streamsize count) override;
  virtual std::streamsize showmanyc() override;
  virtual traits_type::pos_type seekoff(traits_type::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override;
  virtual traits_type::pos_type seekpos(traits_type::pos_type pos, std::ios_base::openmode which = std::ios_base::in) override;

private:
  char* start;
  char* end;
};

class viewstream : public std::istream
{
public:
  viewstream();
  viewstream(const std::uint8_t* start, const std::uint8_t* end);
  viewstream(const std::int8_t* start, const std::int8_t* end);
  viewstream(std::unique_ptr<std::istream> proxy);

  int size() const;

private:
  viewstreambuf m_buf;
  std::unique_ptr<std::istream> m_proxy;
  int m_size;
};

#endif
