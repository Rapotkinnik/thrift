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

using namespace boost;
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

//  auto & raw() {
//    indent_.sign_ = '';
//    endline_.sign_ = '';

//    return *this;
//  }

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
      hana::for_each(hana::accessors<T>(object), [&](const auto & accessor) {
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
  details::Indent indent_;
  details::EndLine endline_;

  friend std::ostream & operator << (std::ostream &, const TJsonSerializer &);
};  // class TJsonSerializer

template <typename T>
inline auto & operator << (std::ostream & out, const TJsonSerializer<T> & ser) {
  return ser.serialize(ser.object_, out);
}

//class TJsonDeserializer {
// public:
//  auto & serialize(const std::string &str, std::ostream &out) {
//    return out << '"' << str << '"';
//  }

//  template <typename T, typename U>
//  auto & to_json(const std::pair<T, U> &pair, std::ostream &out) {
//    to_json(std::to_string(pair.first), out  << '{');
//    return to_json(pair.second, out << ':') << '}';
//  }

//  template <
//    typename T,
//    typename =
//      typename std::enable_if<
//        is_iterable<T>::value>::type>
//  auto & serialize(const T & iterable, std::ostream & out) {
//    out << '[';
//    for (const auto & elem : iterable)
//      to_json(elem, out << ',');

//    return (std::size(iterable) ? out.seekp(-1, std::io::end) : out) << ']';
//  }

//  template <
//    typename T,
//    typename =
//      typename std::enable_if<
//        is_hana_object<T>::value>::type>
//  auto serialize(const T && object, std::ostream & out)
//  {
//    out << '{';
//    size_t count {};
//    hana::for_each(hana::accessors(object), [&](auto acessor) {
//      auto [meta, member] = acessor;
//      auto [name, id] = meta;

//      if (member(object)) {
//        ++cout;
//        out << name << ':';
//        to_json(member(object).value(), out) << ',';
//      }
//    });

//    return (count ? out.seekp(-1, std::io::end) : out) << '}';
//  }

// private:
//  class JsonContext {
//  };  // class JsonContext

//  JsonContext context_;
//};  // class TJsonDeserializer

//template <typename T>
//T from_json(const std::string &json) {
//  T object {};
//  std::sstream ss{json};
//  return ss >> TJsonDeserializer{object}, object;
//};

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
