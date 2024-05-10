#pragma once

#include <memory>

#include <lib/chunk_impl/chunk.h>

namespace cli {

std::shared_ptr<lib::chunk_impl::Chunk> GetChunk(std::string&& path, std::string&& format, std::string&& schema_path);

} // namespace cli
