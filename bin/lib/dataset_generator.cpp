#include "dataset_generator.h"

#include <unordered_set>
#include <ctime>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <lib/chunk_impl/get_stream.h>

namespace cli {

namespace {

    const std::vector<std::string> ZERO_DEPTH_TYPES = {"int", "double", "bool", "string"};
    const std::vector<std::string> INDEPTH_TYPES = {"object", "list"};
    const std::vector<std::string> TYPES = {"int", "double", "bool", "string", "object", "list"};

    int RandomBetween(int min, int max) {
        return std::rand() % (max - min + 1) + min;
    }

    double RandomDouble() {
        return static_cast<double>(std::rand()) / RAND_MAX;
    }

    std::string RandomString(const std::size_t len) {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::string tmp_s;
        tmp_s.reserve(len);

        for (int i = 0; i < len; ++i) {
            tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        return tmp_s;
    }

    template <typename T>
    T RandomElement(const std::vector<T>& elements) {
        return elements[RandomBetween(0, elements.size() - 1)];
    }

    rapidjson::Value GenerateSchemaImpl(
        std::size_t cur_depth,
        const SchemaGeneratorArgs& args,
        rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator) {
        rapidjson::Value schema;
        std::unordered_set<std::string> used_keys;
        schema.SetObject();

        const auto chosen_keys_count = RandomBetween(args.min_keys_count, args.max_keys_count);
        bool at_least_one_in_depth = false;
        for (auto i = 0; i < chosen_keys_count; ++i) {
            const auto chosen_key_length = RandomBetween(args.min_keys_length, args.max_keys_length);
            std::string chosen_key;
            while (true) {
                chosen_key = RandomString(chosen_key_length);
                if (used_keys.find(chosen_key) == used_keys.end()) {
                    used_keys.insert(chosen_key);
                    break;
                }
            }

            std::string chosen_type;
            if (cur_depth == 0) {
                chosen_type = RandomElement(ZERO_DEPTH_TYPES);
            } else if (!at_least_one_in_depth) {
                chosen_type = RandomElement(INDEPTH_TYPES);
                at_least_one_in_depth = true;
            } else {
                chosen_type = RandomElement(TYPES);
            }

            auto key = rapidjson::Value(chosen_key.c_str(), allocator);
            if (std::find(ZERO_DEPTH_TYPES.begin(), ZERO_DEPTH_TYPES.end(), chosen_type) != ZERO_DEPTH_TYPES.end()) {
                auto value = rapidjson::Value(chosen_type.c_str(), allocator);
                schema.AddMember(std::move(key), std::move(value), allocator);
            } else if (chosen_type == "object") {
                auto value = GenerateSchemaImpl(cur_depth - 1, args, allocator);
                schema.AddMember(std::move(key), std::move(value), allocator);
            } else if (chosen_type == "list") {
                auto value = rapidjson::Value();
                value.SetArray();
                value.PushBack(GenerateSchemaImpl(cur_depth - 1, args, allocator), allocator);
                schema.AddMember(std::move(key), std::move(value), allocator);
            }
        }

        return schema;
    }

    rapidjson::Value GenerateDocument(const DatasetGeneratorArgs& args, const rapidjson::Value& schema, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator) {
        rapidjson::Value doc_part;
        doc_part.SetObject();

        for (auto& kv : schema.GetObject()) {
            const auto null_check = RandomDouble();
            if (null_check < args.sparsity / 2) {
                continue;
            } else if (null_check < args.sparsity) {
                doc_part.AddMember(rapidjson::Value(kv.name, allocator), rapidjson::Value(), allocator);
            } else if (kv.value.IsString()) {
                auto type = std::string(kv.value.GetString());
                if (type == "int") {
                    doc_part.AddMember(rapidjson::Value(kv.name, allocator), rapidjson::Value(RandomBetween(-10000, 10000)), allocator);
                } else if (type == "double") {
                    doc_part.AddMember(rapidjson::Value(kv.name, allocator), rapidjson::Value(RandomDouble()), allocator);
                } else if (type == "bool") {
                    doc_part.AddMember(rapidjson::Value(kv.name, allocator), rapidjson::Value(RandomDouble() > 0.5), allocator);
                } else if (type == "string") {
                    doc_part.AddMember(rapidjson::Value(kv.name, allocator), rapidjson::Value(RandomString(10).c_str(), allocator), allocator);
                } else {
                    throw std::runtime_error("Unknown type in schema: " + type);
                }
            } else if (kv.value.IsObject()) {
                doc_part.AddMember(rapidjson::Value(kv.name, allocator), GenerateDocument(args, kv.value.GetObject(), allocator), allocator);
            } else if (kv.value.IsArray()) {
                const auto& inner_schema = kv.value.GetArray()[0];
                const auto size = RandomBetween(args.min_list_size, args.max_list_size);
                rapidjson::Value list;
                list.SetArray();
                for (auto i = 0; i < size; ++i) {
                    list.PushBack(GenerateDocument(args, inner_schema, allocator), allocator);
                }
                doc_part.AddMember(rapidjson::Value(kv.name, allocator), std::move(list), allocator);
            } else {
                throw std::runtime_error("Unknown type in schema");
            }
        }

        return doc_part;
    }

} // namespace

void RunGenerateDataset(DatasetGeneratorArgs&& args) {
    std::srand(std::time(nullptr));

    auto istream = lib::chunk_impl::GetInputStream(args.schema_path);
    std::string schema_json_str;
    *istream >> schema_json_str;

    rapidjson::Document schema;

    if (schema.Parse(schema_json_str.c_str()).HasParseError()) {
        throw std::runtime_error("Failed to parse schema JSON");
    }

    if (!schema.IsObject()) {
        throw std::runtime_error("Schema JSON is not an object");
    }

    auto ostream = lib::chunk_impl::GetOutputStream(args.output_path);

    std::cerr << "Generating documents..." << '\n';
    for (auto i = 0; i < args.docs_count; ++i) {
        rapidjson::Document doc;
        auto doc_value = GenerateDocument(args, schema.GetObject(), doc.GetAllocator());
        doc.Swap(doc_value);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        *ostream << buffer.GetString() << '\n';
        std::cerr << "Generated " << i << '/' << args.docs_count << " documents\n";
    }
}

void RunGenerateSchema(SchemaGeneratorArgs&& args) {
    std::srand(std::time(nullptr));

    rapidjson::Document doc;

    auto schema = GenerateSchemaImpl(args.depth, args, doc.GetAllocator());
    doc.Swap(schema);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    auto stream = lib::chunk_impl::GetOutputStream(args.output_path);
    *stream << buffer.GetString();
}

} // namespace cli
