#include "document.h"

namespace lib::document {

Value::Value(TypeId type_id)
    : type_id_(type_id) {
}

TypeId Value::Id() const {
    return type_id_;
}

Null::Null()
    : Value(TypeId::kNull) {
}

Boolean::Boolean(bool value)
    : Value(TypeId::kBoolean)
    , value(value) {
}

Int32::Int32(int32_t value)
    : Value(TypeId::kInt32)
    , value(value) {
}

Int64::Int64(int64_t value)
    : Value(TypeId::kInt64)
    , value(value) {
}

UInt32::UInt32(int32_t value)
    : Value(TypeId::kUint32)
    , value(value) {
}

UInt64::UInt64(int64_t value)
    : Value(TypeId::kUint64)
    , value(value) {
}

Float32::Float32(float value)
    : Value(TypeId::kFloat32)
    , value(value) {
}

Float64::Float64(double value)
    : Value(TypeId::kFloat64)
    , value(value) {
}

String::String(const std::string& value)
    : Value(TypeId::kString)
    , value(value) {
}

String::String(std::string&& value)
    : Value(TypeId::kString)
    , value(std::move(value)) {
}

Document::Document(const ValueMap& value)
    : Value(TypeId::kDocument)
    , value(value) {
}

Document::Document(ValueMap&& value)
    : Value(TypeId::kDocument)
    , value(std::move(value)) {
}

List::List(const ValueList& value)
    : Value(TypeId::kList)
    , value(value) {
}

List::List(ValueList&& value)
    : Value(TypeId::kList)
    , value(std::move(value)) {
}

} // namespace lib::document
