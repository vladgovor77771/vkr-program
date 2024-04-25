import json
import random
import string

ZERO_DEPTH_TYPES = ["int", "float", "string", "bool"]
INDEPTH_TYPES = ["object", "list"]
TYPES = ZERO_DEPTH_TYPES + INDEPTH_TYPES


def generate_schema(*, depth=10, keys_count=[10, 20], key_length=[10, 20]):
    schema = {}
    chosen_keys_count = random.randint(keys_count[0], keys_count[1])

    at_least_one_in_depth = False
    for i in range(chosen_keys_count):
        chosen_key_length = random.randint(key_length[0], key_length[1])
        chosen_key = None
        while True:
            chosen_key = "".join(
                random.choices(
                    string.ascii_letters + string.digits, k=chosen_key_length
                )
            )

            if chosen_key not in schema:
                break

        if depth == 0:
            chosen_type = random.choice(ZERO_DEPTH_TYPES)
        elif not at_least_one_in_depth and i == chosen_keys_count - 1:
            chosen_type = random.choice(INDEPTH_TYPES)
        else:
            chosen_type = random.choice(TYPES)

        if chosen_type in ZERO_DEPTH_TYPES:
            schema[chosen_key] = chosen_type
        elif chosen_type == "object":
            schema[chosen_key] = generate_schema(
                depth=depth - 1, keys_count=keys_count, key_length=key_length
            )
        elif chosen_type == "list":
            schema[chosen_key] = [
                generate_schema(
                    depth=depth - 1, keys_count=keys_count, key_length=key_length
                )
            ]

    return schema


def generate_document(schema: dict, *, sparcity=0.05):
    doc = {}

    for k, v in schema.items():
        if random.random() < sparcity:
            doc[k] = None
        elif v == "int":
            doc[k] = random.randint(-1000000, 1000000)
        elif v == "float":
            doc[k] = random.random() * random.randint(-1000000, 1000000)
        elif v == "string":
            str_len = random.randint(0, 25)
            doc[k] = "".join(
                random.choices(string.ascii_letters + string.digits, k=str_len)
            )
        elif v == "bool":
            doc[k] = random.random() < 0.5
        elif isinstance(v, dict):
            doc[k] = generate_document(v, sparcity=sparcity)
        elif isinstance(v, list):
            list_len = random.randint(5, 20)
            doc[k] = [
                generate_document(v[0], sparcity=sparcity) for _ in range(list_len)
            ]
    return doc


def generate_documents(schema: dict, *, cnt=1000, sparcity=0.05):
    return [generate_document(schema, sparcity=sparcity) for _ in range(cnt)]


def generate_partial_request(schema, request_ratio=0.1):
    request = []

    def wrapper(obj: dict, cur_path=""):
        for k, v in obj.items():
            if v in ZERO_DEPTH_TYPES:
                if random.random() < request_ratio:
                    request.append(cur_path + "." + k if len(cur_path) > 0 else k)
            elif isinstance(v, dict):
                wrapper(v, k)
            elif isinstance(v, list):
                wrapper(v[0], k)

    wrapper(schema)
    return request


def prepare_data_file(docs: list[dict], path):
    with open(path, "w+") as f:
        for doc in docs:
            f.write(json.dumps(doc) + "\n")
