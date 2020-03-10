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
 *
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <sys/stat.h>

#include "thrift/platform.h"
#include "thrift/generate/t_oop_generator.h"

/**
 * C++ code generator that uses boost::hana library
 * in purpose of having static reflection in compile time
 */

class t_hana_generator : public t_oop_generator {
public:
  using t_options
      = std::map<std::string, std::string>;

  t_hana_generator(t_program* program,
                   const t_options& parsed_options,
                   const std::string& /*option_string*/)
    : t_oop_generator(program) {
  }

  void init_generator() override {}
  void close_generator() override {}

  void generate_enum(t_enum* tenum) override {}
  void generate_const(t_const* tconst) override {}
  void generate_struct(t_struct* tstruct) override {}
  void generate_typedef(t_typedef* ttypedef) override {}
  void generate_service(t_service* tservice) override {}
  void generate_xception(t_struct* txception) override {
    // By default exceptions are the same as structs
    generate_struct(txception);
  }
  void generate_forward_declaration(t_struct* tstruct) override {}
};

THRIFT_REGISTER_GENERATOR(hana, "C++", "    C++ powered by boost::hana\n")
