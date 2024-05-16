#include "bson.h"

#include <fstream>

#include <lib/chunk_impl/common.h>
#include <lib/chunk_impl/get_stream.h>

namespace lib::chunk_impl {

namespace {

    void SkipValue(IStream& stream) {
        auto cch = ReadControlChar(stream);

        switch (*cch) {
            case ControlChar::kNullFlag:
                return;
            case ControlChar::kBooleanFlag: {
                stream.Seekg(1);
                return;
            }
            case ControlChar::kInt32Flag:
            case ControlChar::kUint32Flag:
            case ControlChar::kFloat32Flag:
                stream.Seekg(4);
                return;
            case ControlChar::kInt64Flag:
            case ControlChar::kUint64Flag:
            case ControlChar::kFloat64Flag:
                stream.Seekg(8);
                return;
            case ControlChar::kStringFlag:
            case ControlChar::kDocumentFlag:
            case ControlChar::kListFlag: {
                auto length = Read4Bytes(stream);
                stream.Seekg(length);
                return;
            }
            default:
                throw std::runtime_error("Unexpected control character");
        }
    }

    std::optional<std::shared_ptr<document::Value>> ReadValue(IStream& stream, const TreeNodePtr& root) {
        const auto cch = ReadControlChar(stream);
        if (!cch.has_value()) {
            return std::nullopt;
        }

        if (IsPrimitiveControlChar(*cch)) {
            return ReadPrimitiveValue(*cch, stream);
        }

        switch (*cch) {
            case ControlChar::kDocumentFlag: {
                auto length = Read4Bytes(stream);
                auto end = stream.Tellg();
                end += length;

                document::ValueMap doc_map;
                while (stream.Tellg() < end) {
                    auto key_length = Read4Bytes(stream);
                    char buffer[key_length];
                    stream.Read(buffer, key_length);
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
            case ControlChar::kListFlag: {
                auto length = Read4Bytes(stream);
                auto end = stream.Tellg();
                end += length;

                document::ValueList list;
                while (stream.Tellg() < end) {
                    auto maybe_v = ReadValue(stream, root);
                    if (!maybe_v.has_value()) {
                        throw std::runtime_error("Unexpected end of file");
                    }
                    list.push_back(maybe_v.value());
                }
                return std::static_pointer_cast<document::Value>(std::make_shared<document::List>(std::move(list)));
            }
            default:
                throw std::logic_error("Unreachable code");
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

    std::vector<char> SerializeValue(const std::shared_ptr<document::Value>& value) {
        if (value->IsOfPrimitiveType()) {
            return SerializePrimitiveValue(value);
        }

        std::vector<char> result;

        switch (value->GetTypeId()) {
            case document::TypeId::kDocument: {
                result.emplace_back(static_cast<char>(ControlChar::kDocumentFlag));
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
                result.emplace_back(static_cast<char>(ControlChar::kListFlag));
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
            default:
                throw std::logic_error("Unreachable code");
        }
    }

} // namespace

void BsonChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {
    auto stream = GetOutputStream(path);

    for (const auto& document : documents) {
        auto serialized = SerializeValue(std::static_pointer_cast<document::Value>(document));
        stream->write(&serialized[0], serialized.size());
    }

    stream->flush();
}

} // namespace lib::chunk_impl
