#include "field_descriptor.h"

#include <unordered_set>
#include <sstream>

namespace lib::chunk_impl::dremel {

FieldDescriptor::FieldDescriptor(
    const std::shared_ptr<FieldDescriptor>& parent,
    const std::string& field_name,
    FieldLabel field_label,
    FieldType field_type,
    RepetitionLevel max_repetition_level,
    DefinitionLevel definition_level)
    : parent_(parent)
    , field_name_(field_name)
    , field_label_(field_label)
    , field_type_(field_type)
    , max_repetition_level_(max_repetition_level)
    , definition_level_(definition_level) {
    field_hash_ = std::hash<std::string>()(ConstructPath());
}

void FieldDescriptor::AddChild(const std::shared_ptr<FieldDescriptor>& child) {
    children_.push_back(child);
}

std::string FieldDescriptor::GetFieldName() const {
    return field_name_;
}
FieldLabel FieldDescriptor::GetFieldLabel() const {
    return field_label_;
}
FieldType FieldDescriptor::GetFieldType() const {
    return field_type_;
}
RepetitionLevel FieldDescriptor::GetMaxRepetitionLevel() const {
    return max_repetition_level_;
}
DefinitionLevel FieldDescriptor::GetDefinitionLevel() const {
    return definition_level_;
}
FieldHashType FieldDescriptor::GetFieldHash() const {
    return field_hash_;
}
FieldDescriptorPtr FieldDescriptor::GetParent() const {
    return parent_;
}
const std::vector<FieldDescriptorPtr>& FieldDescriptor::GetChildren() const {
    return children_;
}

bool FieldDescriptor::IsLeaf() const {
    return field_type_ == FieldType::Primitive;
}
bool FieldDescriptor::IsRoot() const {
    return parent_ == nullptr;
}

std::string FieldDescriptor::ConstructPath() const {
    if (IsRoot()) {
        return "";
    }
    return parent_->ConstructPath() + "." + field_name_;
}
std::string FieldDescriptor::ToString() const {
    std::ostringstream oss;
    oss << "<Writer [field_type=" << (int)field_type_ << ", field_label=" << (int)field_label_ << "]: " << ConstructPath()
        << " leaf:" << IsLeaf()
        << " MaxR=" << max_repetition_level_ << ", D=" << definition_level_ << ">";
    return oss.str();
}
std::string FieldDescriptor::Dump(size_t ident_cnt) const {
    std::ostringstream oss;
    oss << std::string(ident_cnt * 2, ' ') << ToString() << '\n';
    for (const auto& child : children_) {
        oss << child->Dump(ident_cnt + 1);
    }
    return oss.str();
}

FieldDescriptorPtr FieldDescriptor::LowestCommonAncestor(const FieldDescriptorPtr& first, const FieldDescriptorPtr& second) {
    auto a = GetPathToRoot(first);
    auto b = GetPathToRoot(second);
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());

    if (a[0]->GetFieldHash() != b[0]->GetFieldHash()) {
        throw std::logic_error("Nodes from different graph");
    }

    auto common = a[0];
    for (auto i = 1; i < std::min(a.size(), b.size()); ++i) {
        if (a[i]->GetFieldHash() != b[i]->GetFieldHash()) {
            break;
        }
        common = a[i];
    }
    return common;
}

std::vector<std::shared_ptr<FieldDescriptor>> FieldDescriptor::GetPathToRoot(const std::shared_ptr<FieldDescriptor>& from) {
    std::vector<std::shared_ptr<FieldDescriptor>> res;
    auto current = from;
    while (current != nullptr) {
        res.push_back(current);
        current = current->GetParent();
    }
    return res;
}

std::vector<std::shared_ptr<FieldDescriptor>> FieldDescriptor::GetPath(const std::shared_ptr<FieldDescriptor>& from, const std::shared_ptr<FieldDescriptor>& to) {
    std::vector<std::shared_ptr<FieldDescriptor>> res;
    auto current = from;
    while (current != nullptr && (to == nullptr || current != to)) {
        res.push_back(current);
        current = current->GetParent();
    }

    if (to != nullptr && current == nullptr) {
        throw std::logic_error("No path to target");
    }

    return res;
}

bool FieldDescriptor::operator==(const FieldDescriptor& other) const {
    return GetFieldHash() == other.GetFieldHash();
}

std::vector<FieldDescriptorPtr> LeafNodes(const FieldDescriptorPtr& root) {
    if (root->IsLeaf()) {
        return {root};
    }
    std::vector<FieldDescriptorPtr> res;
    for (const auto& child : root->GetChildren()) {
        for (auto&& child_leaf : LeafNodes(child)) {
            res.emplace_back(std::move(child_leaf));
        }
    }
    return res;
}

std::size_t FieldHasher::operator()(const FieldDescriptor& k) const {
    return k.GetFieldHash();
}
std::size_t FieldHasher::operator()(const std::shared_ptr<FieldDescriptor>& k) const {
    return k->GetFieldHash();
}

} // namespace lib::chunk_impl::dremel
