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

#include <boost/hana.hpp>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TTypeTraits.h>
#include <thrift/protocol/TProtocolException.h>

namespace apache {
namespace thrift {
namespace protocol {

namespace hana = boost::hana;
using namespace hana::literals;
using apache::thrift::protocol::TType;
using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TProtocolException;

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

auto deserialize_value = hana::overload_linearly(
  [](bool &value, TProtocol & protocol) { return protocol.readBool(value); },
  [](int8_t &value, TProtocol & protocol) { return protocol.readByte(value); },
  [](int16_t &value, TProtocol & protocol) { return protocol.readI16(value); },
  [](int32_t &value, TProtocol & protocol) { return protocol.readI32(value); },
  [](int64_t &value, TProtocol & protocol) { return protocol.readI64(value); },
  [](double &value, TProtocol & protocol) { return protocol.readDouble(value); },
  [](std::string & value, TProtocol & protocol) {return protocol.readString(value); }
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


template <typename T>
class TProtSerializer {
 public:
  explicit TProtSerializer(const T & object)
    : object_(object) {};


  auto & write(TProtocol & out) {
    serialize(object_, out);
    return *this;
  }

private:
  template <typename T>
  auto serialize(const T & value, TProtocol & out) {
    return serialize_value(value, out);
  }

  template <typename T, typename U>
  auto serialize(const std::pair<T, U> & pair, TProtocol & out) {
    return serialize(pair.first, out)
         + serialize(pair.second, out);
  }

  template <typename T, typename ElemTag>
  auto serialize(const T & iterable, TProtocol & out, TTypeTag<TType::T_SET>, ElemTag) {
    auto size = protocol.writeSetBegin(ElemTag::type, std::size(iterable));
    for (const auto & elem : iterable)
      size += serialize(elem, out);

    return size
         + out.writeSetEnd();
  }

  template <typename T, typename ElemTag>
  auto serialize(const T & iterable, TProtocol & out, TTypeTag<TType::T_LIST>, ElemTag) {
    auto size = protocol.writeListBegin(ElemTag::type, std::size(iterable));
    for (const auto & elem : iterable)
      size += serialize(elem, out);

    return size
         + out.writeListEnd();
  }

  template <typename T, typename KeyTag, typename ValueTag>
  auto serialize(const T & iterable, TProtocol & out, TTypeTag<TType::T_MAP>, KeyTag, ValueTag) {
    auto size = protocol.writeMapBegin(KeyTag::type, ValueTag::type, std::size(iterable));
    for (const auto & [key, value] : iterable) {
      size += serialize(key, out);
      size += serialize(value, out);
    }

    return size
         + out.writeMapEnd();
  }

//  template <typename T>
//  auto serialize(const T & iterable, TProtocol & out, TTypeTag<TType::T_LIST>,
//                 typename std::enable_if<is_iterable<T>::value>::type * = 0) {
//    uint32_t size{};
//    // TODO: Find out what Container is...
//    //       auto size = protocol.write<Container>Begin(..., std::size(map));
//    for (const auto & elem : iterable)
//      size += serialize(elem, out);

//    return size;  // TODO: + protocol.write<Container>End();
//  }

  template <typename T>
  auto & serialize(const T & object, TProtocol & out,
                  typename std::enable_if<is_hana_object<T>::value>::type * = 0) {
    uint32_t size = protocol.writeStructBegin(object.__name__);
    hana::for_each(hana::accessors<T>(object), [&](const auto & accessor) {
      auto meta = hana::first(accessor);
      auto member = hana::second(accessor);

//      using op_type = typename std::result_of<member(object)>::type;
//      using type = typename op_type::value_type;

//      if (member(object).has_value()) {
//        size += protocol.writeFieldBegin(hana::first(meta), /*type2type<type>::value*/, id);
//        size += serialize(member(object).value(), out);
//        size += protocol.writeFieldEnd();
//      }

      if (member(object).has_value()) {
        constexpr auto id = meta[1_c];
        constexpr auto name = meta[0_c];
        constexpr auto type = hana::front(meta[2_c]);
        constexpr auto rest = hana::drop_front(meta[2_c])

        size += protocol.writeFieldBegin(name, hana::tag_of_t(type), id);
        size += serialize(member(object).value(), out, rest);
        size += protocol.writeFieldEnd();
      }
    });

    return size
         + protocol.writeFieldStop()
         + protocol.writeStructEnd();
  }

  const T & object_;
};  // class TProtSerializer

template <typename T>
inline auto & operator << (TProtocol & out, const TProtSerializer<T> & ser) {
  return ser.write(out), out;
}

template <typename T>
class TJsonDeserializer {
public:
  explicit TJsonDeserializer(T & object)
    : object_(object) {};

  auto & read(TProtocol & in) {
    deserialize(object_, in);
    return *this;
  }

  auto & object() {
    return object_;
  }

private:
  template <typename T>
  auto & deserialize(T & value, TProtocol & in) {
    return deserialize_value(value, in);
  }

  template <typename T, typename U>
  auto & deserialize(std::pair<T, U> & pair, TProtocol & in) {
    return deserialize(pair.first, out)
         + deserialize(pair.second, out);
  }

  template <typename T>
  auto & deserialize(T & iterable, TProtocol & in,
                     typename std::enable_if<is_iterable<T>::value>::type * = 0) {
    // TODO: Implement
  }

  template <typename T>
  auto deserialize(T & object, TProtocol & in,
                   typename std::enable_if<is_hana_object<T>::value>::type * = 0)
  {
    TType ftype;
    int16_t fid {};
    std::string name {};

    auto size = protocol.readStructBegin(name);
    if (!name.empty() && name != object.__name__)
      throw std::runtime_error("wrong object name");  // TODO: use TProtocolException

    while (true) {
      size += protocol->readFieldBegin(name, ftype, fid);
      if (ftype == TType::T_STOP) {
        break;
      }

      auto skip = true;
      // TODO: use hana::find_if instead of hana::for_each
      hana::for_each(hana::accessors(object), [&](auto & accessor) {
        auto meta = hana::first(accessor);
        auto member = hana::second(accessor);

        constexpr auto id = meta[1_c];
        constexpr auto type = meta[2_c][0_c];

        if (fid == id && ftype == hana::tag_of_t(type)) {
          skip = false;
          size += deserialize(member(object).emplace(), in);
        }

//        using op_type = typename std::result_of<member(object)>::type;
//        using type = typename op_type::value_type;

//        if (fid == id && ftype == type2type<type>::value) {
//          skip = false;
//          size += deserialize(member(object).emplace(), in);
//        }
      });

      if (skip) {
        size += protocol->skip(ftype);
      }

      size += protocol->readFieldEnd();
    }

    // TODO: Check if all required field are filled
    //       and throw TProtocolException if they are not

    return size
         + iprot->readStructEnd();
  }

private:
  T & object_;
};  // class TProtDeserializer

template <typename T>
inline auto & operator >> (TProtocol & in, const TProtDeserializer<T> & ser) {
  return ser.read(in), in;
}
}}} // apache::thrift::protocol
