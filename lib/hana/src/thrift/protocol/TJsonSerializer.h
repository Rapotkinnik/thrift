/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <string>
#include <sstream>
#include <optional>

#include <boost/hana.hpp>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TTypeTraits.h>
#include <thrift/protocol/TProtocolException.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace details {

struct Indent {
  auto & up() {
    return ++count_, *this;
  }

  auto & down() {
    if (!count_) {
      --count_;
    }

    return *this;
  }

  char sign_ = ' ';
  size_t count_ = 0;
};  // struct Indent

inline auto & operator << (std::ostream & out, const Indent & indent) {
  for (auto i = 0; i < indent.count_; ++i) {
    out << indent.sign_;
  }

  return out;
}

class IndentGuard {
 public:
  IndentGuard(Indent &indent, std::ostream & out):
      indent_(indent), out_(out) {
    out_ << indent_.up();
  }

  ~IndentGuard() {
    out_ << indent_.down();
  }

 private:
  Indent &indent_;
  std::ostream &out_;
};  // struct IndentGuard

struct EndLine {
  auto & operator << (std::ostream & out) {
    return out << sign_;
  }

  char sign_ = '\n';
};  // struct EndLine
}  // namespace details

namespace hana = boost::hana;
using namespace hana::literals;
using apache::thrift::protocol::TType;
using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TProtocolException;

template <typename T>
class TJsonSerializer {
 public:
  explicit TJsonSerializer(const T & object)
    : object_(object) {};

  auto & pretty(char indent = '\t') {
    indent_.sign_ = indent;
    endline_.sign_ = '\n';

    return *this;
  }

  // TODO: Add some format configurations
  //       for instance use std::boolalpha or std::fixed etc.

//  auto & raw() {
//    indent_.sign_ = '';
//    endline_.sign_ = '';

//    return *this;
//  }

  auto & write(std::ostream & out) {
    serialize(object_, out);
    return *this;
  }

 private:
  template <typename T>
  auto & serialize(const T & value, std::ostream & out) {
    return out << value;
  }

  auto & serialize(const char * str, std::ostream & out) {
    return out << '"' << str << '"';
  }

  auto & serialize(const std::string & str, std::ostream & out) {
    return out << '"' << str << '"';
  }

  template <typename T, typename U>
  auto & serialize(const std::pair<T, U> & pair, std::ostream & out) {
    out << '{' << endline_;
    {
      details::IndentGuard guard {indent_, out};
      serialize(std::to_string(pair.first), out);
      serialize(pair.second, out << ": ");
    }
    return out << endline_ << '}';
  }

  template <typename T>
  auto & serialize(const T & iterable, std::ostream & out,
                   typename std::enable_if<is_iterable<T>::value>::type * = 0) {
    out << '[' << endline_;
    {
      details::IndentGuard guard {indent_, out};
      for (const auto & elem : iterable) {
        serialize(elem, out) << ", ";
      }

      (std::size(iterable) ? out.seekp(-2, std::ios::end) : out) << endline_;
    }
    return out << ']';
  }

  template <typename T>
  auto & serialize(const T & object, std::ostream & out,
                   typename std::enable_if<is_hana_object<T>::value>::type * = 0) {
    out << '{' << endline_;
    {
      size_t count{};
      details::IndentGuard guard {indent_, out};
      hana::for_each(hana::accessors<T>(), [&](const auto & accessor) {
        auto meta = hana::first(accessor);
        auto member = hana::second(accessor);

        if (member(object).has_value()) {
          count++;
          serialize(hana::first(meta), out) << ": ";     // name
          serialize(member(object).value(), out) << ','; // value
        }
      });

      (count ? out.seekp(-1, std::ios::end) : out) << endline_;
    }

    return out << '}';
  }

  const T & object_;
  mutable details::Indent indent_;
  mutable details::EndLine endline_;
};  // class TJsonSerializer

template <typename T>
inline auto & operator << (std::ostream & out, const TJsonSerializer<T> & ser) {
  return ser.write(out), out;
}


template <typename T>
class TJsonDeserializer {
 public:
  explicit TJsonDeserializer(T & object)
    : object_(object) {};

  auto & read(std::istream & in) {
    deserialize(object_, in);
    return *this;
  }

  auto & object() {
    return object_;
  }

 private:
  template <typename T>
  auto & deserialize(T & value, std::istream & in) {
    return in >> value;
  }

  auto & deserialize(std::string &str, std::istream &in) {
    char ch {};
    std::stringbuf buff{};
    in.ignore(1, '"').get(ch)
      .get(buff, '"').get(ch);
    return str = std::move(buff.str()), in;
  }

  template <typename T, typename U>
  auto & deserialize(std::pair<T, U> & pair, std::istream & in) {
    char ch {};
    std::string key;
    in.ignore(1, '{');
    deserialize(key, in)
        .ignore(1, ':').get(ch);
    deserialize(pair.second, in).get(ch);  // should be '}'

    return std::istringstream{key} >> pair.first, in;
  }

  template <typename T>
  auto & deserialize(T & iterable, std::istream & in,
                     typename std::enable_if<is_iterable<T>::value>::type * = 0) {
    char ch {};
    in.ignore(1, '[');
    while (in.get(ch), ch != ']')
      iterable.emplace_back();
      deserialize(iterable.back(), in)
        .ignore(1, ',')
        .ignore(1, ' ');

    return in;
  }

  template <typename T>
  auto deserialize(T & object, std::istream & in,
                   typename std::enable_if<is_hana_object<T>::value>::type * = 0)
  {
    char ch {};
    std::string name {};

    in.ignore(1, '{');
    while (in.get(ch), ch != '}')
    {
      deserialize(name, in)
          .ignore(1, ':').get(ch);  // should be ':'

      bool skip = true;
      hana::for_each(hana::accessors<T>(), [&](auto & accessor) {
        auto meta = hana::first(accessor);
        auto member = hana::second(accessor);

        if (hana::first(meta) == name) {
          skip = false;
          deserialize(member(object).emplace(), in);
        }
      });

      if (skip) {
        // TODO: ignore data...
        //       in.ignore(1, ',' or '}')
      }
    }

    // TODO: Check if required fields are filled
    //       and throw TProtocolException if they are not

    return in;
  }

 private:
  class JsonContext {
  };  // class JsonContext

  T & object_;
  JsonContext context_;
};  // class TJsonDeserializer

template <typename T>
inline auto & operator >> (std::istream & in, const TJsonDeserializer<T> & ser) {
  return ser.read(in), in;
}

template <typename T>
T from_json(const std::string & json) {
  T object{};
  std::stringstream ss{json};
  return ss >> TJsonDeserializer{object}, object;
};

template <typename T>
auto to_json(const T & object) {
  std::stringstream ss{};
  return ss << TJsonSerializer<T>{object}, ss.str();
};

template <typename T>
auto to_pretty_json(const T & object) {
  std::stringstream ss{};
  return ss << TJsonSerializer<T>{object}.pretty(), ss.str();
};
}}} // namespace apache::thrift::protocol
