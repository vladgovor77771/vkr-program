#include "document.h"

namespace lib::document {

std::string TypeIdToString(TypeId id) {
    switch (id) {
        case TypeId::kNull:
            return "null";
        case TypeId::kBoolean:
            return "boolean";
        case TypeId::kInt32:
            return "int32";
        case TypeId::kUint32:
            return "uint32";
        case TypeId::kInt64:
            return "int64";
        case TypeId::kUint64:
            return "uint64";
        case TypeId::kFloat32:
            return "float32";
        case TypeId::kFloat64:
            return "float64";
        case TypeId::kString:
            return "string";
        case TypeId::kDocument:
            return "document";
        case TypeId::kList:
            return "list";
    }
}

Value::Value(TypeId type_id)
    : type_id_(type_id) {
}

TypeId Value::GetTypeId() const {
    return type_id_;
}

bool Value::IsOfPrimitiveType() const {
    return type_id_ < TypeId::kDocument;
}

bool Value::IsNull() const {
    return type_id_ == TypeId::kNull;
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

Document::Document()
    : Value(TypeId::kDocument) {
}

Document::Document(const ValueMap& value)
    : Value(TypeId::kDocument)
    , value(value) {
}

Document::Document(ValueMap&& value)
    : Value(TypeId::kDocument)
    , value(std::move(value)) {
}

List::List()
    : Value(TypeId::kList) {
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
