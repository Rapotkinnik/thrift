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

namespace hana = boost::hana;

namespace apache {
namespace thrift {
namespace protocol {

template <TType T>
struct TTypeTag {
  static constexpr auto value = T;
};  // struct TTypeTag

template <
  TType T,
  TType ...Rs>
struct TMetaInfo {
  const int16_t id {};
  const char * const name {};

  using Type = TTypeTag<T>;
  using Tags = hana::tuple<TTypeTag<Rs>...>;

  const Type type {};
  const Tags tags {};

  constexpr TMetaInfo(int16_t id, const char * name)
    : id(id)
    , name(name)
  {
  }
};  // struct TMetaInfo

template <
  typename T,
  typename = std::void_t<>>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<
  decltype(std::declval<T>().end()),
  decltype(std::declval<T>().being())>> : std::true_type {};

template <
  typename T,
  typename = std::void_t<>>
struct is_hana_object : std::false_type {};

template <typename T>
struct is_hana_object<T, std::void_t<decltype(T::hana_accessors_impl)>> : std::true_type {};

}}}  // namespace apache::thrift::protocol
