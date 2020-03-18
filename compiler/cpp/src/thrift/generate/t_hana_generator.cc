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
  void generate_typedef(t_typedef* ttypedef) override;
  void generate_service(t_service* tservice) override;
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
struct {type} {{
  // All fields are optional because of that
  // they may be skipped during deserialization
{members}

  bool operator < (const {type} &rhs) const;
  bool operator == (const {type} &rhs) const;
  bool operator != (const {type} &rhs) const {{
    return !(*this == rhs);
  }}

  struct hana_accessors_impl {{
    static BOOST_HANA_CONSTEXPR_LAMBDA auto apply() {{
      return make_tuple(
{accessors}
      );
    }}
  }};
}}  // struct {type};

class {type}Builder {{
 public:
  {type}Builder({params});

{setters}

  operator const {type} &() const {{
    return m_inner;
  }}
  auto make() const {{
    return m_inner;
  }}

 private:
  {type} m_inner {{}};
}};  // struct {type}Builder
)*";

static constexpr auto k_struct_field_template = R"*(
  std::optional<{type}> m_{name};)*";

static constexpr auto k_struct_field_setter_template = R"*(
  auto & set_{name}({type} {name}) {{
    return m_inner.{name} = std::move({name}), *this;
  }};)*";

static constexpr auto k_struct_field_accessor_template = R"*(
        make_pair(make_pair("{name}", {id}), [](auto&& o) {{ return o.{name}; }}))*";

void t_hana_generator::generate_struct(t_struct* tstruct) {
  std::vector<std::string> params{};
  std::vector<std::string> members{};
  std::vector<std::string> setters{};
  std::vector<std::string> accessors{};

  for (const auto member : tstruct->get_members()) {
    if (member->get_req() == t_field::T_REQUIRED) {
      params.emplace_back();
      fmt::format_to(std::back_inserter(params.back()), "{type} {name}",
        fmt::arg("type", type_name(member->get_type())),
        fmt::arg("name", member->get_name()));
    }

    members.emplace_back();
    fmt::format_to(std::back_inserter(members.back()), k_struct_field_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()));

    setters.emplace_back();
    fmt::format_to(std::back_inserter(setters.back()), k_struct_field_setter_template + 1,
      fmt::arg("type", type_name(member->get_type())),
      fmt::arg("name", member->get_name()));

    accessors.emplace_back();
    fmt::format_to(std::back_inserter(accessors.back()), k_struct_field_accessor_template + 1,
      fmt::arg("name", member->get_name()),
      fmt::arg("id", member->get_key()));
  }

  fmt::print(f_types_, k_struct_template,
    fmt::arg("type", tstruct->get_name()),
    fmt::arg("params", fmt::join(params, ", ")),
    fmt::arg("members", fmt::join(members, "\n")),
    fmt::arg("setters", fmt::join(setters, "\n")),
    fmt::arg("accessors", fmt::join(accessors, ",\n")));
}

void t_hana_generator::generate_typedef(t_typedef* ttypedef) {
  using namespace fmt::literals;
  if (ttypedef->is_forward_typedef()) {
    fmt::print(f_types_, "struct {type};\n",
      "type"_a = ttypedef->get_symbolic());
  } else {
    fmt::print(f_types_, "using {name} = {type};\n",
      "type"_a = type_name(ttypedef->get_type()),
      "name"_a = ttypedef->get_symbolic());
  }
}

static constexpr auto k_service_iface_template = R"*(
class {type}If {{
 public:
  virtual ~{type}If() = default;
{methods}
}};  // class {type}If

class {type}IfFactory {{
  public:
   using Handler = {type}If;
   using HandlerPtr = {type}If*;
   using ConnectionInfo = ::apache::thrift::TConnectionInfo;

   virtual ~{type}IfFactory() = default;
   virtual HandlerPtr getHandler(const ConnectionInfo& /*connInfo*/) = 0;
   virtual void releaseHandler(HandlerPtr /*handler*/) = 0;
}}; // class {type}IfFactory

class {type}IfSingletoneFactory : public {type}IfFactory {{
 public:
  using HandlerSharedPtr = std::shader_ptr<Handler>;

  {type}IfSingletoneFactory(HandlerSharedPtr iface)
    : iface_(std::move(iface)) {{
  }}
  virtual ~{type}IfSingletoneFactory() override = default;

  virtual HandlerPtr getHandler(const ConnectionInfo& /*connInfo*/) {{
    return iface_.get();
  }}
  virtual void releaseHandler(HandlerPtr /*handler*/) {{}}

 protected:
   HandlerSharedPtr iface_;
}};  // class {type}IfSingletoneFactory

class {type}Null : public {type}If {{
 public:
  virtual ~{type}Null() override = default;

{null_methods}
}};  // class {type}Null
)*";

static constexpr auto k_service_client_template = R"*(
class {type}Client : public {type}if {{
 public:
  using ProtocolPtr = std::shared_ptr<::apache::thrift::protocol::TProtocol>;

  {type}Client(ProtocolPtr prot)
    : input_prot_(prot)
    , output_prot_(prot) {{
  }}
  {type}Client(ProtocolPtr input_prot, ProtocolPtr output_prot)
    : input_prot_(input_prot)
    , output_prot_(output_prot) {{
  }}
  ~{type}Client() override = default;

  // This methods are more dangerous than useful
  //void setInputProtocol(ProtocolPtr prot) {{
  //  input_prot_ = prot;
  //}}
  //void getOutputProtocol(ProtocolPtr prot) {{
  //  output_prot_ = prot;
  //}}

  auto getInputProtocol() {{
    return input_prot_;
  }}
  auto getOutputProtocol() {{
    return output_prot_;
  }}

  // Client methods
{methods}

 private:
  ProtocolPtr input_prot_;
  ProtocolPtr output_prot_;
}};  // class {type}Cleint

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class {type}ConcurrentClient : public {type}If {
 public:
  using ProtocolPtr = std::shared_ptr<::apache::thrift::protocol::TProtocol>;

  {type}ConcurrentClient(ProtocolPtr prot)
    : input_prot_(prot)
    , output_prot_(prot) {{
  }}
  {type}ConcurrentClient(ProtocolPtr input_prot, ProtocolPtr output_prot)
    : input_prot_(input_prot)
    , output_prot_(output_prot) {{
  }}
  ~{type}ConcurrentClient() override = default;
}};  // class ConcurrentClient
)*";

static constexpr auto k_service_processor_template = R"*(
class {type}Processor : public ::apache::thrift::TDispatchProcessor {
 public:
  using Handler = {type}If;
  using HandlerPtr = std::shared_ptr<{type}If>;
  using ProtocolPtr = ::apache::thrift::protocol::TProtocol*;

  {type}Processor(HandlerPtr iface)
    : iface_(std::move(iface)) {{
{process_map}
  }}
  ~{type}Processor() override = defauld;
  auto getHandler() {{
    return iface_;
  }}

 protected:
  bool dispatchCall(ProtocolPtr iprot, ProtocolPtr oprot, const std::string& fname, int32_t seqid, void* callContext) override;

 private:
  using ProcessFunction = void ({type}Processor::*)(int32_t, ProtocolPtr, ProtocolPtr, void*)>;
  using ProcessMap = std::map<std::string, ProcessFunction>;

  HandlerPtr iface_;
  ProcessMap processMap_;

{process_methods}
}};  // class {type}Processor

class {type}ProcessorFactory : public ::apache::thrift::TProcessorFactory {{
 public:
  using IfFactoryPtr = std::shared_ptr<{type}IfFactory>;
  using ProcessorPtr = shared_ptr<::apache::thrift::TProcessor>;
  using ConnectionInfo = ::apache::thrift::TConnectionInfo;

  {type}ProcessorFactory(IfFactoryPtr handlerFactory)
    : handlerFactory_(std::move(handlerFactory)) {{
  }}

  ProcessorPtr getProcessor(const ConnectionInfo& connInfo);

 protected:
  IfFactoryPtr handlerFactory_;
}};  // class {type}ProcessorFactory
)*";

static constexpr auto k_process_map_template = R"*(
    processMap_["{func}"] = &{type}Processor::process_{func};
)*";

static constexpr auto k_process_method_template = R"*(
    void process_{name}(int32_t seqid, ProtocolPtr iprot, ProtocolPtr oprot, void* connCtx);
)*";

static constexpr auto k_service_method_template = R"*(
  virtual {return} {name}({params}) = 0;
)*";

static constexpr auto k_service_null_method_template = R"*(
  {return} {name}({params}) override {{}}
)*";

void t_hana_generator::generate_service(t_service* tservice) {
  using std::filesystem::create_directories;

  create_directories(get_out_dir());
  std::ofstream f_service {fmt::format("{}/{}.h", get_out_dir(), tservice->get_name())};
  std::ofstream f_service_impl {fmt::format("{}/{}.cpp", get_out_dir(), tservice->get_name())};

  fmt::print(f_service, k_header + 1,
    fmt::arg("autogen_comment", autogen_comment()),
    fmt::arg("dependencies_includes",
      fmt::format(R"*(#include "{}/types.h")*", make_prefix(program_))));

  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_service, k_ns_header, fmt::arg("namespace", ns));

  fmt::print(f_service, k_service_iface_template,
    fmt::arg("type", tservice->get_name()),
    fmt::arg("methods", ""),
    fmt::arg("null_methods", ""));

  fmt::print(f_service, k_service_client_template,
    fmt::arg("type", tservice->get_name()),
    fmt::arg("methods", ""));

  if (auto ns = make_namespace(program_); !ns.empty())
    fmt::print(f_service, k_ns_footer + 1, fmt::arg("namespace", ns));
}

THRIFT_REGISTER_GENERATOR(hana, "C++", "    C++ powered by boost::hana\n")
