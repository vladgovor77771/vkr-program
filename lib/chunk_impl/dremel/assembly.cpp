#include "assembly.h"

#include <algorithm>
#include <iostream>
#include <queue>

namespace lib::chunk_impl::dremel {

void ReaderCache::SetCache(const FieldReaderPtr& first, const FieldReaderPtr& second, FieldReaderPtr cache_result) {
    if (cache.find(first->GetFieldHash()) == cache.end()) {
        cache[first->GetFieldHash()] = {};
    }
    if (cache.find(second->GetFieldHash()) == cache.end()) {
        cache[second->GetFieldHash()] = {};
    }
    cache[first->GetFieldHash()][second->GetFieldHash()] = cache_result;
    cache[second->GetFieldHash()][first->GetFieldHash()] = cache_result;
}

FieldReaderPtr ReaderCache::LowestCommonAncestor(const FieldReaderPtr& first, const FieldReaderPtr& second) {
    const auto first_it = cache.find(first->GetFieldHash());
    if (first_it != cache.end()) {
        const auto second_it = first_it->second.find(second->GetFieldHash());
        if (second_it != first_it->second.end()) {
            return second_it->second;
        }
    }
    const auto common_ancestor = std::static_pointer_cast<FieldReader>(FieldReader::LowestCommonAncestor(std::static_pointer_cast<FieldDescriptor>(first), std::static_pointer_cast<FieldDescriptor>(second)));
    SetCache(first, second, common_ancestor);
    return common_ancestor;
}

RepetitionLevel ReaderCache::LowestCommonMaxRepetitionLevel(const FieldReaderPtr& first, const FieldReaderPtr& second) {
    const auto common_ancestor = LowestCommonAncestor(first, second);
    return common_ancestor->GetMaxRepetitionLevel();
}

RecordReader::RecordReader(const FieldReaderPtr& root)
    : root_(root)
    , cache_(std::make_shared<ReaderCache>())
    , assembler_(RecordAssembler(cache_, root)) {
    auto leaf_nodes = LeafNodes(root);
    leaf_nodes_.resize(leaf_nodes.size());
    std::transform(
        std::make_move_iterator(leaf_nodes.begin()),
        std::make_move_iterator(leaf_nodes.end()),
        leaf_nodes_.begin(), [](const FieldDescriptorPtr& desc) {
            return std::static_pointer_cast<FieldReader>(desc);
        });
    for (auto i = 0; i < leaf_nodes_.size(); ++i) {
        leaf_nodes_[i]->SetFieldIndex(i);
    }
    ConstructFSM();

    // for (auto& fn : leaf_nodes_) {
    //     std::cerr << "fn " << fn->ToString() << '\n';
    // }

    // for (auto& [k, v] : fsm_) {
    //     std::cerr << "key " << k->ToString() << '\n';
    //     for (auto& vl : v) {
    //         std::cerr << "  value " << (vl == nullptr ? "nullptr" : vl->ToString()) << '\n';
    //     }
    // }
}

void RecordReader::ConstructFSM() {
    for (auto i = 0; i < leaf_nodes_.size(); ++i) {
        const auto& current = leaf_nodes_[i];
        auto max_level = current->GetMaxRepetitionLevel();
        FieldReaderPtr barrier = i + 1 < leaf_nodes_.size() ? leaf_nodes_[i + 1] : nullptr;
        RepetitionLevel barrier_level = barrier == nullptr ? 0 : cache_->LowestCommonMaxRepetitionLevel(current, barrier);

        std::vector<FieldReaderPtr> to_fields(max_level + 1, nullptr);

        for (auto j = 0; j < i + 1; ++j) {
            if (leaf_nodes_[j]->GetMaxRepetitionLevel() <= barrier_level) {
                continue;
            }
            auto back_level = cache_->LowestCommonMaxRepetitionLevel(current, leaf_nodes_[j]);
            if (to_fields[back_level] == nullptr) {
                to_fields[back_level] = leaf_nodes_[j];
            }
        }

        for (auto level = max_level; level > barrier_level; --level) {
            if (to_fields[level] == nullptr) {
                to_fields[level] = to_fields[level + 1];
            }
        }

        for (auto level = 0; level < barrier_level + 1; ++level) {
            to_fields[level] = barrier;
        }

        fsm_[current] = std::move(to_fields);
    }
}

RecordAssembler::RecordAssembler(const std::shared_ptr<ReaderCache>& cache, const FieldReaderPtr& root_node)
    : cache_(cache)
    , root_node_(root_node) {
}

void RecordAssembler::Start() {
    stack_ = std::stack<AssemblerStackEntry>();
    stack_.push({std::make_shared<document::Document>(), root_node_});
    last_node_ = nullptr;
}

void RecordAssembler::AssignValue(const FieldReaderPtr& reader) {
    auto row = reader->ReadRow();
    auto barrier = cache_->LowestCommonAncestor(reader, stack_.top().second);

    if (last_node_ != nullptr && reader->GetFieldIndex() <= last_node_->GetFieldIndex()) {
        while (!barrier->IsRoot() && barrier->GetMaxRepetitionLevel() >= row.repetition_level) {
            barrier = std::static_pointer_cast<FieldReader>(barrier->GetParent());
        }
    }
    while (stack_.top().second != barrier) {
        stack_.pop();
    }

    auto raw_path = FieldDescriptor::GetPath(std::static_pointer_cast<FieldDescriptor>(reader), std::static_pointer_cast<FieldDescriptor>(barrier));
    std::queue<FieldReaderPtr> path;
    for (auto it = raw_path.rbegin(); it != raw_path.rend(); ++it) {
        path.push(std::static_pointer_cast<FieldReader>(*it));
    }

    while (!path.empty() && path.front()->GetDefinitionLevel() <= row.definition_level) {
        auto last = stack_.top().first;
        auto node = path.front();
        path.pop();

        if (node->IsLeaf()) {
            if (node != reader) {
                throw std::logic_error("Unexpected leaf node " + node->ToString() + " before current " + reader->ToString());
            }
            if (!path.empty()) {
                throw std::logic_error("Path queue must be empty here");
            }

            if (node->GetFieldLabel() == FieldLabel::Repeated) {
                if (last->value.find(node->GetFieldName()) == last->value.end()) {
                    last->value[node->GetFieldName()] = std::make_shared<document::List>();
                }
                std::static_pointer_cast<document::List>(last->value[node->GetFieldName()])->value.push_back(row.value);
            } else {
                last->value[node->GetFieldName()] = row.value;
            }
        } else if (!row.value->IsNull()) {
            auto inner = std::make_shared<document::Document>();
            if (node->GetFieldLabel() == FieldLabel::Repeated) {
                if (last->value.find(node->GetFieldName()) == last->value.end()) {
                    last->value[node->GetFieldName()] = std::make_shared<document::List>();
                }
                std::static_pointer_cast<document::List>(last->value[node->GetFieldName()])->value.push_back(inner);
            } else {
                last->value[node->GetFieldName()] = inner;
            }
            stack_.push({inner, node});
        }
    }

    last_node_ = reader;
}

std::shared_ptr<document::Document> RecordAssembler::CollectRecord() {
    while (stack_.size() > 1) {
        stack_.pop();
    }
    auto res = stack_.top();
    stack_.pop();
    return std::static_pointer_cast<document::Document>(res.first);
}

std::shared_ptr<document::Document> RecordReader::NextRecord() {
    // std::cerr << "\nStarting assembling new record\n\n";
    auto current_reader = leaf_nodes_[0];
    assembler_.Start();

    while (current_reader != nullptr) {
        if (current_reader->IsDone()) {
            // all records read
            return nullptr;
        }

        assembler_.AssignValue(current_reader);
        current_reader = fsm_[current_reader][current_reader->NextRepetitionLevel()];
    }

    return assembler_.CollectRecord();
}

} // namespace lib::chunk_impl::dremel
