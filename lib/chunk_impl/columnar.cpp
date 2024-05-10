#include "columnar.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <lib/chunk_impl/common.h>
#include <lib/chunk_impl/get_stream.h>

namespace lib::chunk_impl {

namespace {
    void CreateDirectoryIfNeeded(const std::string& path) {
        std::filesystem::path chunk_path(path);

        std::error_code ec;
        const auto created = std::filesystem::create_directories(chunk_path, ec);
        if (!created && ec) {
            throw std::runtime_error(ec.message());
        }
    }

    void RecurseCreateWritersTree(const rapidjson::Value& schema, const std::shared_ptr<ColumnarFieldWriter>& root_writer) {
        for (auto& kv : schema.GetObject()) {
            auto max_repetition_level = root_writer->max_repetition_level;
            auto definition_level = root_writer->definition_level + 1;
            const auto field_name = std::string(kv.name.GetString());

            auto value = &kv.value;
            ColumnarFieldLabel field_label = ColumnarFieldLabel::Optional;

            if (value->IsArray()) {
                ++max_repetition_level;
                field_label = ColumnarFieldLabel::Repeated;
                value = &kv.value.GetArray()[0];
            }

            if (value->IsObject()) {
                auto child = std::make_shared<ColumnarFieldWriter>(root_writer->chunk_path, field_name, field_label, ColumnarFieldType::Object, max_repetition_level, definition_level, root_writer);
                root_writer->children[field_name] = child;
                RecurseCreateWritersTree(value->GetObject(), child);
            } else {
                auto child = std::make_shared<ColumnarFieldWriter>(root_writer->chunk_path, field_name, field_label, ColumnarFieldType::Primitive, max_repetition_level, definition_level, root_writer);
                root_writer->children[field_name] = child;
            }
        }
    }

    void RecurseCreateReadersTree(const rapidjson::Value& schema, const std::shared_ptr<ColumnarFieldReader>& root_reader, const TreeNodePtr& tree) {
        for (auto& kv : schema.GetObject()) {
            auto max_repetition_level = root_reader->max_repetition_level;
            auto definition_level = root_reader->definition_level + 1;
            const auto field_name = std::string(kv.name.GetString());
            if (!tree->IsLeaf() && tree->children.find(field_name) == tree->children.end()) {
                continue;
            }

            auto value = &kv.value;
            ColumnarFieldLabel field_label = ColumnarFieldLabel::Optional;

            if (value->IsArray()) {
                ++max_repetition_level;
                field_label = ColumnarFieldLabel::Repeated;
                value = &kv.value.GetArray()[0];
            }

            if (value->IsObject()) {
                auto child = std::make_shared<ColumnarFieldReader>(root_reader->chunk_path, field_name, field_label, ColumnarFieldType::Object, max_repetition_level, definition_level, root_reader);
                root_reader->children[field_name] = child;
                RecurseCreateReadersTree(value->GetObject(), child, tree->IsLeaf() ? tree : tree->children[field_name]);
            } else {
                auto child = std::make_shared<ColumnarFieldReader>(root_reader->chunk_path, field_name, field_label, ColumnarFieldType::Primitive, max_repetition_level, definition_level, root_reader);
                root_reader->children[field_name] = child;
            }
        }
    }
} // namespace

rapidjson::Document ColumnarChunk::ReadSchema() const {
    if (schema_path.empty()) {
        throw std::runtime_error("Need to pass schema path for this operation");
    }

    auto istream = lib::chunk_impl::GetInputStream(schema_path);
    std::string schema_json_str;
    *istream >> schema_json_str;

    rapidjson::Document schema;

    if (schema.Parse(schema_json_str.c_str()).HasParseError()) {
        throw std::runtime_error("Failed to parse schema JSON");
    }

    if (!schema.IsObject()) {
        throw std::runtime_error("Schema JSON is not an object");
    }

    return schema;
}

ColumnarChunk::ColumnarChunk(const std::string& chunk_path, const std::string& schema_path)
    : Chunk(chunk_path)
    , schema_path(schema_path) {
}

std::shared_ptr<std::istream> ColumnarFieldReader::GetOrCreateStream() {
    if (stream != nullptr) {
        return stream;
    }

    auto path = std::filesystem::path(chunk_path).append(GetPath()).string();
    stream = GetInputStream(path);
    return stream;
}

std::optional<ColumnarMetaEntry> ColumnarFieldReader::ReadMeta() {
    stream = GetOrCreateStream();
    if (stream->eof()) {
        return std::nullopt;
    }

    auto r = Read4Bytes(*stream);
    auto d = Read2Bytes(*stream);
    return ColumnarMetaEntry{
        .definition_level = d,
        .repetition_level = r};
}

void ColumnarFieldReader::Rollback() {
    auto stream = GetOrCreateStream();
    stream->seekg(-6, std::ios_base::cur);
}

std::shared_ptr<document::Value> ColumnarFieldReader::ReadValue() {
    auto stream = GetOrCreateStream();
    const auto cch = ReadControlChar(*stream);
    return ReadPrimitiveValue(*cch, *stream);
}

std::shared_ptr<document::Document> ColumnarFieldReader::ReadRecord() {
    document::ValueMap map;

    // for (auto& [key, child] : children) {

    // }
    return nullptr;
}

std::vector<std::shared_ptr<document::Document>> ColumnarChunk::Read(const TreeNodePtr& tree) const {
    const auto schema = ReadSchema();

    auto reader = std::make_shared<ColumnarFieldReader>(path, "__root__", ColumnarFieldLabel::Optional, ColumnarFieldType::Object, 0, 0, nullptr);
    RecurseCreateReadersTree(schema, reader, tree);

    std::vector<std::shared_ptr<document::Document>> res;
    while (true) {
        auto doc = reader->ReadRecord();
        if (doc == nullptr) {
            break;
        }
        res.emplace_back(std::move(doc));
    }
    return res;
}

void ColumnarFieldWriter::FlushAll() {
    if (stream != nullptr) {
        stream->flush();
    }
    for (auto& [_, child] : children) {
        std::static_pointer_cast<ColumnarFieldWriter>(child)->FlushAll();
    }
}

std::shared_ptr<std::ostream> ColumnarFieldWriter::GetOrCreateStream() {
    if (stream != nullptr) {
        return stream;
    }

    auto path = std::filesystem::path(chunk_path).append(GetPath()).string();
    stream = GetOutputStream(path);
    return stream;
}

std::string ColumnarFieldDescriptor::GetPath() const {
    if (IsRoot()) {
        return "";
    }
    return parent->GetPath() + "." + field_name;
}

std::string ColumnarFieldDescriptor::Dump(size_t ident_cnt) const {
    std::ostringstream oss;
    oss << std::string(ident_cnt * 2, ' ') << ToString() << '\n';
    for (const auto& [k, child] : children) {
        oss << child->Dump(ident_cnt + 1);
    }
    return oss.str();
}

std::string ColumnarFieldDescriptor::ToString() const {
    std::ostringstream oss;
    oss << "<Writer [field_type=" << (int)field_type << ", field_label=" << (int)field_label << "]: " << GetPath()
        << " leaf:" << IsLeaf()
        << " R=" << max_repetition_level << ", D=" << definition_level << ">";
    return oss.str();
}

void ColumnarFieldWriter::WriteNull(uint32_t r, uint16_t d) {
    if (field_type == ColumnarFieldType::Object) {
        for (auto [_, child] : children) {
            std::static_pointer_cast<ColumnarFieldWriter>(child)->WriteNull(r, d);
        }
    } else {
        WritePrimitiveImpl(r, d, std::make_shared<document::Null>());
        return;
    }
}

void ColumnarFieldWriter::WritePrimitiveImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value) {
    if (value == nullptr) {
        return WriteNull(r, d);
    }
    auto res = Serialize4Bytes(r);
    auto d_serialized = Serialize2Bytes(d);
    auto value_serialized = SerializePrimitiveValue(value);
    res.insert(res.end(), d_serialized.begin(), d_serialized.end());
    res.insert(res.end(), value_serialized.begin(), value_serialized.end());

    auto stream = GetOrCreateStream();
    stream->write(res.data(), res.size());
    // std::cerr << "Writing " << field_name << " " << r << " " << d << ' ' << "some_value" << '\n';
}

void ColumnarFieldWriter::WriteImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value) {
    if (value == nullptr) {
        WriteNull(r, d);
        return;
    }

    if (field_label == ColumnarFieldLabel::Optional) {
        const auto doc = std::static_pointer_cast<document::Document>(value);

        const auto val_it = doc->value.find(field_name);
        const auto local_d = val_it != doc->value.end() ? d + 1 : d;
        std::optional<std::shared_ptr<document::Value>> val;
        if (val_it != doc->value.end()) {
            val = val_it->second;
        }

        if (field_type == ColumnarFieldType::Object) {
            for (auto [_, child] : children) {
                std::static_pointer_cast<ColumnarFieldWriter>(child)->WriteImpl(r, local_d, val.value_or(nullptr));
            }
            return;
        } else {
            WritePrimitiveImpl(r, local_d, val.value_or(nullptr));
            return;
        }
    } else {
        const auto doc = std::static_pointer_cast<document::Document>(value);
        const auto list_it = doc->value.find(field_name);
        if (list_it == doc->value.end()) {
            WriteNull(r, d);
            return;
        }
        const auto list_val = std::static_pointer_cast<document::List>(list_it->second);
        if (list_val->value.empty()) {
            WriteNull(r, d);
            return;
        }
        auto local_r = r;
        for (const auto& list_elem : list_val->value) {
            if (field_type == ColumnarFieldType::Object) {
                for (auto& [_, child] : children) {
                    std::static_pointer_cast<ColumnarFieldWriter>(child)->WriteImpl(local_r, d + 1, list_elem);
                }
            } else {
                WritePrimitiveImpl(local_r, d + 1, list_elem);
            }
            local_r = max_repetition_level;
        }
    }
}

void ColumnarFieldWriter::Write(const std::shared_ptr<document::Document>& value) {
    for (auto& [_, child] : children) {
        std::static_pointer_cast<ColumnarFieldWriter>(child)->WriteImpl(0, 0, std::static_pointer_cast<document::Value>(value));
    }
}

void ColumnarChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {
    const auto schema = ReadSchema();

    CreateDirectoryIfNeeded(path);
    auto writer = std::make_shared<ColumnarFieldWriter>(path, "__root__", ColumnarFieldLabel::Optional, ColumnarFieldType::Object, 0, 0, nullptr);
    RecurseCreateWritersTree(schema, writer);

    std::cerr << writer->Dump() << '\n';
    for (const auto& doc : documents) {
        writer->Write(doc);
    }

    writer->FlushAll();
}

} // namespace lib::chunk_impl
