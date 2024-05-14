#pragma once

#include <stack>

#include <lib/chunk_impl/dremel/field_reader.h>
#include <lib/document/document.h>

namespace lib::chunk_impl::dremel {

using FSM = std::unordered_map<FieldReaderPtr, std::vector<FieldReaderPtr>, FieldHasher>;

class ReaderCache {
private:
    std::unordered_map<FieldHashType, std::unordered_map<FieldHashType, FieldReaderPtr>> cache;

    void SetCache(const FieldReaderPtr& first, const FieldReaderPtr& second, FieldReaderPtr cache_result);

public:
    ReaderCache(){};

    FieldReaderPtr LowestCommonAncestor(const FieldReaderPtr& first, const FieldReaderPtr& second);
    RepetitionLevel LowestCommonMaxRepetitionLevel(const FieldReaderPtr& first, const FieldReaderPtr& second);
};

using AssemblerStackEntry = std::pair<std::shared_ptr<document::Document>, FieldReaderPtr>;

class RecordAssembler {
private:
    std::shared_ptr<ReaderCache> cache_;
    FieldReaderPtr root_node_;
    FieldReaderPtr last_node_;
    std::stack<AssemblerStackEntry> stack_;

public:
    RecordAssembler(const std::shared_ptr<ReaderCache>& cache, const FieldReaderPtr& root_node_);

    void Start();
    void AssignValue(const FieldReaderPtr& reader);

    // also flushes self
    std::shared_ptr<document::Document> CollectRecord();
};

class RecordReader {
private:
    FSM fsm_;
    std::shared_ptr<ReaderCache> cache_;
    std::vector<FieldReaderPtr> leaf_nodes_;
    FieldReaderPtr root_;
    RecordAssembler assembler_;

    void ConstructFSM();

public:
    explicit RecordReader(const FieldReaderPtr& root);

    std::shared_ptr<document::Document> NextRecord();
};

} // namespace lib::chunk_impl::dremel
