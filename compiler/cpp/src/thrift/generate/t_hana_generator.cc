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
  std::string type_name(const t_type* ttype);

  void generate_enum(t_enum* tenum) override;
  void generate_const(t_const* tconst) override;
  void generate_struct(t_struct* tstruct) override;
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
#include <optional>
#include <algorithm>
#include <functional>

#include <boost/hana.hpp>

#include <thrift/TBase.h>
#include <thrift/Thrift.h>

{dependencies_includes}
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

/**
 * Returns the C++ type that corresponds to the thrift type.
 *
 * @param tbase The base type
 * @return Explicit C++ type, i.e. "int32_t"
 */
std::string t_hana_generator::type_name(const t_type* ttype) {
  if (auto tbase = dynamic_cast<const t_base_type*>(ttype)) {
    switch (tbase->get_base()) {
    case t_base_type::TYPE_VOID:
      return "void";
    case t_base_type::TYPE_STRING:
      return "std::string";
    case t_base_type::TYPE_BOOL:
      return "bool";
    case t_base_type::TYPE_I8:
      return "int8_t";
    case t_base_type::TYPE_I16:
      return "int16_t";
    case t_base_type::TYPE_I32:
      return "int32_t";
    case t_base_type::TYPE_I64:
      return "int64_t";
    case t_base_type::TYPE_DOUBLE:
      return "double";
    default:
      throw "compiler error: no C++ base type name for base type "
          + t_base_type::t_base_name(tbase->get_base());
    }
  }

  if (auto tmap = dynamic_cast<const t_map*>(ttype))
    return fmt::format("std::map<{key}, {value}>",
      fmt::arg("key", type_name(tmap->get_key_type())),
      fmt::arg("value", type_name(tmap->get_val_type())));

  if (auto tset = dynamic_cast<const t_set*>(ttype))
    return fmt::format("std::set<{}>", type_name(tset->get_elem_type()));

  if (auto tlist = dynamic_cast<const t_list*>(ttype))
    return fmt::format("std::vector<{}>", type_name(tlist->get_elem_type()));

  auto program = ttype->get_program();
  if (program && program != get_program())
    return make_namespace(program) + "::" + ttype->get_name();

  return ttype->get_name();
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

  fmt::print(f_types_, k_header + 1,
    fmt::arg("autogen_comment", autogen_comment()),
    fmt::arg("dependencies_includes", fmt::join(includes, "\n")));

  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_types_, k_ns_header, fmt::arg("namespace", ns));
}

void t_hana_generator::close_generator() {
  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_types_, k_ns_footer + 1, fmt::arg("namespace", ns));

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

void t_hana_generator::generate_const(t_const* tconst) {
  constexpr auto const_pattern = "extern const {type} g_{name} = {value};\n";
  constexpr auto constexpr_pattern = "extern consexpr {type} g_{name} = {value};\n";

  //const auto type = tconst->get_type();
  //if (type->is_base_type() && !type->is_string()) {
  //  fmt::print(f_types_, constexpr_pattern,
  //    fmt::arg("type", type_name(type)),
  //    fmt::arg("name", tconst->get_name()),
  //    fmt::arg("value", tconst->get_value()));
  //}
}


static constexpr auto k_struct_template = R"*(

class {type} : public ::apache::thrift::TBase {{
public:
  // TODO: hide it if class
  //       has required fields
  {type}();
  // TODO: add costructor
  //       with required fields
  ~{type}() override nothrow = default;

  bool operator < (const {type} &rhs) const;
  bool operator == (const {type} &rhs) const;
  bool operator != (const {type} &rhs) const {{
    return !(*this == rhs);
  }}

{getters}

{setters}

  // TODO: Use template reader/writer outside
  uint32_t read(protocol::TProtocol* iprot) override;
  uint32_t write(protocol::TProtocol* oprot) const override;

private:
  // All fields are optional because of that
  // they may be skipped during deserialization
{members}
}};  // class {type}

namespace boost::hana {{
  template <>
  struct accessors_impl<{type}> {{
    static BOOST_HANA_CONSTEXPR_LAMBDA auto apply() {{
      return make_tuple(
{accessors}
      );
    }}
  }};
}}  // namespace boost::hana
)*";

static constexpr auto k_struct_field_template = R"*(
  std::optional<{type}> m_{name};)*";

static constexpr auto k_struct_field_getter_template = R"*(
  auto get_{name}() const {{
    return m_{name};
  }};)*";

static constexpr auto k_struct_field_setter_template = R"*(
  auto set_{name}({type} {name}) {{
    return m_{name} = std::move({name}), *this;
  }};)*";

static constexpr auto k_struct_field_accessor_template = R"*(
        make_pair(make_tuple("{name}", {type}, {id}), [](auto&& o) {{
          return o.{name}();
        }}))*";

void t_hana_generator::generate_struct(t_struct* tstruct) {
  std::vector<std::string> members{};
  std::vector<std::string> getters{};
  std::vector<std::string> setters{};
  std::vector<std::string> accessors{};
  std::vector<std::string> required_members{};

  for (const auto member : tstruct->get_members()) {
    if (member->get_req() == t_field::T_REQUIRED) {
      required_members.push_back(member->get_name());
    }

    members.emplace_back();
    fmt::format_to(std::back_inserter(members.back()), k_struct_field_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()));

    getters.emplace_back();
    fmt::format_to(std::back_inserter(getters.back()), k_struct_field_getter_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()));

    setters.emplace_back();
    fmt::format_to(std::back_inserter(setters.back()), k_struct_field_setter_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()));

    accessors.emplace_back();
    fmt::format_to(std::back_inserter(accessors.back()), k_struct_field_accessor_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()),
      fmt::arg("id", member->get_key()));
  }

  fmt::print(f_types_, k_struct_template,
    fmt::arg("type", tstruct->get_name()),
    fmt::arg("members", fmt::join(members, "\n")),
    fmt::arg("getters", fmt::join(getters, "\n")),
    fmt::arg("setters", fmt::join(setters, "\n")),
    fmt::arg("accessors", fmt::join(accessors, ",\n")));
}

THRIFT_REGISTER_GENERATOR(hana, "C++", "    C++ powered by boost::hana\n")
