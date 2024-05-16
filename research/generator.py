import random

ZERO_DEPTH_TYPES = ["int", "double", "string", "bool"]
INDEPTH_TYPES = ["object", "list"]
TYPES = ZERO_DEPTH_TYPES + INDEPTH_TYPES


def generate_partial_request(schema, request_ratio=0.1):
    request = []

    def wrapper(obj: dict, cur_path=""):
        for k, v in obj.items():
            inner = cur_path + "." + k if len(cur_path) > 0 else k
            if v in ZERO_DEPTH_TYPES:
                if random.random() < request_ratio:
                    request.append(inner)
            elif isinstance(v, dict):
                wrapper(v, inner)
            elif isinstance(v, list):
                wrapper(v[0], inner)

    wrapper(schema)
    return request
