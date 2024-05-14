#include "common.h"

#include <memory>
#include <vector>

namespace lib::chunk_impl {

std::optional<ControlChar> ReadControlChar(std::istream& stream) {
    char ch;
    if (!stream.get(ch)) {
        return std::nullopt;
    }
    return static_cast<ControlChar>(ch);
}

bool IsPrimitiveControlChar(ControlChar cch) {
    switch (cch) {
        case ControlChar::kNullFlag:
        case ControlChar::kBooleanFlag:
        case ControlChar::kInt32Flag:
        case ControlChar::kUint32Flag:
        case ControlChar::kInt64Flag:
        case ControlChar::kUint64Flag:
        case ControlChar::kFloat32Flag:
        case ControlChar::kFloat64Flag:
        case ControlChar::kStringFlag:
            return true;
        default:
            return false;
    }
}

std::shared_ptr<document::Value> ReadPrimitiveValue(ControlChar cch, std::istream& stream) {
    switch (cch) {
        case ControlChar::kNullFlag:
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Null>());
        case ControlChar::kBooleanFlag: {
            char ch;
            stream.get(ch);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Boolean>(static_cast<bool>(ch)));
        }
        case ControlChar::kInt32Flag: {
            auto val = static_cast<int32_t>(Read4Bytes(stream));
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Int32>(val));
        }
        case ControlChar::kUint32Flag: {
            auto val = Read4Bytes(stream);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt32>(val));
        }
        case ControlChar::kInt64Flag: {
            auto val = static_cast<int64_t>(Read8Bytes(stream));
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Int64>(val));
        }
        case ControlChar::kUint64Flag: {
            auto val = Read8Bytes(stream);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt64>(val));
        }
        case ControlChar::kFloat32Flag: {
            auto val = ReadFloat(stream);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Float32>(val));
        }
        case ControlChar::kFloat64Flag: {
            auto val = Read8Bytes(stream);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Float64>(val));
        }
        case ControlChar::kStringFlag: {
            auto val = ReadString(stream);
            return std::static_pointer_cast<document::Value>(std::make_shared<document::String>(std::move(val)));
        }
        case ControlChar::kDocumentFlag:
        case ControlChar::kListFlag:
            throw std::runtime_error("Not primitive value");
    }
}

std::vector<char> SerializePrimitiveValue(const std::shared_ptr<document::Value>& value) {
    std::vector<char> result;

    switch (value->GetTypeId()) {
        case document::TypeId::kNull:
            result.emplace_back((char)ControlChar::kNullFlag);
            return result;
        case document::TypeId::kBoolean:
            result.emplace_back((char)ControlChar::kBooleanFlag);
            result.emplace_back((char)static_cast<char>(std::static_pointer_cast<document::Boolean>(value)->value));
            return result;
        case document::TypeId::kInt32: {
            result.emplace_back((char)ControlChar::kInt32Flag);
            auto serialized = Serialize4Bytes(static_cast<uint32_t>(std::static_pointer_cast<document::Int32>(value)->value));
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kUint32: {
            result.emplace_back((char)ControlChar::kUint32Flag);
            auto serialized = Serialize4Bytes(std::static_pointer_cast<document::UInt32>(value)->value);
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kInt64: {
            result.emplace_back((char)ControlChar::kInt64Flag);
            auto serialized = Serialize8Bytes(static_cast<uint64_t>(std::static_pointer_cast<document::Int64>(value)->value));
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kUint64: {
            result.emplace_back((char)ControlChar::kUint64Flag);
            auto serialized = Serialize8Bytes(std::static_pointer_cast<document::UInt64>(value)->value);
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kFloat32: {
            result.emplace_back((char)ControlChar::kFloat32Flag);
            auto serialized = SerializeFloat(std::static_pointer_cast<document::Float32>(value)->value);
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kFloat64: {
            result.emplace_back((char)ControlChar::kFloat64Flag);
            auto serialized = SerializeDouble(std::static_pointer_cast<document::Float64>(value)->value);
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kString: {
            result.emplace_back((char)ControlChar::kStringFlag);
            auto serialized = SerializeString(std::static_pointer_cast<document::String>(value)->value);
            result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
            return result;
        }
        case document::TypeId::kDocument:
        case document::TypeId::kList:
            throw std::runtime_error("Not primitive value");
    }
}

uint64_t Read8Bytes(std::istream& stream) {
    std::uint64_t result = 0;
    char buffer[8];

    if (!stream.read(buffer, 8)) {
        throw std::runtime_error("Failed to read 8 bytes from stream.");
    }

    result = static_cast<uint64_t>(static_cast<uint8_t>(buffer[0])) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[1])) << 8) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[2])) << 16) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[3])) << 24) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[4])) << 32) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[5])) << 40) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[6])) << 48) |
             (static_cast<uint64_t>(static_cast<uint8_t>(buffer[7])) << 56);

    return result;
}

uint16_t Read2Bytes(std::istream& stream) {
    std::uint16_t result = 0;
    char buffer[2];

    if (!stream.read(buffer, 2)) {
        throw std::runtime_error("Failed to read 2 bytes from stream.");
    }

    result = static_cast<uint8_t>(buffer[0]) |
             (static_cast<uint8_t>(buffer[1]) << 8);

    return result;
}

uint32_t Read4Bytes(std::istream& stream) {
    std::uint32_t result = 0;
    char buffer[4];

    if (!stream.read(buffer, 4)) {
        throw std::runtime_error("Failed to read 4 bytes from stream.");
    }

    result = static_cast<uint8_t>(buffer[0]) |
             (static_cast<uint8_t>(buffer[1]) << 8) |
             (static_cast<uint8_t>(buffer[2]) << 16) |
             (static_cast<uint8_t>(buffer[3]) << 24);

    return result;
}

float ReadFloat(std::istream& stream) {
    auto temp = Read4Bytes(stream);
    float res;
    std::memcpy(&res, &temp, sizeof(float));
    return res;
}

double ReadDouble(std::istream& stream) {
    auto temp = Read8Bytes(stream);
    double res;
    std::memcpy(&res, &temp, sizeof(double));
    return res;
}

std::string ReadString(std::istream& stream) {
    auto length = Read4Bytes(stream);
    char buffer[length];
    if (!stream.read(buffer, length)) {
        throw std::runtime_error("Failed to read bytes from stream, tried to read string of length " + std::to_string(length));
    }
    return std::string(buffer, length);
}

std::vector<char> Serialize2Bytes(uint16_t value) {
    std::vector<char> buffer(2);
    buffer[0] = static_cast<char>(value & 0xFF);
    buffer[1] = static_cast<char>((value >> 8) & 0xFF);
    return buffer;
}

std::vector<char> Serialize4Bytes(uint32_t value) {
    std::vector<char> buffer(4);
    buffer[0] = static_cast<char>(value & 0xFF);
    buffer[1] = static_cast<char>((value >> 8) & 0xFF);
    buffer[2] = static_cast<char>((value >> 16) & 0xFF);
    buffer[3] = static_cast<char>((value >> 24) & 0xFF);
    return buffer;
}

std::vector<char> Serialize8Bytes(uint64_t value) {
    std::vector<char> buffer(8);
    buffer[0] = static_cast<char>(value & 0xFF);
    buffer[1] = static_cast<char>((value >> 8) & 0xFF);
    buffer[2] = static_cast<char>((value >> 16) & 0xFF);
    buffer[3] = static_cast<char>((value >> 24) & 0xFF);
    buffer[4] = static_cast<char>((value >> 32) & 0xFF);
    buffer[5] = static_cast<char>((value >> 40) & 0xFF);
    buffer[6] = static_cast<char>((value >> 48) & 0xFF);
    buffer[7] = static_cast<char>((value >> 56) & 0xFF);
    return buffer;
}

std::vector<char> SerializeFloat(float value) {
    std::uint32_t temp;
    std::memcpy(&temp, &value, sizeof(float));
    return Serialize4Bytes(temp);
}

std::vector<char> SerializeDouble(double value) {
    std::uint64_t temp;
    std::memcpy(&temp, &value, sizeof(double));
    return Serialize8Bytes(temp);
}

std::vector<char> SerializeString(const std::string& value) {
    std::vector<char> result;
    auto serialized_length = Serialize4Bytes(value.size());
    result.insert(result.end(), std::make_move_iterator(serialized_length.begin()), std::make_move_iterator(serialized_length.end()));
    result.insert(result.end(), value.begin(), value.end());
    return result;
}

} // namespace lib::chunk_impl
