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

#ifndef _THRIFT_PROTOCOL_TBASESERIALIZER_H_
#define _THRIFT_PROTOCOL_TBASESERIALIZER_H_ 1

#include <boost/hana.hpp>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TProtocolException.h>

namespace apache {
namespace thrift {
namespace protocol {

using namespace boost;
using apache::protocol::TType;
using apache::protocol::TProtocol;
  T_STOP       = 0,
  T_VOID       = 1,
  T_BOOL       = 2,
  T_BYTE       = 3,
  T_I08        = 3,
  T_I16        = 6,
  T_I32        = 8,
  T_U64        = 9,
  T_I64        = 10,
  T_DOUBLE     = 4,
  T_STRING     = 11,
  T_UTF7       = 11,
  T_STRUCT     = 12,
  T_MAP        = 13,
  T_SET        = 14,
  T_LIST       = 15,
  T_UTF8       = 16,
  T_UTF16      = 17

hana::make_map(
template <TType v>
using type2enum = std::integral_constant<TType, v>;

type2enum<TType::T_BOOL>
template <typename T>
struct type2type {};

template <>
struct type2type<bool> {
  static constexpr value = TType::T_BOOL;
}

template <>
struct type2type<int8_t> {
  static constexpr value = TType::T_I08;
}

template <>
struct type2type<int16_t> {
  static constexpr value = TType::T_I16;
}

template <>
struct type2type<int32_t> {
  static constexpr value = TType::T_I32;
}

template <>
struct type2type<int64_t> {
  static constexpr value = TType::T_I64;
}

template <>
struct type2type<uint64_t> {
  static constexpr value = TType::T_U64;
}

template <>
struct type2type<float> {
  static constexpr value = TType::T_DOUBLE;
}

template <>
struct type2type<double> {
  static constexpr value = TType::T_DOUBLE;
}

template <>
struct type2type<std::string> {
  static constexpr value = TType::T_STRING;
}

template <typename T>
struct type2type<std::set<T>> {
  static constexpr value = TType::T_SET;
}

template <typename T>
struct type2type<std::vector<T>> {
  static constexpr value = TType::T_LIST;
}

template <typename K, typename V>
struct type2type<std::map<K, V>> {
  static constexpr value = TType::T_MAP;
}

template <typename T>
auto type2type_v(const T &) {
  return type2type<T>::value;
};

template <
  typename T,
  typename = std::void_t<>>
struct is_iterable : std::false_type {};

template <
  typename T>
struct is_iterable <
    T, std::void_t <
    decltype(declval<T>().end()),
    decltype(declval<T>().being())>> : std::true_type {};

template <
  typename T,
  typename = std::void_t<>>
struct is_hana_object : std::false_type {};

template <typename T>
struct is_hana_object <T, std::void_t<decltype(T::hana_accessors_impl)>> : std::true_type {};
	  

auto serialize(bool value, TProtocol & protocol) {
  return protocol.writeBool(value);
};

auto serialize(int8_t value, TProtocol & protocol) {
  return protocol.writeByte(value);
};

auto serialize(int16_t value, TProtocol & protocol) {
  return protocol.writeI16(value);
};

auto serialize(int32_t value, TProtocol & protocol) {
  return protocol.writeI32(value);
};

auto serialize(int64_t value, TProtocol & protocol) {
  return protocol.writeI64(value);
};

auto serialize(double value, TProtocol & protocol) {
  return protocol.writeDouble(value);
};

auto serialize(const std::string & value, TProtocol & protocol) {
  return protocol.writeString(value);
};

auto serialize_value = hana::overload_linearly(
    [](bool value, TProtocol & protocol) { return protocol.writeBool(value); },
    [](int8_t value, TProtocol & protocol) { return protocol.writeByte(value); },
    [](int16_t value, TProtocol & protocol) { return protocol.writeI16(value); },
	[](int32_t value, TProtocol & protocol) { return protocol.writeI32(value); },
	[](int64_t value, TProtocol & protocol) { return protocol.writeI64(value); },
	[](double value, TProtocol & protocol) { return protocol.writeDouble(value); },
	[](const std::string & value, TProtocol & protocol) {return protocol.writeString(value); }
);

// I don't know what to do with it yet
//auto serialize(const std::vector<int8_t> & value, TProtocol & protocol) {
//  return protocol.writeBinary(value);
//};

template <typename T>
auto serialize(const std::vector<T> & list, TProtocol & protocol) {
  auto size = protocol.writeListBegin(type2type<T>::value, std::size(list));

  for (const auto & elem : list) {
    size += serialize(elem, protocol);
  }

  return size + protocol.writeListEnd();
};

template <typename K, typename V>
auto serialize(const std::map<K, V> & map, TProtocol & protocol) {
  auto size = protocol.writeMapBegin(
    type2type<K>::value, type2type<V>::value, std::size(map));

  for (const auto & [key, value] : map) {
    size += serialize(key, protocol);
    size += serialize(value, protocol);
  }

  return size + protocol.writeMapEnd();
};


template <
  typename T,
  typename =
    typename std::enable_if<
	  is_hana_object<T>::value>::type>
auto serialize(const T && object, TProtocol & protocol)
{
  uint32_t size {};
  size += protocol.writeStructBegin(object.__name__);
  hana::for_each(hana::accessors(object), [&](auto accessor) {
    auto [meta, member] = accessor;
    auto [name, id] = meta;
	
    using op_type = typename std::result_of<member(object)>::type;
    using type = typename op_type::value_type;
	
    if (member(object).has_value()) {
      size += protocol.writeFieldBegin(name, type2type<type>::value, id);
      size += serialize(member(object).value(), protocol);
      size += protocol.writeFieldEnd();
	}
  });

  size += protocol.writeFieldStop();
  size += protocol.writeStructEnd();

  return size;
}

template <
  typename T,
  typename =
    typename std::enable_if<
	  is_hana_object<T>::value>::type>
auto deserialize(T & object, TProtocol & protocol)
{
  TType ftype;
  int16_t fid {};
  uint32_t size {};
  std::string name {};
   
  size += protocol.readStructBegin(name);
  if (!name.empty() && name != object.__name__)
    throw std::runtime_error("wrong object name");
	
  while (true) {
    size += protocol->readFieldBegin(name, ftype, fid);
	if (ftype == TType::T_STOP) {
      break;
    }
	
    hana::for_each(hana::accessors(object), [&](auto accessor) {
      auto [meta, member] = accessor;
      auto [name, fid] = meta;
	
      using op_type = typename std::result_of<member(object)>::type;
      using type = typename op_type::value_type;
	  
	  if (fid == id && ftype == type2type<type>::value)
		size += deserialize(member(object).emplace(), protocol);
	  else
		size += protocol->skip(ftype);
    });
  
    size += protocol->readFieldEnd();
  }

  size += iprot->readStructEnd();
  return size;
}
}}} // apache::thrift::protocol

#endif // #define _THRIFT_PROTOCOL_TBASESERIALIZER_H_ 1