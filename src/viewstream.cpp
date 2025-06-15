#include "viewstream.h"
#include <cstring>

viewstreambuf::viewstreambuf(const std::uint8_t* start, const std::uint8_t* end)
: start(const_cast<char*>((char*)start)), end(const_cast<char*>((char*)end))
{
  setg(this->start, this->start, this->end);
}

std::streamsize viewstreambuf::showmanyc()
{
  return end - gptr();
}

std::streamsize viewstreambuf::xsgetn(char* s, std::streamsize count)
{
  char* pos = gptr();
  if (pos + count > end) {
    count = end - pos;
  }
  std::memcpy(s, pos, count);
  gbump(count);
  return count;
}

viewstreambuf::traits_type::pos_type viewstreambuf::seekoff(viewstreambuf::traits_type::off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
{
  switch (dir) {
  case std::ios_base::cur:
    return seekpos((gptr() - start) + off, which);
  case std::ios_base::end:
    return seekpos((end - start) + off, which);
  default:
    return seekpos(off, which);
  }
}

viewstreambuf::traits_type::pos_type viewstreambuf::seekpos(viewstreambuf::traits_type::pos_type p, std::ios_base::openmode which)
{
  char* pos = start + p;
  if (pos > end) {
    pos = end;
  }
  setg(start, pos, end);
  return pos - start;
}

viewstream::viewstream()
: std::istream(&m_buf), m_buf(nullptr, nullptr), m_size(0)
{
  // initializers only
}

viewstream::viewstream(const std::uint8_t* start, const std::uint8_t* end)
: std::istream(&m_buf), m_buf(start, end), m_size(end - start)
{
  // initializers only
}

viewstream::viewstream(const std::int8_t* start, const std::int8_t* end)
: std::istream(&m_buf), m_buf((const std::uint8_t*)start, (const std::uint8_t*)end), m_size(end - start)
{
  // initializers only
}

viewstream::viewstream(std::unique_ptr<std::istream> proxy)
: std::istream(proxy ? proxy->rdbuf() : nullptr), m_buf(nullptr, nullptr), m_proxy(std::move(proxy))
{
  if (proxy) {
    auto pos = proxy->tellg();
    proxy->seekg(0, std::ios::end);
    auto end = proxy->tellg();
    proxy->seekg(pos);
    m_size = end - pos;
  } else {
    m_size = 0;
  }
}

int viewstream::size() const
{
  return m_size;
}
