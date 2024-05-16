#pragma once

#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace lib::chunk_impl::dremel {

enum class FieldLabel {
    // Required, // all keys are optional in json
    Optional,
    Repeated,
};

enum class FieldType {
    Primitive,
    Object,
};

using FieldHashType = std::size_t;
using RepetitionLevel = std::uint32_t;
using DefinitionLevel = std::uint16_t;

class FieldDescriptor {
protected:
    std::shared_ptr<FieldDescriptor> parent_;
    std::vector<std::shared_ptr<FieldDescriptor>> children_;

    std::string field_name_;
    FieldLabel field_label_;
    FieldType field_type_;
    RepetitionLevel max_repetition_level_;
    DefinitionLevel definition_level_;

    FieldHashType field_hash_;

public:
    FieldDescriptor() = delete;
    FieldDescriptor(const FieldDescriptor&) = delete;
    FieldDescriptor(FieldDescriptor&&) = delete;
    FieldDescriptor& operator=(const FieldDescriptor&) = delete;
    FieldDescriptor& operator=(FieldDescriptor&&) = delete;

    FieldDescriptor(
        const std::shared_ptr<FieldDescriptor>& parent,
        const std::string& field_name,
        FieldLabel field_label,
        FieldType field_type,
        RepetitionLevel max_repetition_level,
        DefinitionLevel definition_level);

    std::shared_ptr<FieldDescriptor> GetParent() const;
    const std::vector<std::shared_ptr<FieldDescriptor>>& GetChildren() const;
    std::string GetFieldName() const;
    FieldLabel GetFieldLabel() const;
    FieldType GetFieldType() const;
    RepetitionLevel GetMaxRepetitionLevel() const;
    DefinitionLevel GetDefinitionLevel() const;
    FieldHashType GetFieldHash() const;

    void AddChild(const std::shared_ptr<FieldDescriptor>& child);

    bool IsLeaf() const;
    bool IsRoot() const;
    bool HasAnyChild() const;

    std::string ConstructPath() const;
    std::string ToString() const;
    std::string Dump(size_t ident_cnt = 0) const;

    bool operator==(const FieldDescriptor& other) const;

    static std::shared_ptr<FieldDescriptor> LowestCommonAncestor(const std::shared_ptr<FieldDescriptor>& first, const std::shared_ptr<FieldDescriptor>& second);
    static std::vector<std::shared_ptr<FieldDescriptor>> GetPath(const std::shared_ptr<FieldDescriptor>& from, const std::shared_ptr<FieldDescriptor>& to);
    static std::vector<std::shared_ptr<FieldDescriptor>> GetPathToRoot(const std::shared_ptr<FieldDescriptor>& from);
};

using FieldDescriptorPtr = std::shared_ptr<FieldDescriptor>;

std::vector<FieldDescriptorPtr> LeafNodes(const FieldDescriptorPtr& root);

struct FieldHasher {
    std::size_t operator()(const FieldDescriptor& k) const;
    std::size_t operator()(const FieldDescriptorPtr& k) const;

    template <class T>
    std::size_t operator()(const std::shared_ptr<T>& k) const {
        static_assert(std::is_base_of<FieldDescriptor, T>::value, "Template parameter T must inherit from class FieldDescriptor");
        return operator()(std::static_pointer_cast<FieldDescriptor>(k));
    }
};

} // namespace lib::chunk_impl::dremel
