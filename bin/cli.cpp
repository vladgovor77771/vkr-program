#include <CLI/CLI.hpp>

#include <bin/transform.h>
#include <bin/read.h>

int main(int argc, char** argv) {
    CLI::App app(
        "Serialization/Deserialization tool for document-like data. "
        "It has implementations for 3 types: JSON, self-written BSON "
        "(JSON with optimizations) and columnar storing type, like Google Dremel",
        "chunk-processor");

    cli::TransformArgs transform_args;
    CLI::App* transform = app.add_subcommand(
        "transform",
        "Transforms input data with specified format to another format to output path.");
    transform->add_option("--input-path", transform_args.input_path, "Path to input file")->required();
    transform->add_option("--input-format", transform_args.input_format, "Input format")->required();
    transform->add_option("--output-path", transform_args.output_path, "Path to output file")->required();
    transform->add_option("--output-format", transform_args.output_format, "Output format")->required();

    cli::ReadArgs read_args;
    CLI::App* read = app.add_subcommand(
        "read",
        "Read data of specified format and possibly write to stdout. Partial read supported.");
    read->add_option("--path", read_args.path, "Path to data file")->required();
    read->add_option("--format", read_args.format, "Data format")->required();
    read->add_option("--columns", read_args.columns, "Columns to read");
    read->add_flag("--write-to-stdout", read_args.write_to_stdout, "Write output to stdout in JSONLINE format");

    CLI11_PARSE(app, argc, argv);

    if (*transform) {
        cli::RunTransform(std::move(transform_args));
    }

    if (*read) {
        cli::RunRead(std::move(read_args));
    }

    return 0;
}
