#include "json.h"

#include <fstream>
#include <iterator>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <lib/chunk_impl/io.h>

namespace lib::chunk_impl {

namespace {

    std::shared_ptr<document::Value> ParseRapidJsonValue(rapidjson::Value&& value, const TreeNodePtr& root) {
        if (value.IsNull()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Null>());
        }
        if (value.IsBool()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Boolean>(value.GetBool()));
        }
        if (value.IsUint()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt32>(value.GetUint()));
        }
        if (value.IsUint64()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::UInt64>(value.GetUint64()));
        }
        if (value.IsInt()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Int32>(value.GetInt()));
        }
        if (value.IsInt64()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Int64>(value.GetInt64()));
        }
        if (value.IsFloat()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Float32>(value.GetFloat()));
        }
        if (value.IsDouble()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Float64>(value.GetDouble()));
        }
        if (value.IsString()) {
            return std::static_pointer_cast<document::Value>(std::make_shared<document::String>(value.GetString()));
        }
        if (value.IsObject()) {
            document::ValueMap doc_map;
            for (auto&& m : value.GetObject()) {
                auto key = m.name.GetString();
                if (root->IsLeaf()) {
                    doc_map[key] = ParseRapidJsonValue(std::move(m.value), root);
                    continue;
                }

                const auto it = root->children.find(key);
                if (it == root->children.end()) {
                    continue;
                }

                doc_map[key] = ParseRapidJsonValue(std::move(m.value), it->second);
            }
            return std::static_pointer_cast<document::Value>(std::make_shared<document::Document>(std::move(doc_map)));
        }
        if (value.IsArray()) {
            auto arr = value.GetArray();
            document::ValueList list;
            list.reserve(arr.Size());
            for (auto&& v : arr) {
                list.push_back(ParseRapidJsonValue(std::move(v), root));
            }
            return std::static_pointer_cast<document::Value>(std::make_shared<document::List>(std::move(list)));
        }

        throw std::runtime_error("Type is not supported");
    }

    std::shared_ptr<document::Document> ReadJsonLine(std::string&& line, const TreeNodePtr& tree) {
        rapidjson::Document doc;

        if (doc.Parse(line.c_str()).HasParseError()) {
            throw std::runtime_error("Failed to parse JSON");
        }

        if (!doc.IsObject()) {
            throw std::runtime_error("JSON is not an object");
        }

        return std::static_pointer_cast<document::Document>(ParseRapidJsonValue(doc.GetObject(), tree));
    }

} // namespace

std::vector<std::shared_ptr<document::Document>> JsonChunk::Read(const TreeNodePtr& tree) const {
    auto stream = GetInputStream(path);

    std::vector<std::shared_ptr<document::Document>> res;
    while (!stream->Eof()) {
        std::string line = stream->ReadLine();
        if (line.empty()) {
            break;
        }

        res.emplace_back(ReadJsonLine(std::move(line), tree));
    }

    return res;
}

namespace {

    rapidjson::Value ValueToRapidJsonValue(const std::shared_ptr<document::Value>& value, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator) {
        rapidjson::Value rj_value;
        switch (value->GetTypeId()) {
            case document::TypeId::kNull:
                rj_value.SetNull();
                return rj_value;
            case document::TypeId::kBoolean:
                rj_value.SetBool(std::static_pointer_cast<document::Boolean>(value)->value);
                return rj_value;
            case document::TypeId::kInt32:
                rj_value.SetInt(std::static_pointer_cast<document::Int32>(value)->value);
                return rj_value;
            case document::TypeId::kUint32:
                rj_value.SetUint(std::static_pointer_cast<document::UInt32>(value)->value);
                return rj_value;
            case document::TypeId::kInt64:
                rj_value.SetInt64(std::static_pointer_cast<document::Int64>(value)->value);
                return rj_value;
            case document::TypeId::kUint64:
                rj_value.SetUint64(std::static_pointer_cast<document::UInt64>(value)->value);
                return rj_value;
            case document::TypeId::kFloat32:
                rj_value.SetFloat(std::static_pointer_cast<document::Float32>(value)->value);
                return rj_value;
            case document::TypeId::kFloat64:
                rj_value.SetDouble(std::static_pointer_cast<document::Float64>(value)->value);
                return rj_value;
            case document::TypeId::kString:
                rj_value.SetString(std::static_pointer_cast<document::String>(value)->value.c_str(), allocator);
                return rj_value;
            case document::TypeId::kDocument: {
                rj_value.SetObject();
                const auto& derived = std::static_pointer_cast<document::Document>(value);
                for (const auto& [k, v] : derived->value) {
                    rapidjson::Value obj_key(k.c_str(), allocator);
                    rapidjson::Value obj_value = ValueToRapidJsonValue(v, allocator);
                    rj_value.AddMember(std::move(obj_key), std::move(obj_value), allocator);
                }
                return rj_value;
            }
            case document::TypeId::kList: {
                rj_value.SetArray();
                const auto& derived = std::static_pointer_cast<document::List>(value);
                for (const auto& v : derived->value) {
                    rapidjson::Value obj_value = ValueToRapidJsonValue(v, allocator);
                    rj_value.PushBack(std::move(obj_value), allocator);
                }
                return rj_value;
            }
        }
    }

    std::string DocumentToJson(const std::shared_ptr<document::Document>& document) {
        rapidjson::Document rj_doc;
        rj_doc.SetObject();

        auto rj_value = ValueToRapidJsonValue(std::static_pointer_cast<document::Value>(document), rj_doc.GetAllocator());
        rj_doc.Swap(rj_value);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        rj_doc.Accept(writer);

        return std::string(buffer.GetString(), buffer.GetSize());
    }
} // namespace

void JsonChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {
    auto stream = GetOutputStream(path);

    for (const auto& doc : documents) {
        auto json = DocumentToJson(doc);
        stream->Write(json.c_str(), json.size());
        stream->Write("\n", 1);
    }
}

} // namespace lib::chunk_impl
