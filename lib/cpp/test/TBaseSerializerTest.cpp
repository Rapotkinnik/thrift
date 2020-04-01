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

#include <thrift/protocol/TDebugProtocol.h>
#include <thrift/protocol/TBaseSerializer.h>
#include <thrift/transport/TBufferTransports.h>

#include "gen-cpp/DebugProtoTest_types.h"

#define BOOST_TEST_MODULE SerializerTest
#include <boost/test/unit_test.hpp>

using namespace thrift::test::debug;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TDebugProtocol;
using apache::thrift::protocol::TBaseSerializer;


BOOST_AUTO_TEST_CASE(test_struct_serialization) {
  static constexpr auto expected = R"*(

  )*";++
  
  Object obj {
    // bla-bla
  };
  
  auto buffer = std::make_shared<TMemoryBuffer>();
  auto protocol = std::make_shared<TDebugProtocol>(buffer);
  
  serialize(obj, protocol);
  
  auto result = buffer->getDataAsString();
  BOOST_CHECK_MESSAGE(!result.compare(expected),
    "Expected:\n" << expected << "\nGotten:\n" << result);
}

BOOST_AUTO_TEST_CASE(test_struct_deserialization) {
  
}

BOOST_AUTO_TEST_CASE(test_message_serialization) {
  
}

BOOST_AUTO_TEST_CASE(test_message_deserialization) {
  
}