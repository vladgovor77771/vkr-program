#include "columnar.h"

#include <filesystem>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <lib/chunk_impl/dremel/assembly.h>
#include <lib/chunk_impl/dremel/field_reader.h>
#include <lib/chunk_impl/dremel/field_writer.h>
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

    void RecurseCreateWritersTree(const rapidjson::Value& schema, const std::shared_ptr<dremel::FieldWriter>& root_writer) {
        for (auto& kv : schema.GetObject()) {
            auto max_repetition_level = root_writer->GetMaxRepetitionLevel();
            auto definition_level = root_writer->GetDefinitionLevel() + 1;
            const auto field_name = std::string(kv.name.GetString());

            auto value = &kv.value;
            dremel::FieldLabel field_label = dremel::FieldLabel::Optional;

            if (value->IsArray()) {
                ++max_repetition_level;
                field_label = dremel::FieldLabel::Repeated;
                value = &kv.value.GetArray()[0];
            }

            if (value->IsObject()) {
                auto child = std::make_shared<dremel::FieldWriter>(root_writer->GetChunkPath(), root_writer, field_name, field_label, dremel::FieldType::Object, max_repetition_level, definition_level);
                root_writer->AddChild(child);
                RecurseCreateWritersTree(value->GetObject(), child);
            } else {
                auto child = std::make_shared<dremel::FieldWriter>(root_writer->GetChunkPath(), root_writer, field_name, field_label, dremel::FieldType::Primitive, max_repetition_level, definition_level);
                root_writer->AddChild(child);
            }
        }
    }

    void RecurseCreateReadersTree(const rapidjson::Value& schema, const std::shared_ptr<dremel::FieldReader>& root_reader, const TreeNodePtr& tree) {
        for (auto& kv : schema.GetObject()) {
            auto max_repetition_level = root_reader->GetMaxRepetitionLevel();
            auto definition_level = root_reader->GetDefinitionLevel() + 1;
            const auto field_name = std::string(kv.name.GetString());
            if (!tree->IsLeaf() && tree->children.find(field_name) == tree->children.end()) {
                continue;
            }

            auto value = &kv.value;
            dremel::FieldLabel field_label = dremel::FieldLabel::Optional;

            if (value->IsArray()) {
                ++max_repetition_level;
                field_label = dremel::FieldLabel::Repeated;
                value = &kv.value.GetArray()[0];
            }

            if (value->IsObject()) {
                auto child = std::make_shared<dremel::FieldReader>(root_reader->GetChunkPath(), root_reader, field_name, field_label, dremel::FieldType::Object, max_repetition_level, definition_level);
                root_reader->AddChild(child);
                RecurseCreateReadersTree(value->GetObject(), child, tree->IsLeaf() ? tree : tree->children[field_name]);
            } else {
                auto child = std::make_shared<dremel::FieldReader>(root_reader->GetChunkPath(), root_reader, field_name, field_label, dremel::FieldType::Primitive, max_repetition_level, definition_level);
                root_reader->AddChild(child);
            }
        }
    }
} // namespace

rapidjson::Document ColumnarChunk::ReadSchema() const {
    if (schema_path.empty()) {
        throw std::runtime_error("Need to pass schema path for this operation");
    }

    auto istream = lib::chunk_impl::GetInputStream(schema_path);
    std::string schema_json_str = istream->ReadLine();

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

std::vector<std::shared_ptr<document::Document>> ColumnarChunk::Read(const TreeNodePtr& tree) const {
    const auto schema = ReadSchema();

    const auto path_ptr = std::make_shared<std::string>(path);
    auto root_field_reader = std::make_shared<dremel::FieldReader>(path_ptr, nullptr, "__root__", dremel::FieldLabel::Optional, dremel::FieldType::Object, 0, 0);
    RecurseCreateReadersTree(schema, root_field_reader, tree);
    if (!root_field_reader->HasAnyChild()) {
        return {};
    }

    dremel::RecordReader reader(root_field_reader);
    std::vector<std::shared_ptr<document::Document>> res;
    while (true) {
        auto doc = reader.NextRecord();
        if (doc == nullptr) {
            break;
        }
        res.emplace_back(std::move(doc));
    }
    return res;
}

void ColumnarChunk::Write(const std::vector<std::shared_ptr<document::Document>>& documents) const {
    const auto schema = ReadSchema();

    CreateDirectoryIfNeeded(path);
    const auto path_ptr = std::make_shared<std::string>(path);
    auto root_field_writer = std::make_shared<dremel::FieldWriter>(path_ptr, nullptr, "__root__", dremel::FieldLabel::Optional, dremel::FieldType::Object, 0, 0);
    RecurseCreateWritersTree(schema, root_field_writer);
    if (!root_field_writer->HasAnyChild()) {
        return;
    }

    // std::cerr << root_field_writer->Dump() << '\n';
    for (const auto& doc : documents) {
        root_field_writer->Write(doc);
    }

    root_field_writer->FlushAll();
}

} // namespace lib::chunk_impl
