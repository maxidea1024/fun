#pragma once

#include "fun/http/http.h"
#include <initializer_list>
#include <type_traits>
#include <vector>


namespace fun {
namespace http {


class File
{
public:
  template <typename StringType>
  explicit File(StringType&& file_path)
    : file_path{FORWARD(file_path)}
  {
  }

  String file_path;
};


class Buffer
{
public:
  typedef const unsigned char* DataType;

  template <typename Iterator, typename StringType>
  explicit Buffer(Iterator begin, Iterator end, StringType&& filename)
    : data{reinterpret_cast<DataType>(&(*begin))}
    , data_len{static_cast<unsigned long>(std::distance(begin, end))}
    , filename{FORWARD(filename)}
  {
    is_random_access_iterator(begin, end);
    static_assert(sizeof(*begin) == 1, "only byte buffers can be used");
  }

  template <typename Iterator>
  typename std::enable_if<
    std::is_same<typename std::iterator_traits<Iterator>::iterator_category, std::random_access_iterator_tag>::value
    >::type
  is_random_access_iterator(Iterator /* begin */, Iterator /* end */ ) {}

  DataType data;
  unsigned long data_len;
  String filename;
};


class Part
{
public:
  Part(const String& name, const String& value, const String& content_type = {})
    : name{name}
    , value{value}
    , content_type{content_type}
    , is_file{false}
    , is_buffer{false}
  {
  }

  Part(const String& name, const std::int32_t& value, const String& content_type = {})
    : name{name}
    , value{UTF8_TO_TCHAR(std::to_string(value).c_str())}
    , content_type{content_type}
    , is_file{false}
    , is_buffer{false}
  {
  }

  Part(const String& name, const File& file, const String& content_type = {})
    : name{name}
    , value{file.file_path}
    , content_type{content_type}
    , is_file{true}
    , is_buffer{false}
  {
  }

  Part(const String& name, const Buffer& buffer, const String& content_type = {})
    : name{name}
    , value{buffer.filename}
    , content_type{content_type}
    , data{buffer.data}
    , data_len{buffer.data_len}
    , is_file{false}
    , is_buffer{true}
  {
  }

  String name;
  String value;
  String content_type;
  Buffer::DataType data;
  unsigned long data_len;
  bool is_file;
  bool is_buffer;
};


class Multipart
{
public:
  Multipart(const std::initializer_list<Part>& parts)
    : parts(parts)
  {
  }

  std::vector<Part> parts;
};


} // namespace http
} // namespace fun
