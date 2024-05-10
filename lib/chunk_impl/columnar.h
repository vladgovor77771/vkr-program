#pragma once

#include <lib/chunk_impl/chunk.h>

namespace lib::chunk_impl {

class ColumnarChunk: public Chunk {
private:
    std::string schema_path;

public:
    ColumnarChunk(const std::string& chunk_path, const std::string& schema_path);

    std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const override;
    void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const override;
};

enum class ColumnarFieldLabel {
    Optional,
    Repeated,
};

enum class ColumnarFieldType {
    Primitive,
    Object,
};

class ColumnarFieldWriter {
private:
    std::shared_ptr<std::ostream> GetOrCreateStream();

    void WriteNull(uint32_t r, uint16_t d);
    void WritePrimitiveImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value);
    void WriteImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value);

public:
    std::string chunk_path;
    std::string field_name;
    ColumnarFieldLabel field_label;
    ColumnarFieldType field_type;
    uint32_t max_repetition_level;
    uint16_t definition_level;
    std::shared_ptr<std::ostream> stream;
    std::shared_ptr<ColumnarFieldWriter> parent;
    std::unordered_map<std::string, std::shared_ptr<ColumnarFieldWriter>> children;

    ColumnarFieldWriter(
        const std::string& chunk_path,
        const std::string& field_name,
        ColumnarFieldLabel field_label,
        ColumnarFieldType field_type,
        uint32_t max_repetition_level,
        uint16_t definition_level,
        std::shared_ptr<ColumnarFieldWriter> parent)
        : chunk_path(chunk_path)
        , field_label(field_label)
        , field_name(field_name)
        , field_type(field_type)
        , max_repetition_level(max_repetition_level)
        , definition_level(definition_level)
        , parent(parent) {
    }

    void Write(const std::shared_ptr<document::Document>& value);

    bool IsLeaf() const {
        return children.empty();
    }

    bool IsRoot() const {
        return parent == nullptr;
    }

    std::string GetPath() const;
    std::string ToString() const;
    std::string Dump(size_t ident_cnt = 0) const;

    void FlushAll();
};

} // namespace lib::chunk_impl
