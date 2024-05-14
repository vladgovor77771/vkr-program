#include "field_writer.h"

#include <filesystem>

#include <lib/chunk_impl/common.h>
#include <lib/chunk_impl/get_stream.h>

namespace lib::chunk_impl::dremel {

FieldWriter::FieldWriter(
    const std::shared_ptr<std::string>& chunk_path,
    const std::shared_ptr<FieldWriter>& parent,
    const std::string& field_name,
    FieldLabel field_label,
    FieldType field_type,
    RepetitionLevel max_repetition_level,
    DefinitionLevel definition_level)
    : FieldDescriptor(std::static_pointer_cast<FieldDescriptor>(parent), field_name, field_label, field_type, max_repetition_level, definition_level)
    , chunk_path_(chunk_path) {
}

std::shared_ptr<std::ostream> FieldWriter::GetOrCreateStream() {
    if (stream != nullptr) {
        return stream;
    }

    auto path = std::filesystem::path(*chunk_path_).append(ConstructPath()).string();
    stream = GetOutputStream(path);
    return stream;
}

void FieldWriter::WriteNull(RepetitionLevel r, DefinitionLevel d) {
    // std::cerr << ConstructPath() << " in WriteNull "
    //           << " r=" << r << " d=" << d << '\n';
    if (field_type_ == FieldType::Object) {
        for (const auto& child : children_) {
            std::static_pointer_cast<FieldWriter>(child)->WriteNull(r, d);
        }
    } else {
        WritePrimitiveImpl(r, d, std::make_shared<document::Null>());
        return;
    }
}

void FieldWriter::WritePrimitiveImpl(RepetitionLevel r, DefinitionLevel d, const std::shared_ptr<document::Value>& value) {
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
}

void FieldWriter::WriteImpl(RepetitionLevel r, DefinitionLevel d, const std::shared_ptr<document::Value>& value) {
    if (value == nullptr || value->IsNull()) {
        WriteNull(r, d);
        return;
    }

    if (field_label_ == FieldLabel::Optional) {
        const auto doc = std::static_pointer_cast<document::Document>(value);

        const auto val_it = doc->value.find(field_name_);
        const auto local_d = (val_it != doc->value.end() && !val_it->second->IsNull()) ? d + 1 : d;
        std::optional<std::shared_ptr<document::Value>> val;
        if (val_it != doc->value.end() && !val_it->second->IsNull()) {
            val = val_it->second;
        }

        if (field_type_ == FieldType::Object) {
            for (const auto& child : children_) {
                std::static_pointer_cast<FieldWriter>(child)->WriteImpl(r, local_d, val.value_or(nullptr));
            }
            return;
        } else {
            WritePrimitiveImpl(r, local_d, val.value_or(nullptr));
            return;
        }
    } else {
        const auto doc = std::static_pointer_cast<document::Document>(value);
        const auto list_it = doc->value.find(field_name_);
        if (list_it == doc->value.end() || list_it->second->IsNull()) {
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
            if (field_type_ == FieldType::Object) {
                for (const auto& child : children_) {
                    if (list_elem->IsNull()) {
                        std::static_pointer_cast<FieldWriter>(child)->WriteNull(local_r, d + 1);
                        continue;
                    }
                    std::static_pointer_cast<FieldWriter>(child)->WriteImpl(local_r, d + 1, list_elem);
                }
            } else {
                WritePrimitiveImpl(local_r, d + 1, list_elem);
            }
            local_r = max_repetition_level_;
        }
    }
}

void FieldWriter::Write(const std::shared_ptr<document::Document>& value) {
    for (auto& child : children_) {
        std::static_pointer_cast<FieldWriter>(child)->WriteImpl(0, 0, std::static_pointer_cast<document::Value>(value));
    }
}

std::shared_ptr<std::string> FieldWriter::GetChunkPath() const {
    return chunk_path_;
}

void FieldWriter::FlushAll() {
    if (IsLeaf()) {
        if (stream == nullptr) {
            return;
        }
        stream->flush();
        std::static_pointer_cast<std::ofstream>(stream)->close();
        return;
    }
    for (const auto& child : children_) {
        std::static_pointer_cast<FieldWriter>(child)->FlushAll();
    }
}

} // namespace lib::chunk_impl::dremel
