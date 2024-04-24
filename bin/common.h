#pragma once

#include <memory>

#include <lib/chunk_impl/chunk.h>

namespace cli {

std::shared_ptr<lib::Chunk> GetChunk(std::string&& path, std::string&& format);

} // namespace cli
