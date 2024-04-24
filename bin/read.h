#pragma once

#include <string>

namespace cli {

struct ReadArgs {
    std::string path;
    std::string format;
    std::string columns;
    bool write_to_stdout;
};


void RunRead(ReadArgs&& args);

} // namespace cli
