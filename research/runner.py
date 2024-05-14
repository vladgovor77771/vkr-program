import json
import subprocess
import traceback


def run_cli(command):
    try:
        process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        stdout, stderr = process.communicate()

        if process.returncode != 0:
            print(
                f"Error: Command [{' '.join(command)}] failed with return code {process.returncode}"
            )
            print(stderr)
            return None, None

        return stdout, stderr
    except Exception as e:
        print(f"An error occurred: {traceback.format_exc()}")
        raise e


def run_transform(
    binary, input_path, input_format, output_path, output_format, schema_path=None
):
    command = [
        binary,
        "transform",
        "--input-path",
        input_path,
        "--input-format",
        input_format,
        "--output-path",
        output_path,
        "--output-format",
        output_format,
    ]
    if schema_path is not None:
        command.append("--schema-path")
        command.append(schema_path)
    _, stderr = run_cli(command)
    return json.loads(stderr)


def run_read(binary, path, format, *, schema_path=None, partial_request=[]):
    command = [binary, "read", "--path", path, "--format", format]
    if len(partial_request) > 0:
        command.append(",".join(partial_request))
    if schema_path is not None:
        command.append("--schema-path")
        command.append(schema_path)

    _, stderr = run_cli(command)
    return json.loads(stderr)


def run_generate_schema(
    binary, output_path, *, depth=10, keys_count=[10, 20], keys_length=[10, 20]
):
    command = [
        binary,
        "generate-schema",
        "--output-path",
        output_path,
        "--depth",
        str(depth),
        "--min-keys-count",
        str(keys_count[0]),
        "--max-keys-count",
        str(keys_count[1]),
        "--min-keys-length",
        str(keys_length[0]),
        "--max-keys-length",
        str(keys_length[1]),
    ]
    run_cli(command)


def run_generate_dataset(
    binary,
    output_path,
    schema_path,
    docs_count: int,
    *,
    sparsity=0.05,
    lists_size=[1, 5],
):
    command = [
        binary,
        "generate-dataset",
        "--output-path",
        output_path,
        "--schema-path",
        schema_path,
        "--docs-count",
        str(docs_count),
        "--sparsity",
        str(sparsity),
        "--min-list-size",
        str(lists_size[0]),
        "--max-list-size",
        str(lists_size[1]),
    ]
    run_cli(command)
