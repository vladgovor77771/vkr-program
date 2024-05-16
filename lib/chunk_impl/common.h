#pragma once

#include <fstream>
#include <optional>

#include <lib/chunk_impl/get_stream.h>
#include <lib/document/document.h>

namespace lib::chunk_impl {

enum class ControlChar {
    kNullFlag = 'n',     // length 0
    kBooleanFlag = 'b',  // length 4
    kInt32Flag = 'i',    // length 4
    kUint32Flag = 'u',   // length 4
    kInt64Flag = 'g',    // length 8
    kUint64Flag = 'z',   // length 8
    kFloat32Flag = 'f',  // length 4
    kFloat64Flag = 'd',  // length 8
    kStringFlag = 's',   // length vary
    kDocumentFlag = 'o', // length vary
    kListFlag = 'l',     // length vary
};

std::optional<ControlChar> ReadControlChar(IStream& stream);
bool IsPrimitiveControlChar(ControlChar cch);
std::shared_ptr<document::Value> ReadPrimitiveValue(ControlChar cch, IStream& stream);
std::vector<char> SerializePrimitiveValue(const std::shared_ptr<document::Value>& value);

uint16_t Read2Bytes(IStream& stream);
uint32_t Read4Bytes(IStream& stream);
uint64_t Read8Bytes(IStream& stream);
float ReadFloat(IStream& stream);
double ReadDouble(IStream& stream);
std::string ReadString(IStream& stream);

std::vector<char> Serialize2Bytes(uint16_t value);
std::vector<char> Serialize4Bytes(uint32_t value);
std::vector<char> Serialize8Bytes(uint64_t value);
std::vector<char> SerializeFloat(float value);
std::vector<char> SerializeDouble(double value);
std::vector<char> SerializeString(const std::string& value);

} // namespace lib::chunk_impl
