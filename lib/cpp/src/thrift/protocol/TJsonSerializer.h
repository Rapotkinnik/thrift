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

#ifndef _THRIFT_PROTOCOL_TJSONSERIALIZER_H_
#define _THRIFT_PROTOCOL_TJSONSERIALIZER_H_ 1

#includ <string>
#includ <sstream>

#include <boost/hana.hpp>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TProtocolException.h>

namespace apache {
namespace thrift {
namespace protocol {

using namespace boost;
using apache::protocol::TType;
using apache::protocol::TProtocol;

template <typename T>
class TJsonSerializer {
 public:
  TJsonSerializer(const T & object) : object_(object) {};

  auto & operator << (std::ostream & out) {
    return serialize(object_, out), out;
  }
  
  auto & pretty(char sign = '\t') {
	indent_.sign_ = sign;
	endline_.sign_ = '\n';
	
    return *this;
  }
  
  auto & raw() {
	indent_.sign_ = '';
	endline_.sign_ = '';
	
    return *this;
  }
  
 private:
  struct Indent {
	auto & operator << (std::ostream & out) {
	  for (auto i = 0; i < count_; ++i)
        out << ident_;
	  return out;
	}
	
	auto & operator++() {
	  return ++count_, *this;
	}
	
	auto & operator--() {
	  if (!count_) {
        --count_;
	  }
	  
	  return *this;
	}
	
    char sign_ = ' ';
	size_t count_ = 0;
  };  // struct Indent
  
  struct EndLine {
    auto & operator << (std::ostream & out) {
	  return out << sign_;
	  
    char sign_ = '\n';
  }  // struct EndLine
  
  auto & serialize(const std::string &str, std::ostream &out) {
    return out << '"' << str << '"';
  }
  
  template <typename T, typename U>
  auto & serialize(const std::pair<T, U> &pair, std::ostream & out) {
    out << '{' << endline_ << indent_++;
    serialize(std::to_string(pair.first), out);
    return serialize(pair.second, out << ': ') << endline_ << indent_-- << '}';
  }
  
  template <
  typename T,
  typename =
    typename std::enable_if<
      is_iterable<T>::value>::type>
  auto & serialize(const T & iterable, std::ostream & out) {
    out << '[' << endline_ << indent_++;
    for (const auto & elem : iterable)
      serialize(elem, out << ', ');

    return (std::size(iterable) ? out.seekp(-2, std::io::end) : out) << endline_ << indent_-- << ']';
  }

  template <
    typename T,
    typename =
      typename std::enable_if<
        is_hana_object<T>::value>::type>
  auto serialize(const T && object, std::ostream & out) 
  {
    size_t count {};
    out << '{' << endline_ << indent_++;
    hana::for_each(hana::accessors(object), [&](auto acessor) {
      auto [meta, member] = acessor;
      auto [name, id] = meta;

      if (member(object)) {
        ++cout;
        out << name << ': ';
        serialize(member(object).value(), out) << ',';
      }
    });
	
    return (count ? out.seekp(-1, std::io::end) : out) << endline_ << indent_-- << '}';
  }
  
  Indent indent_;
  EndLine endline_;
  const T & object_;
};  // class TJsonSerializer

class TJsonDeserializer {
 public:
  auto & serialize(const std::string &str, std::ostream &out) {
    return out << '"' << str << '"';
  }
  
  template <typename T, typename U>
  auto & to_json(const std::pair<T, U> &pair, std::ostream &out) {
    to_json(std::to_string(pair.first), out  << '{');
    return to_json(pair.second, out << ':') << '}';
  }
  
  template <
  typename T,
  typename =
    typename std::enable_if<
	  is_iterable<T>::value>::type>
  auto & serialize(const T & iterable, std::ostream & out) {
    out << '[';
    for (const auto & elem : iterable)
      to_json(elem, out << ',');

    return (std::size(iterable) ? out.seekp(-1, std::io::end) : out) << ']';
  }

  template <
    typename T,
    typename =
      typename std::enable_if<
        is_hana_object<T>::value>::type>
  auto serialize(const T && object, std::ostream & out) 
  {
    out << '{';
    size_t count {};
    hana::for_each(hana::accessors(object), [&](auto acessor) {
      auto [meta, member] = acessor;
      auto [name, id] = meta;

      if (member(object)) {
        ++cout;
        out << name << ':';
        to_json(member(object).value(), out) << ',';
      }
    });
	
    return (count ? out.seekp(-1, std::io::end) : out) << '}';
  }
  
 private:
  class JsonContext {
  };  // class JsonContext
  
  JsonContext context_;
};  // class TJsonDeserializer

template <typename T>
T from_json(const std::string &json) {
  T object {};
  std::sstream ss{json};
  return ss >> TJsonDeserializer{object}, object;
};

template <typename T>
auto to_json(const T && object) {
  std::sstream ss{};
  return ss << TJsonSerializer{object}, ss.str();
};

template <typename T>
auto to_pretty_json(const T && object) {
  std::sstream ss{};
  return ss << TJsonSerializer{object}.pretty(), ss.str();
};
}}} // apache::thrift::protocol

#endif // #define _THRIFT_PROTOCOL_TJSONSERIALIZER_H_ 1