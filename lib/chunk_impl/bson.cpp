#include "bson.h"

#include <fstream>

#include <lib/chunk_impl/escape.h>

namespace lib::chunk_impl {

namespace {
    uint32_t Read8Bytes(std::istream& stream) {
        uint64_t result = 0;
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

    uint32_t Read4Bytes(std::istream& stream) {
        uint32_t result = 0;
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

    void SkipValue(std::istream& stream) {
        char ch;
        stream.get(ch);

        switch (ch) {
            case static_cast<char>(BsonControlChars::kNullFlag):
                return;
            case static_cast<char>(BsonControlChars::kBooleanFlag): {
                stream.ignore(1);
                return;
            }
            case static_cast<char>(BsonControlChars::kInt32Flag):
            case static_cast<char>(BsonControlChars::kUint32Flag):
            case static_cast<char>(BsonControlChars::kFloat32Flag):
                stream.ignore(4);
                return;
            case static_cast<char>(BsonControlChars::kInt64Flag):
            case static_cast<char>(BsonControlChars::kUint64Flag):
            case static_cast<char>(BsonControlChars::kFloat64Flag):
                stream.ignore(8);
                return;
            case static_cast<char>(BsonControlChars::kStringFlag):
            case static_cast<char>(BsonControlChars::kDocumentFlag):
            case static_cast<char>(BsonControlChars::kListFlag): {
                auto length = Read4Bytes(stream);
                stream.ignore(length);
                return;
            }
            default:
                throw std::runtime_error("Unexpected control character");
        }
    }

    std::shared_ptr<document::Value> ReadValue(std::istream& stream, const std::optional<std::unordered_set<std::string>>& columns, const std::string& root) {
        char ch;
        stream.get(ch);

        switch (ch) {
            case static_cast<char>(BsonControlChars::kNullFlag):
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Null>());
            case static_cast<char>(BsonControlChars::kBooleanFlag): {
                stream.get(ch);
                auto val = static_cast<uint8_t>(ch) == 1;
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Boolean>(val));
            }
            case static_cast<char>(BsonControlChars::kInt32Flag): {
                auto val = static_cast<int32_t>(Read4Bytes(stream));
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Int32>(val));
            }
            case static_cast<char>(BsonControlChars::kUint32Flag): {
                auto val = Read4Bytes(stream);
                return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt32>(val));
            }
            case static_cast<char>(BsonControlChars::kInt64Flag): {
                auto val = static_cast<int64_t>(Read8Bytes(stream));
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Int64>(val));
            }
            case static_cast<char>(BsonControlChars::kUint64Flag): {
                auto val = Read8Bytes(stream);
                return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt64>(val));
            }
            case static_cast<char>(BsonControlChars::kFloat32Flag): {
                auto val = static_cast<float>(Read4Bytes(stream));
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Float32>(val));
            }
            case static_cast<char>(BsonControlChars::kFloat64Flag): {
                auto val = static_cast<double>(Read8Bytes(stream));
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Float64>(val));
            }
            case static_cast<char>(BsonControlChars::kStringFlag): {
                auto length = Read4Bytes(stream);
                char buffer[length];
                if (!stream.read(buffer, length)) {
                    throw std::runtime_error("Failed to read 4 bytes from stream.");
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::String>(std::string(buffer, length)));
            }
            case static_cast<char>(BsonControlChars::kDocumentFlag): {
                auto length = Read4Bytes(stream);
                auto end = stream.tellg();
                end += length;

                document::ValueMap doc_map;
                while (stream.tellg() < end) {
                    auto key_length = Read4Bytes(stream);
                    char buffer[key_length];
                    if (!stream.read(buffer, key_length)) {
                        throw std::runtime_error("Failed to read 4 bytes from stream.");
                    }
                    auto key = std::string(buffer, length);
                    auto column_name = root + '.' + EscapeDot(key);
                    if (!columns.has_value() || columns.value().find(column_name) != columns.value().end()) {
                        doc_map[key] = ReadValue(stream, columns, column_name);
                    } else {
                        SkipValue(stream);
                    }
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Document>(std::move(doc_map)));
            }
            case static_cast<char>(BsonControlChars::kListFlag): {
                auto length = Read4Bytes(stream);
                auto end = stream.tellg();
                end += length;

                document::ValueList list;
                while (stream.tellg() < end) {
                    list.push_back(ReadValue(stream, columns, root));
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::List>(std::move(list)));
            }
            default:
                throw std::runtime_error("Unexpected control character");
        }
    }
} // namespace

std::vector<std::shared_ptr<document::Document>> BsonChunk::Read(const std::optional<std::unordered_set<std::string>>& columns) const {
    auto file = std::ifstream(path);
    if (!file) {
        throw std::runtime_error("Couldn't open input stream");
    }

    char ch;
    return {};
}

void BsonChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {

}

} // namespace lib::chunk_impl
