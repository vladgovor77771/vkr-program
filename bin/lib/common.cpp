#include "common.h"

#include <lib/chunk_impl/bson.h>
#include <lib/chunk_impl/columnar.h>
#include <lib/chunk_impl/json.h>

namespace cli {

std::shared_ptr<lib::chunk_impl::Chunk> GetChunk(std::string&& path, std::string&& format, std::string&& schema_path) {
    if (format == "json") {
        return std::static_pointer_cast<lib::chunk_impl::Chunk>(std::make_shared<lib::chunk_impl::JsonChunk>(std::move(path)));
    } else if (format == "bson") {
        return std::static_pointer_cast<lib::chunk_impl::Chunk>(std::make_shared<lib::chunk_impl::BsonChunk>(std::move(path)));
    } else if (format == "columnar") {
        return std::static_pointer_cast<lib::chunk_impl::Chunk>(std::make_shared<lib::chunk_impl::ColumnarChunk>(std::move(path), std::move(schema_path)));
    } else {
        throw std::runtime_error("Unknown chunk format, supported formats are [json, bson, columnar]");
    }
}

} // namespace cli
