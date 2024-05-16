#include "field_reader.h"

#include <filesystem>

#include <lib/chunk_impl/common.h>
#include <lib/chunk_impl/get_stream.h>

namespace lib::chunk_impl::dremel {

FieldReader::FieldReader(
    const std::shared_ptr<std::string>& chunk_path,
    const std::shared_ptr<FieldReader>& parent,
    const std::string& field_name,
    FieldLabel field_label,
    FieldType field_type,
    RepetitionLevel max_repetition_level,
    DefinitionLevel definition_level)
    : FieldDescriptor(std::static_pointer_cast<FieldDescriptor>(parent), field_name, field_label, field_type, max_repetition_level, definition_level)
    , chunk_path_(chunk_path) {
    field_index_ = SIZE_MAX;
}

std::shared_ptr<IStream> FieldReader::GetOrCreateStream() {
    if (stream != nullptr) {
        return stream;
    }

    auto path = std::filesystem::path(*chunk_path_).append(ConstructPath()).string();
    stream = GetInputStream(path);
    return stream;
}

std::shared_ptr<std::string> FieldReader::GetChunkPath() const {
    return chunk_path_;
}

Row FieldReader::ReadRow() {
    if (!IsLeaf()) {
        throw std::logic_error("Only leaf nodes are allowed to call ReadMeta()");
    }
    if (IsDone()) {
        throw std::logic_error("Called ReadRow when reader on EOF");
    }

    const auto r = Read4Bytes(*stream);
    const auto d = Read2Bytes(*stream);
    const auto cch = ReadControlChar(*stream);
    const auto value = ReadPrimitiveValue(*cch, *stream);

    return Row{
        .repetition_level = r,
        .definition_level = d,
        .value = value,
    };
}

RepetitionLevel FieldReader::NextRepetitionLevel() {
    if (IsDone()) {
        return 0;
    }
    const auto r = Read4Bytes(*stream);
    stream->Seekg(-4);
    return r;
}

std::size_t FieldReader::GetFieldIndex() const {
    return field_index_;
}

void FieldReader::SetFieldIndex(std::size_t index) {
    if (!IsLeaf()) {
        throw std::logic_error("Tried to set field index on non-leaf node");
    }
    if (field_index_ != SIZE_MAX) {
        throw std::logic_error("Tried to set field index on already set node");
    }
    field_index_ = index;
}

bool FieldReader::IsDone() {
    if (!IsLeaf()) {
        throw std::logic_error("Tried to check IsDone on non-leaf node");
    }
    stream = GetOrCreateStream();
    return stream->Peek() == EOF;
}

} // namespace lib::chunk_impl::dremel
