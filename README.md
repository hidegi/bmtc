# bmtc

A command-line tool for working with BMT-files. It can inspect `.bmt` files and convert JSON to BMT.

BMT is a compact binary serialization format backed by a Red-Black tree (similar to Minecraft's NBT format but open and unencumbered). Keys are lexicographically ordered and the file is zlib-compressed. See [BMT-C-API](https://github.com/hidegi/BMT-C-API) for the full format spec.

## Usage

```
bmtc -p, --print   <file.bmt>              Print a BMT file
bmtc -c, --convert <file.json> <out.bmt>   Convert JSON to BMT
bmtc -v, --version                          Print version
bmtc -h, --help                             Print this help
```

## Flags

| Flag | Long form | Arguments | Description |
|------|-----------|-----------|-------------|
| `-p` | `--print` | `<file.bmt>` | Decode and pretty-print a BMT file |
| `-c` | `--convert` | `<file.json> <out.bmt>` | Convert a JSON file to BMT |
| `-v` | `--version` | — | Print the tool version |
| `-h` | `--help` | — | Print usage |

## JSON → BMT conversion

The converter maps JSON types to the smallest fitting BMT type automatically:

| JSON type | BMT type |
|-----------|----------|
| integer | `byte` / `short` / `int` / `long` (smallest that fits the value) |
| float | `float` or `double` (precision-preserving) |
| string | `string` |
| boolean | `byte` (0 or 1) |
| object | tree (subtree) |
| array | typed list (`byte-list`, `int-list`, `string-list`, etc.) |
| null | skipped with a warning |

For numeric arrays the converter scans all elements to pick the smallest type that fits the entire array. The top-level JSON value must be an object (not an array).

## Build

Requires CMake ≥ 3.14, a C++17 compiler, and GNU Make. Dependencies (`bmt-api`, `nlohmann/json`) are fetched automatically by CMake.

```sh
cmake -Bbuild -G"Unix Makefiles"
cd build && make
```

The binary is at `build/bmtc` (or `build/bmt-cli` depending on generator output).
