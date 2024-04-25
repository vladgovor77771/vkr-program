#pragma once

#include <string>

namespace lib::document {

enum class TypeId {
    kNull,
    kBoolean,
    kInt32,
    kUint32,
    kInt64,
    kUint64,
    kFloat32,
    kFloat64,
    kString,
    kDocument,
    kList,
};

std::string TypeIdToString(TypeId id);

class Value {
protected:
    const TypeId type_id_;

public:
    Value(TypeId type_id);
    TypeId GetTypeId() const;
};

class Null: public Value {
public:
    Null();
};

class Boolean: public Value {
public:
    const bool value;
    Boolean(bool value);
};

class Int32: public Value {
public:
    const int32_t value;
    Int32(int32_t value);
};

class Int64: public Value {
public:
    const int64_t value;
    Int64(int64_t value);
};

class UInt32: public Value {
public:
    const uint32_t value;
    UInt32(int32_t value);
};

class UInt64: public Value {
public:
    const uint64_t value;
    UInt64(int64_t value);
};

class Float32: public Value {
public:
    const float value;
    Float32(float value);
};

class Float64: public Value {
public:
    const double value;
    Float64(double value);
};

class String: public Value {
public:
    const std::string value;
    String(const std::string& value);
    String(std::string&& value);
};

using ValueMap = std::unordered_map<std::string, std::shared_ptr<document::Value>>;

class Document: public Value {
public:
    const ValueMap value;
    Document(const ValueMap& value);
    Document(ValueMap&& value);
};

using ValueList = std::vector<std::shared_ptr<Value>>;

class List: public Value {
public:
    const ValueList value;
    List(const ValueList& value);
    List(ValueList&& value);
};

} // namespace lib::document
