/****************************************************************************
 * Copyright (c) 2026 Hidegi
 *
 * This software is provided ‘as-is’, without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ****************************************************************************/
#include <bmt.h>
#include <buffer.h>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <limits>
#include <nlohmann/json.hpp>

#define BMT_CLI_VERSION "1.0.0"

using JSON = nlohmann::json;

static void cmd_print(const char* path);
static void cmd_print_json(const char* path);
static void cmd_convert(const char* inputPath, const char* outputPath);
static BMT_node read_from_json(const char* path);
static void parse(const char* key, const JSON& node, BMT_node* tree);
static void parse_list(BMT_tag listTag, const JSON& array, BMT_node* list);
static BMT_tag get_tag(const JSON& node);
static BMT_tag get_number_tag(const JSON& node);
static BMT_tag get_integer_tag(BMT_long value);
static BMT_tag get_integer_tag_range(BMT_long min, BMT_long max);
static BMT_tag get_decimal_tag(BMT_double value);
static BMT_tag get_decimal_tag_range(BMT_double min, BMT_double max);
static BMT_tag get_list_tag(const JSON& node);
static BMT_tag get_array_number_tag(const JSON& array);
static void collect_bst(const BMT_node node, JSON& obj);
static JSON node_value_to_json(const BMT_node node);
static JSON list_to_json_array(const BMT_node list_head);

static void print_usage()
{
    puts("Usage:");
    puts("  bmt -p, --print   <file.bmt>              Print a BMT file");
    puts("  bmt -j, --json    <file.bmt>              Print a BMT file as JSON");
    puts("  bmt -c, --convert <file.json> <out.bmt>   Convert JSON to BMT");
    puts("  bmt -v, --version                          Print version");
    puts("  bmt -h, --help                             Print this help");
}

int main(int argc, char** argv)
{
    enum
    {
        MODE_NONE,
        MODE_PRINT,
        MODE_JSON,
        MODE_CONVERT
    } mode = MODE_NONE;

    static struct option long_options[] = {{"print", no_argument, nullptr, 'p'},
                                           {"json", no_argument, nullptr, 'j'},
                                           {"convert", no_argument, nullptr, 'c'},
                                           {"version", no_argument, nullptr, 'v'},
                                           {"help", no_argument, nullptr, 'h'},
                                           {nullptr, 0, nullptr, 0}};

    int c;
    while ((c = getopt_long(argc, argv, "pjcvh", long_options, nullptr)) != -1)
    {
        switch (c)
        {
            case 'p':
                mode = MODE_PRINT;
                break;
            case 'j':
                mode = MODE_JSON;
                break;
            case 'c':
                mode = MODE_CONVERT;
                break;
            case 'v':
                printf("bmt version %s\n", BMT_CLI_VERSION);
                return EXIT_SUCCESS;
            case 'h':
                print_usage();
                return EXIT_SUCCESS;
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    int remaining = argc - optind;

    switch (mode)
    {
        case MODE_PRINT:
            if (remaining < 1)
            {
                fprintf(stderr, "error: --print requires <file.bmt>\n");
                print_usage();
                return EXIT_FAILURE;
            }
            cmd_print(argv[optind]);
            break;

        case MODE_JSON:
            if (remaining < 1)
            {
                fprintf(stderr, "error: --json requires <file.bmt>\n");
                print_usage();
                return EXIT_FAILURE;
            }
            cmd_print_json(argv[optind]);
            break;

        case MODE_CONVERT:
            if (remaining < 2)
            {
                fprintf(stderr, "error: --convert requires <file.json> and <out.bmt>\n");
                print_usage();
                return EXIT_FAILURE;
            }
            cmd_convert(argv[optind], argv[optind + 1]);
            break;

        default:
            print_usage();
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void cmd_print(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return;
    }

    file.seekg(0, std::ios::end);
    BMT_size length = static_cast<BMT_size>(file.tellg());
    file.seekg(0, std::ios::beg);

    BMT_buffer buf{};
    bmt_BufferReserve(&buf, length);
    file.read(reinterpret_cast<char*>(buf.data), static_cast<std::streamsize>(length));
    buf.length = length;
    file.close();

    BMT_node tree = bmt_DecodeTree(buf);
    bmt_BufferFree(&buf);

    if (!tree)
    {
        fprintf(stderr, "error: '%s' is not a valid BMT file\n", path);
        return;
    }

    bmt_Print(tree);
    bmt_Delete(&tree);
}

static void collect_bst(const BMT_node node, JSON& obj)
{
    if (!node || node->tag == BMT_TAG_NULL)
        return;
    collect_bst(node->minor, obj);
    if (node->name)
        obj[node->name] = node_value_to_json(node);
    collect_bst(node->major, obj);
}

static JSON list_to_json_array(const BMT_node list_head)
{
    JSON arr = JSON::array();
    for (BMT_node c = list_head; c; c = c->major)
    {
        switch (c->tag)
        {
            case BMT_TAG_BYTE:
                arr.push_back(c->payload.tag_byte);
                break;
            case BMT_TAG_SHORT:
                arr.push_back(c->payload.tag_short);
                break;
            case BMT_TAG_INT:
                arr.push_back(c->payload.tag_int);
                break;
            case BMT_TAG_LONG:
                arr.push_back(c->payload.tag_long);
                break;
            case BMT_TAG_FLOAT:
                arr.push_back(c->payload.tag_float);
                break;
            case BMT_TAG_DOUBLE:
                arr.push_back(c->payload.tag_double);
                break;
            case BMT_TAG_STRING:
                arr.push_back(std::string(c->payload.tag_string));
                break;
            case BMT_TAG_ROOT:
            {
                JSON sub = JSON::object();
                collect_bst(c->payload.tag_object, sub);
                arr.push_back(sub);
                break;
            }
            default:
                arr.push_back(list_to_json_array(c->payload.tag_object));
                break;
        }
    }
    return arr;
}

static JSON node_value_to_json(const BMT_node node)
{
    switch (node->tag)
    {
        case BMT_TAG_BYTE:
            return node->payload.tag_byte;
        case BMT_TAG_SHORT:
            return node->payload.tag_short;
        case BMT_TAG_INT:
            return node->payload.tag_int;
        case BMT_TAG_LONG:
            return node->payload.tag_long;
        case BMT_TAG_FLOAT:
            return node->payload.tag_float;
        case BMT_TAG_DOUBLE:
            return node->payload.tag_double;
        case BMT_TAG_STRING:
            return std::string(node->payload.tag_string);
        case BMT_TAG_ROOT:
        {
            JSON sub = JSON::object();
            collect_bst(node->payload.tag_object, sub);
            return sub;
        }
        default:
            return list_to_json_array(node->payload.tag_object);
    }
}

static void cmd_print_json(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return;
    }

    file.seekg(0, std::ios::end);
    BMT_size length = static_cast<BMT_size>(file.tellg());
    file.seekg(0, std::ios::beg);

    BMT_buffer buf{};
    bmt_BufferReserve(&buf, length);
    file.read(reinterpret_cast<char*>(buf.data), static_cast<std::streamsize>(length));
    buf.length = length;
    file.close();

    BMT_node tree = bmt_DecodeTree(buf);
    bmt_BufferFree(&buf);

    if (!tree)
    {
        fprintf(stderr, "error: '%s' is not a valid BMT file\n", path);
        return;
    }

    JSON j = JSON::object();
    collect_bst(tree, j);
    puts(j.dump(2).c_str());
    bmt_Delete(&tree);
}

static void cmd_convert(const char* inputPath, const char* outputPath)
{
    BMT_node tree = read_from_json(inputPath);
    if (!tree)
        return;

    BMT_buffer buf = bmt_EncodeTree(tree);
    bmt_Delete(&tree);

    std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        fprintf(stderr, "error: cannot open output '%s'\n", outputPath);
        bmt_BufferFree(&buf);
        return;
    }

    file.write(reinterpret_cast<const char*>(buf.data), static_cast<std::streamsize>(buf.length));
    printf("written '%s' (%llu bytes)\n", outputPath, static_cast<unsigned long long>(buf.length));
    bmt_BufferFree(&buf);
}

static BMT_node read_from_json(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        fprintf(stderr, "error: cannot open '%s'\n", path);
        return nullptr;
    }

    JSON doc;
    try
    {
        doc = JSON::parse(f);
    }
    catch (const nlohmann::json::exception& e)
    {
        fprintf(stderr, "error: JSON parse failed for '%s': %s\n", path, e.what());
        return nullptr;
    }

    if (doc.is_array())
    {
        fprintf(stderr, "error: top-level JSON array cannot be converted to a BMT tree\n");
        return nullptr;
    }

    BMT_node result = nullptr;
    for (auto& element : doc.items())
        parse(element.key().c_str(), element.value(), &result);

    return result;
}

static void parse(const char* key, const JSON& node, BMT_node* tree)
{
    BMT_tag tag = get_tag(node);
    if (tag == BMT_TAG_NULL)
    {
        fprintf(stderr, "warning: skipping unsupported JSON value for key '%s'\n", key);
        return;
    }

    try
    {
        switch (tag)
        {
            case BMT_TAG_BYTE:
                bmt_InsertByte(tree, key, node.get<BMT_byte>());
                break;
            case BMT_TAG_SHORT:
                bmt_InsertShort(tree, key, node.get<BMT_short>());
                break;
            case BMT_TAG_INT:
                bmt_InsertInt(tree, key, node.get<BMT_int>());
                break;
            case BMT_TAG_LONG:
                bmt_InsertLong(tree, key, node.get<BMT_long>());
                break;
            case BMT_TAG_FLOAT:
                bmt_InsertFloat(tree, key, node.get<BMT_float>());
                break;
            case BMT_TAG_DOUBLE:
                bmt_InsertDouble(tree, key, node.get<BMT_double>());
                break;
            case BMT_TAG_STRING:
                bmt_InsertString(tree, key, node.get<std::string>().c_str());
                break;
            case BMT_TAG_ROOT:
            {
                if (!node.empty())
                {
                    BMT_node subtree = bmt_AllocTree();
                    for (auto& e : node.items())
                        parse(e.key().c_str(), e.value(), &subtree);
                    bmt_Insert(tree, key, subtree);
                    bmt_Delete(&subtree);
                }
                else
                {
                    bmt_CreateTree(tree, key);
                }
                break;
            }
            default:
            {
                if (!node.is_array())
                {
                    fprintf(stderr, "warning: expected array for list tag, skipping '%s'\n", key);
                    return;
                }
                BMT_node list = bmt_AllocList();
                parse_list(tag, node, &list);
                bmt_Insert(tree, key, list);
                bmt_Delete(&list);
                break;
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        fprintf(stderr, "warning: exception for key '%s': %s\n", key, e.what());
    }
}

static void parse_list(BMT_tag listTag, const JSON& array, BMT_node* list)
{
    for (const auto& node : array)
    {
        BMT_tag tag = node.is_array() ? BMT_TAG_LIST : static_cast<BMT_tag>(listTag & ~BMT_TAG_LIST);
        try
        {
            switch (tag)
            {
                case BMT_TAG_BYTE:
                    bmt_AppendByte(list, node.get<BMT_byte>());
                    break;
                case BMT_TAG_SHORT:
                    bmt_AppendShort(list, node.get<BMT_short>());
                    break;
                case BMT_TAG_INT:
                    bmt_AppendInt(list, node.get<BMT_int>());
                    break;
                case BMT_TAG_LONG:
                    bmt_AppendLong(list, node.get<BMT_long>());
                    break;
                case BMT_TAG_FLOAT:
                    bmt_AppendFloat(list, node.get<BMT_float>());
                    break;
                case BMT_TAG_DOUBLE:
                    bmt_AppendDouble(list, node.get<BMT_double>());
                    break;
                case BMT_TAG_STRING:
                    bmt_AppendString(list, node.get<std::string>().c_str());
                    break;
                case BMT_TAG_ROOT:
                {
                    if (!node.empty())
                    {
                        BMT_node subtree = bmt_AllocTree();
                        for (auto& e : node.items())
                            parse(e.key().c_str(), e.value(), &subtree);
                        bmt_Append(list, subtree);
                        bmt_Delete(&subtree);
                    }
                    break;
                }
                default:
                {
                    if (node.is_array() && !node.empty())
                    {
                        BMT_node nested = bmt_AllocList();
                        parse_list(listTag, node, &nested);
                        bmt_Append(list, nested);
                        bmt_Delete(&nested);
                    }
                    break;
                }
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            fprintf(stderr, "warning: exception in list element: %s\n", e.what());
        }
    }
}

static BMT_tag get_integer_tag(BMT_long value)
{
    if (value >= CHAR_MIN && value <= CHAR_MAX)
        return BMT_TAG_BYTE;
    if (value >= SHRT_MIN && value <= SHRT_MAX)
        return BMT_TAG_SHORT;
    if (value >= INT_MIN && value <= INT_MAX)
        return BMT_TAG_INT;
    return BMT_TAG_LONG;
}

static BMT_tag get_integer_tag_range(BMT_long min, BMT_long max)
{
    return std::max(get_integer_tag(min), get_integer_tag(max));
}

static BMT_tag get_decimal_tag(BMT_double value)
{
    return (fabs(static_cast<BMT_double>(static_cast<BMT_float>(value)) - value) < FLT_EPSILON * fabs(value))
               ? BMT_TAG_FLOAT
               : BMT_TAG_DOUBLE;
}

static BMT_tag get_decimal_tag_range(BMT_double min, BMT_double max)
{
    return std::max(get_decimal_tag(min), get_decimal_tag(max));
}

template<typename T>
static void get_min_max(const JSON& array, T& out_min, T& out_max)
{
    out_min = std::numeric_limits<T>::max();
    out_max = std::numeric_limits<T>::lowest();
    for (const auto& el : array)
    {
        T v = el.get<T>();
        if (v < out_min)
            out_min = v;
        if (v > out_max)
            out_max = v;
    }
}

static BMT_tag get_array_number_tag(const JSON& array)
{
    if (array.empty())
        return BMT_TAG_NULL;
    const JSON& front = array[0];
    if (front.is_number_integer())
    {
        BMT_long mn, mx;
        get_min_max<BMT_long>(array, mn, mx);
        return get_integer_tag_range(mn, mx);
    }
    if (front.is_number_float())
    {
        BMT_double mn, mx;
        get_min_max<BMT_double>(array, mn, mx);
        return get_decimal_tag_range(mn, mx);
    }
    return BMT_TAG_NULL;
}

static BMT_tag get_number_tag(const JSON& node)
{
    if (node.is_number_integer())
        return get_integer_tag(node.get<BMT_long>());
    if (node.is_number_float())
        return get_decimal_tag(node.get<BMT_double>());
    return BMT_TAG_NULL;
}

static BMT_tag get_list_tag(const JSON& node)
{
    if (!node.is_array() || node.empty())
        return BMT_TAG_LIST;
    const JSON& front = node[0];
    if (front.is_boolean())
        return static_cast<BMT_tag>(BMT_TAG_LIST | BMT_TAG_BYTE);
    if (front.is_number())
        return static_cast<BMT_tag>(BMT_TAG_LIST | get_array_number_tag(node));
    if (front.is_string())
        return static_cast<BMT_tag>(BMT_TAG_LIST | BMT_TAG_STRING);
    if (front.is_object())
        return static_cast<BMT_tag>(BMT_TAG_LIST | BMT_TAG_ROOT);
    if (front.is_array())
        return static_cast<BMT_tag>(BMT_TAG_LIST | get_list_tag(front));
    return BMT_TAG_LIST;
}

static BMT_tag get_tag(const JSON& node)
{
    if (node.is_object())
        return BMT_TAG_ROOT;
    if (node.is_array())
        return get_list_tag(node);
    if (node.is_boolean())
        return BMT_TAG_BYTE;
    if (node.is_number())
        return get_number_tag(node);
    if (node.is_string())
        return BMT_TAG_STRING;
    return BMT_TAG_NULL;
}
