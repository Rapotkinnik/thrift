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
auto & to_json(const T &value, std::ostream &out) {
  return out << value;
}

auto & to_json(const std::string &str, std::ostream &out) {
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
auto & to_json(const T & iterable, std::ostream & out) {
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
auto to_json(const T && object, std::ostream & out) 
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
};
}}} // apache::thrift::protocol

#endif // #define _THRIFT_PROTOCOL_TJSONSERIALIZER_H_ 1