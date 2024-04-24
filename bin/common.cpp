#include "common.h"

#include <lib/chunk_impl/json.h>
#include <lib/chunk_impl/bson.h>

namespace cli {

std::shared_ptr<lib::Chunk> GetChunk(std::string&& path, std::string&& format) {
    if (format == "json") {
        return std::static_pointer_cast<lib::Chunk>(std::make_shared<lib::chunk_impl::JsonChunk>(std::move(path)));
    } else if (format == "bson") {
        return std::static_pointer_cast<lib::Chunk>(std::make_shared<lib::chunk_impl::BsonChunk>(std::move(path)));
    } else if (format == "columnar") {
        throw std::runtime_error("Not implemented");
    } else {
        throw std::runtime_error("Unknown chunk format, supported formats are [json, bson, columnar]");
    }
}

} // namespace cli
