#ifndef JITSIM_CIRCUIT_HPP_INCLUDED
#define JITSIM_CIRCUIT_HPP_INCLUDED

#include <jitsim/simanalysis.hpp>
#include <jitsim/primitive.hpp>

#include <cassert>
#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <deque>
#include <utility>
#include <functional>
#include <optional>

#include <llvm/ADT/APInt.h>

namespace JITSim {

class IFace;
class Definition;
class Instance;
class ValueSlice;

class Value {
private:
  std::string name;
  int width;
public:
  Value(const std::string &name_, int w)
    : name(name_), width(w)
  {}

  const std::string & getName() const { return name; }
  int getWidth() const { return width; }
};

class ValueSlice {
private:
  const Definition *definition;
  const Instance *instance;
  const IFace *iface;
  const Value *val;
  int offset;
  int width;
  bool is_whole;
  std::optional<llvm::APInt> constant;

public:
  ValueSlice(const Definition *definition_, const Instance *instance_,
             const Value *val_, int offset_, int width_);

  ValueSlice(const std::vector<bool> &constant_);

  const Definition * getDefinition() const { return definition; }
  const Instance * getInstance() const { return instance; }
  const IFace * getIFace() const { return iface; }
  const Value * getValue() const { return val; }

  int getEndIdx() const { return offset + width; }
  int getWidth() const { return width; }
  int getOffset() const { return offset; }
  bool isWhole() const { return is_whole; }
  bool isConstant() const { return constant.has_value(); }
  bool isDefinitionAttached() const { return !!definition; }
  bool isInstanceAttached() const { return !!instance; }

  void extend(const ValueSlice &other);

  std::string repr() const;
};

class Select {
private:
  std::vector<ValueSlice> slices;

  bool has_many_slices = true;

  const ValueSlice *direct_value = nullptr;

  void compressSlices();
public:
  Select(ValueSlice &&slice_);
  Select(std::vector<ValueSlice> &&slices_);

  const std::vector<ValueSlice> & getSlices() const { return slices; }

  std::string repr() const;
};

class Input {
private:
  std::string name;
  int width;
  std::optional<Select> select;
public:
  Input(const std::string &name_, int w)
    : name(name_), width(w), select() {}

  bool isConnected() const { return select.has_value(); }
  void connect(Select && conn) { select = std::move(conn); }

  const Select& getSelect() const { return *select; }

  const std::string & getName() const { return name; }
  int getWidth() const { return width; }
};

class IFace {
private:
  std::string name;
  std::vector<Input> inputs;
  std::vector<Value> outputs;
  std::unordered_map<std::string, Input *> input_lookup;
  std::unordered_map<std::string, Value *> output_lookup;
  bool is_definition;
public:
  IFace(const std::string &name_,
        std::vector<Input> &&inputs_,
        std::vector<Value> &&outputs_,
        const bool is_definition_);

  IFace(const IFace &) = delete;
  IFace(IFace &&) = default;

  const std::string & getName() const { return name; }
  const std::vector<Value> & getOutputs() const { return outputs; }
  const std::vector<Input> & getInputs() const { return inputs; }
  std::vector<Value> & getOutputs() { return outputs; }
  std::vector<Input> & getInputs() { return inputs; }

  const Value * getOutput(const std::string &name) const;
  const Input * getInput(const std::string &name) const;
  Value * getOutput(const std::string &name);
  Input * getInput(const std::string &name);

  void print(const std::string &prefix = "") const;
  void print_connectivity(const std::string &prefix = "") const;

  bool isDefinition() const { return is_definition; }
  bool isInstance() const { return !is_definition; }
};

class Instance {
private:
  std::string name;
  IFace interface;
  const Definition *defn;
public:
  Instance(const std::string &name_, 
           std::vector<Input> &&inputs,
           std::vector<Value> &&outputs,
           const Definition *defn);

  IFace & getIFace() { return interface; }
  const IFace & getIFace() const { return interface; }
  const SimInfo & getSimInfo() const;
  const Definition & getDefinition() const { return *defn; }
  const std::string & getName() const { return name; }

  void print(const std::string &prefix = "") const;
};

class Definition {
private:
  std::string name;
  IFace interface;

  std::vector<Instance> instances;

  SimInfo siminfo;
public:
  Definition(const std::string &name,
             std::vector<Input> &&inputs,
             std::vector<Value> &&outputs,
             std::vector<Instance> &&instances,
             std::function<void (Definition&, std::vector<Instance> &instances)> make_connections);

  Definition(const std::string &name,
             std::vector<Input> &&inputs,
             std::vector<Value> &&outputs,
             const Primitive &primitive);

  Instance makeInstance(const std::string &name) const;

  const IFace & getIFace() const { return interface; }
  IFace & getIFace() { return interface; }
  const std::string & getName() const { return name; }
  const SimInfo & getSimInfo() const { return siminfo; }

  void print(const std::string &prefix = "") const;
};

class Circuit {
private:
  std::deque<Definition> definitions;
  Definition *top_defn;
public:
  Circuit(std::deque<Definition>&& defns)
    : definitions(move(defns)),
      top_defn(&definitions.back())
  {}

  void print() const;

  const std::deque<Definition>& getDefinitions() const { return definitions; }
};

}

#endif
