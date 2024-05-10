#pragma once

#include <stack>

#include <lib/chunk_impl/chunk.h>

namespace lib::chunk_impl {

class ColumnarChunk: public Chunk {
private:
    std::string schema_path;

public:
    ColumnarChunk(const std::string& chunk_path, const std::string& schema_path);

    std::vector<std::shared_ptr<document::Document>> Read(const TreeNodePtr& tree = TreeNode::Default()) const override;
    void Write(const std::vector<std::shared_ptr<document::Document>>& documents) const override;
    rapidjson::Document ReadSchema() const;
};

enum class ColumnarFieldLabel {
    Optional,
    Repeated,
};

enum class ColumnarFieldType {
    Primitive,
    Object,
};

class ColumnarFieldDescriptor {
public:
    std::string chunk_path;
    std::string field_name;
    ColumnarFieldLabel field_label;
    ColumnarFieldType field_type;
    uint32_t max_repetition_level;
    uint16_t definition_level;
    std::shared_ptr<ColumnarFieldDescriptor> parent;
    std::unordered_map<std::string, std::shared_ptr<ColumnarFieldDescriptor>> children;

    ColumnarFieldDescriptor(
        const std::string& chunk_path,
        const std::string& field_name,
        ColumnarFieldLabel field_label,
        ColumnarFieldType field_type,
        uint32_t max_repetition_level,
        uint16_t definition_level,
        std::shared_ptr<ColumnarFieldDescriptor> parent)
        : chunk_path(chunk_path)
        , field_label(field_label)
        , field_name(field_name)
        , field_type(field_type)
        , max_repetition_level(max_repetition_level)
        , definition_level(definition_level)
        , parent(parent) {
    }

    bool IsLeaf() const {
        return children.empty();
    }

    bool IsRoot() const {
        return parent == nullptr;
    }

    std::string GetPath() const;
    std::string ToString() const;
    std::string Dump(size_t ident_cnt = 0) const;
};

// class ColumnarAssemblyBuilder {
// private:
//     std::shared_ptr<ColumnarFieldReader> field_graph_root;
//     std::stack<std::pair<std::shared_ptr<document::Value>, std::shared_ptr<ColumnarFieldReader>>> stack;
//     std::vector<std::shared_ptr<document::Document>> msgs;
//     std::shared_ptr<ColumnarFieldReader> last_node;

// public:
//     std::vector<std::shared_ptr<document::Document>> GetMessages() const {
//         return msgs;
//     }

//     void Start() {
//         stack.push(field_graph_root);
//         last_node = nullptr;
//     }

//     void Rollback() {
//         stack = std::stack<std::shared_ptr<ColumnarFieldReader>>();
//     }

//     bool Done() {
//     }
// };

struct ColumnarMetaEntry {
    uint32_t repetition_level;
    uint16_t definition_level;
};

class ColumnarFieldReader: public ColumnarFieldDescriptor {
private:
    std::shared_ptr<std::istream> GetOrCreateStream();

public:
    std::shared_ptr<std::istream> stream;

    ColumnarFieldReader(
        const std::string& chunk_path,
        const std::string& field_name,
        ColumnarFieldLabel field_label,
        ColumnarFieldType field_type,
        uint32_t max_repetition_level,
        uint16_t definition_level,
        std::shared_ptr<ColumnarFieldReader> parent)
        : ColumnarFieldDescriptor(chunk_path, field_name, field_label, field_type, max_repetition_level, definition_level, std::static_pointer_cast<ColumnarFieldDescriptor>(parent)) {
    }

    // void Validate(const rapidjson::Document& schema) const;
    std::optional<ColumnarMetaEntry> ReadMeta();
    std::shared_ptr<document::Value> ReadValue();
    std::shared_ptr<document::Document> ReadRecord();
    void Rollback();
};

class ColumnarFieldWriter: public ColumnarFieldDescriptor {
private:
    std::shared_ptr<std::ostream> GetOrCreateStream();

    void WriteNull(uint32_t r, uint16_t d);
    void WritePrimitiveImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value);
    void WriteImpl(uint32_t r, uint16_t d, const std::shared_ptr<document::Value>& value);

public:
    std::shared_ptr<std::ostream> stream;

    ColumnarFieldWriter(
        const std::string& chunk_path,
        const std::string& field_name,
        ColumnarFieldLabel field_label,
        ColumnarFieldType field_type,
        uint32_t max_repetition_level,
        uint16_t definition_level,
        std::shared_ptr<ColumnarFieldWriter> parent)
        : ColumnarFieldDescriptor(chunk_path, field_name, field_label, field_type, max_repetition_level, definition_level, std::static_pointer_cast<ColumnarFieldDescriptor>(parent)) {
    }

    void Write(const std::shared_ptr<document::Document>& value);
    void FlushAll();
};

} // namespace lib::chunk_impl
