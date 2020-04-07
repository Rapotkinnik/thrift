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
#include <utility>
#include <type_traits>

#include <boost/hana.hpp>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TProtocolException.h>

namespace apache {
namespace thrift {
namespace protocol {

using namespace boost;
using apache::thrift::protocol::TType;
using apache::thrift::protocol::TProtocol;
//T_STOP       = 0,
//    T_VOID       = 1,
//    T_BOOL       = 2,
//    T_BYTE       = 3,
//    T_I08        = 3,
//    T_I16        = 6,
//    T_I32        = 8,
//    T_U64        = 9,
//    T_I64        = 10,
//    T_DOUBLE     = 4,
//  T_STRING     = 11,
//  T_UTF7       = 11,
//  T_STRUCT     = 12,
//  T_MAP        = 13,
//  T_SET        = 14,
//  T_LIST       = 15,
//  T_UTF8       = 16,
//  T_UTF16      = 17

//hana::make_map(
//template <TType v>
//using type2enum = std::integral_constant<TType, v>;

//type2enum<TType::T_BOOL>

template <typename T>
struct type2type {};

template <>
struct type2type<bool> {
  static constexpr auto value = TType::T_BOOL;
};

template <>
struct type2type<int8_t> {
  static constexpr auto value = TType::T_I08;
};

template <>
struct type2type<int16_t> {
  static constexpr auto value = TType::T_I16;
};

template <>
struct type2type<int32_t> {
  static constexpr auto value = TType::T_I32;
};

template <>
struct type2type<int64_t> {
  static constexpr auto value = TType::T_I64;
};

template <>
struct type2type<uint64_t> {
  static constexpr auto value = TType::T_U64;
};

template <>
struct type2type<float> {
  static constexpr auto value = TType::T_DOUBLE;
};

template <>
struct type2type<double> {
  static constexpr auto value = TType::T_DOUBLE;
};

template <>
struct type2type<std::string> {
  static constexpr auto value = TType::T_STRING;
};

template <typename T>
struct type2type<std::set<T>> {
  static constexpr auto value = TType::T_SET;
};

template <typename T>
struct type2type<std::vector<T>> {
  static constexpr auto value = TType::T_LIST;
};

template <typename K, typename V>
struct type2type<std::map<K, V>> {
  static constexpr auto value = TType::T_MAP;
};

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
struct is_iterable <T, std::void_t <
    decltype(std::declval<T>().end()),
    decltype(std::declval<T>().being())>> : std::true_type {};

template <
  typename T,
  typename = std::void_t<>>
struct is_hana_object : std::false_type {};

template <typename T>
struct is_hana_object <T, std::void_t<decltype(T::hana_accessors_impl)>> : std::true_type {};

}}}  // namespace apache::thrift::protocol
