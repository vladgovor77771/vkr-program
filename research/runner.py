import json
import subprocess


def run_cli(command):
    try:
        process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        _, stderr = process.communicate()

        if process.returncode != 0:
            print(
                f"Error: Command [{' '.join(command)}] failed with return code {process.returncode}"
            )
            print(stderr)
            return {}

        try:
            result = json.loads(stderr)
            return result
        except json.JSONDecodeError:
            print("Error: Failed to decode JSON from stderr")
            return {}
    except Exception as e:
        print(f"An error occurred: {e}")
        return {}


def run_transform(binary, input_path, input_format, output_path, output_format):
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
    return run_cli(command)


def run_read(binary, path, format, partial_request=[]):
    command = [binary, "transform", "--path", path, "--format", format]
    if len(partial_request) > 0:
        command.append(",".join(partial_request))
    return run_cli(command)
