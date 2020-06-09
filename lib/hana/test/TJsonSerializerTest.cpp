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

#include <memory>
#include <sstream>

#include <thrift/protocol/TJsonSerializer.h>

#include "gen-cpp/DebugProtoTest_types.h"

#define BOOST_TEST_MODULE JsonSerializerTest
#include <boost/test/unit_test.hpp>

using namespace thrift::test::debug;
using namespace apache::thrift::protocol;

BOOST_AUTO_TEST_CASE(test_empty_struct_to_json_serialization) {
  Empty obj{};
  const auto expected = "{}";
  const auto result = to_pretty_json(obj);

  BOOST_CHECK_MESSAGE(!result.compare(expected),
    "Expected:\n" << expected << "\nGotten:\n" << result);
}

BOOST_AUTO_TEST_CASE(test_bonk_struct_to_json_serialization) {
  Bonk obj{};
  obj.type = 1;
  obj.message = "test";

  const auto expected = R"*({
  "type": 1,
  "message": "test"
})*";
  const auto result = to_pretty_json(obj);

  BOOST_CHECK_MESSAGE(!result.compare(expected),
    "Expected:\n" << expected << "\nGotten:\n" << result);
}

BOOST_AUTO_TEST_CASE(test_one_of_each_struct_to_json_serialization) {
  OneOfEach obj{};
  obj.im_true = true;
  obj.im_false = false;
  obj.integer32 = 4242;
  obj.double_precision = 42.42;
  obj.some_characters = "some";
//  obj.zomg_unicode = u8"unicode";

  const auto expected = R"*({
  "im_true": true,
  "im_false": false,
  "a_bite": 0x7f,
  "integer16": 0x7fff,
  "integer32": 4242,
  "integer64": 10000000000,
  "double_precision": 42.42,
  "some_characters": some,
  "byte_list": [1,2,3],
  "i16_list": [1,2,3],
  "i64_list": [1,2,3]
})*";
  const auto result = to_pretty_json(obj);

  BOOST_CHECK_MESSAGE(!result.compare(expected),
                      "Expected:\n" << expected << "\nGotten:\n" << result);
}

//BOOST_AUTO_TEST_CASE(test_nested_struct_to_json_serialization) {
//  Object obj {};
//  constexpr auto expected = "{}";

//  std::stringstream ss{};
//  ss << TJsonSerializer{obj}.pretty();

//  BOOST_CHECK_MESSAGE(!ss.str().compare(expected),
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_all_in_one_struct_to_json_serialization) {
//  OneOfEach obj {};
//  constexpr auto expected = "{}";

//  std::stringstream ss{};
//  ss << TJsonSerializer{obj}.pretty();

//  BOOST_CHECK_MESSAGE(!ss.str().compare(expected),
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_map_of_structs_to_json_serialization) {
//  Object obj {};
//  constexpr auto expected = "{}";

//  std::stringstream ss{};
//  ss << TJsonSerializer{obj}.pretty();

//  BOOST_CHECK_MESSAGE(!ss.str().compare(expected),
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_list_of_structs_to_json_serialization) {
//  Object obj {};
//  constexpr auto expected = "{}";

//  std::stringstream ss{};
//  ss << TJsonSerializer{obj}.pretty();

//  BOOST_CHECK_MESSAGE(!ss.str().compare(expected),
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_empty_struct_from_json_deserialization) {
//  Object result {};
//  Object expected {};

//  std::stringstream ss{
//    R"*({"name": "whatever"})*"};
//  TJsonSerializer{}.deserialize(obj, ss);

//  BOOST_CHECK_MESSAGE(result == expected,
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_nested_struct_from_json_deserialization) {
//  Object result {};
//  Object expected {};

//  auto json = R"*({
//    "empty": {},
//      "nested": {
//        "bla": 2,
//        "bla": 2
//      }
//  })*";
//  std::stringstream ss{json};
//  TJsonSerializer{}.deserialize(obj, ss);

//  BOOST_CHECK_MESSAGE(result == expected,
//    "Expected:\n" << expected << "\nGotten:\n" << ss.str());
//}

//BOOST_AUTO_TEST_CASE(test_message_json_serialization) {
//}

//BOOST_AUTO_TEST_CASE(test_message_json_deserialization) {
//}
