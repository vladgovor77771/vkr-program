#include "transform.h"

#include <chrono>
#include <iostream>

#include <bin/lib/common.h>

namespace cli {

void RunTransform(TransformArgs&& args) {
    const auto input_chunk = GetChunk(std::move(args.input_path), std::move(args.input_format));
    const auto output_chunk = GetChunk(std::move(args.output_path), std::move(args.output_format));

    auto start = std::chrono::high_resolution_clock::now();
    const auto documents = input_chunk->Read();
    auto stop = std::chrono::high_resolution_clock::now();
    const auto duration_read = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    start = std::chrono::high_resolution_clock::now();
    output_chunk->Write(documents);
    stop = std::chrono::high_resolution_clock::now();
    const auto duration_write = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    std::cerr << "{\"read_duration_ns\": " << duration_read.count() << ", \"write_duration_ns\": " << duration_write.count() << "}\n";
}

} // namespace cli
