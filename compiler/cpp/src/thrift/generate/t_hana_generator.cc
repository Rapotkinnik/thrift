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

#include <fstream>
#include <filesystem>

#include <sys/stat.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "thrift/platform.h"
#include "thrift/generate/t_oop_generator.h"

/**
 * C++ code generator that uses boost::hana library
 * in purpose of having static reflection in compile time
 */

class t_hana_generator : public t_oop_generator {
public:
  using t_options = std::map<std::string, std::string>;

  t_hana_generator(t_program* program,
                   const t_options& parsed_options,
                   const std::string& /*option_string*/)
    : t_oop_generator(program) {}

  void init_generator() override;
  void close_generator() override;

  std::string get_out_dir() const override;

  void generate_enum(t_enum* tenum) override;
  void generate_const(t_const* tconst) override {}
  void generate_struct(t_struct* tstruct) override {}
  void generate_typedef(t_typedef* ttypedef) override {}
  void generate_service(t_service* tservice) override {}
  void generate_xception(t_struct* txception) override {
    // By default exceptions are the same as structs
    generate_struct(txception);
  }
  void generate_forward_declaration(t_struct* tstruct) override {}

private:
  std::string s_namespace_;
  std::ofstream f_types_;
  std::ofstream f_types_impl_;
  ofstream_with_content_based_conditional_update f_header_;
  ofstream_with_content_based_conditional_update f_service_;
};

static constexpr auto k_header = R"*(
{autogen_comment}
#pragma once

#include <memory>
#include <ostream>
#include <algorithm>
#include <functional>

#include <boost/hana.hpp>

#include <thrift/TToString.h>

{dependencies_includes}

namespace hana = boost::hana;
)*";

static constexpr auto k_ns_header = R"*(
namespace {namespace} {{
)*";

static constexpr auto k_ns_footer = R"*(
}}  // {namespace}
)*";

auto make_prefix(const t_program* program) {
  auto ns = program->get_namespace("cpp");
  return std::replace(ns.begin(), ns.end(), '.', '/'), ns;
};

auto make_namespace(const t_program* program) {
  auto ns = program->get_namespace("cpp");

  std::string result{};
  std::string::size_type loc;
  while ((loc = ns.find(".")) != std::string::npos) {
    result += ns.substr(0, loc);
    result += "::";
    ns = ns.substr(loc + 1);
  }

  return result += ns.substr(0, loc);
};

std::string t_hana_generator::get_out_dir() const {
  auto prefix = make_prefix(program_);
  auto out_path = program_->get_out_path();

  if (!prefix.empty())
    out_path += prefix + '/';

  return out_path;
}

void t_hana_generator::init_generator() {
  using std::filesystem::create_directories;

  create_directories(get_out_dir());
  f_types_.open(get_out_dir() + "types.h");
  f_types_impl_.open(get_out_dir() + "types.cpp");

  std::vector<std::string> includes {};
  for (const auto program : program_->get_includes()) {
    includes.emplace_back();
    fmt::format_to(std::back_inserter(includes.back()),
      R"*(#include "{}/types.h")*", make_prefix(program));
  }

  fmt::print(f_types_, k_header,
    fmt::arg("autogen_comment", autogen_comment()),
    fmt::arg("dependencies_includes", fmt::join(includes, "\n")));

  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_types_, k_ns_header, fmt::arg("namespace", ns));
}

void t_hana_generator::close_generator() {
  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_types_, k_ns_footer, fmt::arg("namespace", ns));

  f_types_.close();
}

// TODO: Move definition to the impl file
static constexpr auto k_enum = R"*(
enum class {type} {{
  {values}
}};

namespace std {{
inline constexpr auto to_string({type} {name}) {{
  static constexpr char * {name}2string[] = {{
    {indexes}
  }};

  return {name}2string[{name}];
}};
}}  // namespace std

inline std::ostream& operator<<(std::ostream& out, {type} {name}) {{
  return out << std::to_string({name});
}};
)*";

void t_hana_generator::generate_enum(t_enum* tenum) {
  constexpr auto kvp_pattern = "{0} = {1}";
  constexpr auto index_pattern = R"*([{1}::{0}] = "{0}")*";

  std::vector<std::string> values {};
  std::vector<std::string> indexes {};
  auto name = lowercase(tenum->get_name());

  for (const auto kvp : tenum->get_constants()) {
    values.push_back(fmt::format(kvp_pattern, kvp->get_name(), kvp->get_value()));
    indexes.push_back(fmt::format(index_pattern, kvp->get_name(), tenum->get_name()));
  }

  fmt::print(f_types_, k_enum,
    fmt::arg("name", name),
    fmt::arg("type", tenum->get_name()),
    fmt::arg("values", fmt::join(values, ",\n  ")),
    fmt::arg("indexes", fmt::join(indexes, ",\n    ")));
}

THRIFT_REGISTER_GENERATOR(hana, "C++", "    C++ powered by boost::hana\n")
