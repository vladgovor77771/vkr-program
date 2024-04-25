#include "bson.h"

#include <fstream>

#include <lib/chunk_impl/get_stream.h>

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

    float ReadFloat(std::istream& stream) {
        auto temp = Read4Bytes(stream);
        float res;
        std::memcpy(&res, &temp, sizeof(float));
        return res;
    }

    float ReadDouble(std::istream& stream) {
        auto temp = Read8Bytes(stream);
        double res;
        std::memcpy(&res, &temp, sizeof(double));
        return res;
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

    std::optional<std::shared_ptr<document::Value>> ReadValue(std::istream& stream, const TreeNodePtr& root) {
        char ch;
        if (!stream.get(ch)) {
            return std::nullopt;
        }

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
                auto val = ReadFloat(stream);
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Float32>(val));
            }
            case static_cast<char>(BsonControlChars::kFloat64Flag): {
                auto val = Read8Bytes(stream);
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
                        throw std::runtime_error("Failed to read length bytes from stream.");
                    }
                    auto key = std::string(buffer, key_length);
                    if (root->IsLeaf()) {
                        auto maybe_v = ReadValue(stream, root);
                        if (!maybe_v.has_value()) {
                            throw std::runtime_error("Unexpected end of file");
                        }
                        doc_map[key] = maybe_v.value();
                        continue;
                    }

                    const auto it = root->children.find(key);
                    if (it == root->children.end()) {
                        SkipValue(stream);
                        continue;
                    }

                    doc_map[key] = ReadValue(stream, it->second).value();
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::Document>(std::move(doc_map)));
            }
            case static_cast<char>(BsonControlChars::kListFlag): {
                auto length = Read4Bytes(stream);
                auto end = stream.tellg();
                end += length;

                document::ValueList list;
                while (stream.tellg() < end) {
                    auto maybe_v = ReadValue(stream, root);
                    if (!maybe_v.has_value()) {
                        throw std::runtime_error("Unexpected end of file");
                    }
                    list.push_back(maybe_v.value());
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::List>(std::move(list)));
            }
            default:
                throw std::runtime_error("Unexpected control character");
        }
    }

} // namespace

std::vector<std::shared_ptr<document::Document>> BsonChunk::Read(const TreeNodePtr& tree) const {
    auto stream = GetInputStream(path);

    std::vector<std::shared_ptr<document::Document>> result;

    while (true) {
        auto doc = ReadValue(*stream, tree);
        if (!doc.has_value()) {
            break;
        }
        if (doc.value()->GetTypeId() != document::TypeId::kDocument) {
            throw std::runtime_error("Awaited document type, got " + document::TypeIdToString(doc.value()->GetTypeId()));
        }
        result.emplace_back(std::static_pointer_cast<document::Document>(doc.value()));
    }

    return result;
}

namespace {

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
        uint32_t temp;
        std::memcpy(&temp, &value, sizeof(float));
        return Serialize4Bytes(temp);
    }

    std::vector<char> SerializeDouble(double value) {
        uint64_t temp;
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

    std::vector<char> SerializeValue(const std::shared_ptr<document::Value>& value) {
        std::vector<char> result;

        switch (value->GetTypeId()) {
            case document::TypeId::kNull:
                result.emplace_back(static_cast<char>(BsonControlChars::kNullFlag));
                return result;
            case document::TypeId::kBoolean:
                result.emplace_back(static_cast<char>(BsonControlChars::kBooleanFlag));
                result.emplace_back(static_cast<char>(std::static_pointer_cast<document::Boolean>(value)->value));
                return result;
            case document::TypeId::kInt32: {
                result.emplace_back(static_cast<char>(BsonControlChars::kInt32Flag));
                auto serialized = Serialize4Bytes(static_cast<uint32_t>(std::static_pointer_cast<document::Int32>(value)->value));
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kUint32: {
                result.emplace_back(static_cast<char>(BsonControlChars::kUint32Flag));
                auto serialized = Serialize4Bytes(std::static_pointer_cast<document::UInt32>(value)->value);
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kInt64: {
                result.emplace_back(static_cast<char>(BsonControlChars::kInt64Flag));
                auto serialized = Serialize8Bytes(static_cast<uint64_t>(std::static_pointer_cast<document::Int64>(value)->value));
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kUint64: {
                result.emplace_back(static_cast<char>(BsonControlChars::kUint64Flag));
                auto serialized = Serialize8Bytes(std::static_pointer_cast<document::UInt64>(value)->value);
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kFloat32: {
                result.emplace_back(static_cast<char>(BsonControlChars::kFloat32Flag));
                auto serialized = SerializeFloat(std::static_pointer_cast<document::Float32>(value)->value);
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kFloat64: {
                result.emplace_back(static_cast<char>(BsonControlChars::kFloat64Flag));
                auto serialized = SerializeDouble(std::static_pointer_cast<document::Float64>(value)->value);
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kString: {
                result.emplace_back(static_cast<char>(BsonControlChars::kStringFlag));
                auto serialized = SerializeString(std::static_pointer_cast<document::String>(value)->value);
                result.insert(result.end(), std::make_move_iterator(serialized.begin()), std::make_move_iterator(serialized.end()));
                return result;
            }
            case document::TypeId::kDocument: {
                result.emplace_back(static_cast<char>(BsonControlChars::kDocumentFlag));
                const auto& map = std::static_pointer_cast<document::Document>(value)->value;

                std::vector<char> serialized_document;
                for (const auto& [k, v] : map) {
                    auto serialized_key = SerializeString(k);
                    serialized_document.insert(serialized_document.end(), std::make_move_iterator(serialized_key.begin()), std::make_move_iterator(serialized_key.end()));
                    auto serialized_value = SerializeValue(v);
                    serialized_document.insert(serialized_document.end(), std::make_move_iterator(serialized_value.begin()), std::make_move_iterator(serialized_value.end()));
                }

                auto serialized_document_size = Serialize4Bytes(serialized_document.size());
                result.insert(result.end(), std::make_move_iterator(serialized_document_size.begin()), std::make_move_iterator(serialized_document_size.end()));
                result.insert(result.end(), std::make_move_iterator(serialized_document.begin()), std::make_move_iterator(serialized_document.end()));
                return result;
            }
            case document::TypeId::kList: {
                result.emplace_back(static_cast<char>(BsonControlChars::kListFlag));
                const auto& list = std::static_pointer_cast<document::List>(value)->value;

                std::vector<char> serialized_list;
                for (const auto& v : list) {
                    auto serialized_value = SerializeValue(v);
                    serialized_list.insert(serialized_list.end(), std::make_move_iterator(serialized_value.begin()), std::make_move_iterator(serialized_value.end()));
                }

                auto serialized_list_size = Serialize4Bytes(serialized_list.size());
                result.insert(result.end(), std::make_move_iterator(serialized_list_size.begin()), std::make_move_iterator(serialized_list_size.end()));
                result.insert(result.end(), std::make_move_iterator(serialized_list.begin()), std::make_move_iterator(serialized_list.end()));
                return result;
            }
        }
    }

} // namespace

void BsonChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {
    auto stream = GetOutputStream(path);

    for (const auto& document : documents) {
        auto serialized = SerializeValue(std::static_pointer_cast<document::Value>(document));
        stream->write(&serialized[0], serialized.size());
    }
}

} // namespace lib::chunk_impl
