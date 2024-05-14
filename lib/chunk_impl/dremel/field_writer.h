#pragma once

#include <lib/chunk_impl/dremel/field_descriptor.h>
#include <lib/document/document.h>

namespace lib::chunk_impl::dremel {

class FieldWriter: public FieldDescriptor {
private:
    std::shared_ptr<std::string> chunk_path_;
    std::shared_ptr<std::ostream> stream;

    std::shared_ptr<std::ostream> GetOrCreateStream();

    void WriteNull(RepetitionLevel r, DefinitionLevel d);
    void WritePrimitiveImpl(RepetitionLevel r, DefinitionLevel d, const std::shared_ptr<document::Value>& value);
    void WriteImpl(RepetitionLevel r, DefinitionLevel d, const std::shared_ptr<document::Value>& value);

public:
    FieldWriter() = delete;
    FieldWriter(const FieldWriter&) = delete;
    FieldWriter(FieldWriter&&) = delete;
    FieldWriter& operator=(const FieldWriter&) = delete;
    FieldWriter& operator=(FieldWriter&&) = delete;

    FieldWriter(
        const std::shared_ptr<std::string>& chunk_path,
        const std::shared_ptr<FieldWriter>& parent,
        const std::string& field_name,
        FieldLabel field_label,
        FieldType field_type,
        RepetitionLevel max_repetition_level,
        DefinitionLevel definition_level);

    std::shared_ptr<std::string> GetChunkPath() const;
    void Write(const std::shared_ptr<document::Document>& value);
    void FlushAll();
};

using FieldWriterPtr = std::shared_ptr<FieldWriter>;

} // namespace lib::chunk_impl::dremel
