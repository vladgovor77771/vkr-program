#include <CLI/CLI.hpp>

#include <bin/lib/dataset_generator.h>
#include <bin/lib/read.h>
#include <bin/lib/transform.h>

int main(int argc, char** argv) {
    CLI::App app(
        "Serialization/Deserialization tool for document-like data. "
        "It has implementations for 3 types: JSON, self-written BSON "
        "(JSON with optimizations) and columnar storing type, like Google Dremel",
        "cli");

    cli::TransformArgs transform_args;
    CLI::App* transform = app.add_subcommand(
        "transform",
        "Transforms input data with specified format to another format to output path.");
    transform->add_option("--input-path", transform_args.input_path, "Path to input file.")->required();
    transform->add_option("--input-format", transform_args.input_format, "Input format.")->required();
    transform->add_option("--output-path", transform_args.output_path, "Path to output file.")->required();
    transform->add_option("--output-format", transform_args.output_format, "Output format.")->required();

    cli::ReadArgs read_args;
    CLI::App* read = app.add_subcommand(
        "read",
        "Read data of specified format and possibly write to stdout. Partial read supported.");
    read->add_option("--path", read_args.path, "Path to data file.")->required();
    read->add_option("--format", read_args.format, "Data format.")->required();
    read->add_option("--columns", read_args.columns, "Columns to read.");
    read->add_flag("--write-to-stdout", read_args.write_to_stdout, "Write output to stdout in JSONLINE format.");

    cli::DatasetGeneratorArgs dataset_generator_args;
    CLI::App* generate_dataset = app.add_subcommand(
        "generate-dataset",
        "Generates dataset in JSON format.");
    generate_dataset->add_option("--output-path", dataset_generator_args.output_path, "Path to output file.")->required();
    generate_dataset->add_option("--depth", dataset_generator_args.depth, "Depth of generated schema.")->required();
    generate_dataset->add_option(
                        "--sparsity",
                        dataset_generator_args.sparsity,
                        "Sparcity factor of documents in dataset. "
                        "The higher value is the more sparse documents will be generated.")
        ->default_val(0.05);
    generate_dataset->add_option(
                        "--min-keys-counts",
                        dataset_generator_args.min_keys_count,
                        "The lower bound of the number of keys of nested documents in schema.")
        ->default_val(5);
    generate_dataset->add_option(
                        "--max-keys-counts",
                        dataset_generator_args.max_keys_count,
                        "The upper bound of the number of keys of nested documents in schema.")
        ->default_val(10);
    generate_dataset->add_option(
                        "--min-keys-length",
                        dataset_generator_args.max_keys_count,
                        "The lower bound of the length of keys of nested documents in schema.")
        ->default_val(5);
    generate_dataset->add_option(
                        "--max-keys-length",
                        dataset_generator_args.max_keys_count,
                        "The upper bound of the length of keys of nested documents in schema.")
        ->default_val(10);

    CLI11_PARSE(app, argc, argv);

    if (*transform) {
        cli::RunTransform(std::move(transform_args));
    } else if (*read) {
        cli::RunRead(std::move(read_args));
    } else if (*generate_dataset) {
        cli::RunGenerateDataset(std::move(dataset_generator_args));
    }

    return 0;
}
