#pragma once

#include <lib/chunk_impl/dremel/field_descriptor.h>
#include <lib/chunk_impl/io.h>
#include <lib/document/document.h>

namespace lib::chunk_impl::dremel {

struct Row {
    const RepetitionLevel repetition_level;
    const DefinitionLevel definition_level;
    const std::shared_ptr<document::Value> value;
};

class FieldReader: public FieldDescriptor {
private:
    std::size_t field_index_;
    std::shared_ptr<std::string> chunk_path_;
    std::shared_ptr<IStream> GetOrCreateStream();

public:
    FieldReader() = delete;
    FieldReader(const FieldReader&) = delete;
    FieldReader(FieldReader&&) = delete;
    FieldReader& operator=(const FieldReader&) = delete;
    FieldReader& operator=(FieldReader&&) = delete;

    std::shared_ptr<IStream> stream;

    FieldReader(
        const std::shared_ptr<std::string>& chunk_path,
        const std::shared_ptr<FieldReader>& parent,
        const std::string& field_name,
        FieldLabel field_label,
        FieldType field_type,
        RepetitionLevel max_repetition_level,
        DefinitionLevel definition_level);

    bool IsDone();
    RepetitionLevel NextRepetitionLevel();
    std::shared_ptr<std::string> GetChunkPath() const;
    std::size_t GetFieldIndex() const;
    void SetFieldIndex(std::size_t index);

    Row ReadRow();
};

using FieldReaderPtr = std::shared_ptr<FieldReader>;

} // namespace lib::chunk_impl::dremel
