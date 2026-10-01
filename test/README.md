# lobif Tests

End-to-end approval tests which are run aginst a compiled `lobif` executable.

## Test structure

Approval tests are divided by `lobif` operation.

| Path       | Description |
| ---        | ---         |
| file/      | Tests for the **write** and **read** operations. |
| page/      | Tests for the **page-search** and **page-get** operations. |
| encoding/  | Tests for the **encode** and **decode** operations. |

### File tests

Approval tests for the **write** and **read** operations are divided into positive and negative tests.

#### Positive file tests

Positive tests ensure when storing or retrieving a given file from the Library of Babel, the expected output is written to `stdout`.
No `stderr` output is expected.

| File path                       | Description |
| ---                             | ---         |
| {test_name}.**src**             | Original file. |
| {test_name}.{encoding}.**addr** | Address of the Library of Babel page containing the file encoded using the method specified in the file name. |

For each pair of .src and .{encoding}.addr files, two tests will be executed:
1. **write** operation: The .src file will be encoded using the method specified in the .addr file name, then the Library of Babel page whose contents matches the encoded file will be located. The returned Library of Babel page address is expected to match the .addr file.
2. **read** operation: The Library of Babel page at the address given given in the .addr file will be fetched, then the page contents will be decoded using the method specified in the .addr file name. The decoded result is expected to match the .src file.

#### Negative file tests

Negative tests ensure when performing a given operation using a given file input, the expected error is written to `stderr`.
No `stdout` output is expected.

| File path                       | Description |
| ---                             | ---         |
| {operation}/{test_name}.**in**  | File input. |
| {operation}/{test_name}.**err** | Expected error output. |

The operation to execute is derived the {operation} directory of the file path (**write** or **read**).

### Page tests

Approval tests for the **page-search** and **page-get** operations are divided into positive and negative tests.

#### Positive page tests

Positive tests ensure when searching for or fetching a Library of Babel page, the expected output is written to `stdout`.
No `stderr` output is expected.

| File path            | Description |
| ---                  | ---         |
| {test_name}.**txt**  | Library of Babel page contents. |
| {test_name}.**addr** | Library of Babel page address. |

For each pair of .txt and .addr files, two tests will be executed:
1. **page-search** operation: The Library of Babel page whose contents matches the .txt file will be searched for. The returned Library of Babel page address is expected to match the .addr file.
2. **page-get** operation: The Library of Babel page at the address given given in the .addr file will be fetched. The returned Library of Babel page contents is expected to match the .txt file.

#### Negative page tests

Negative tests ensure when performing a given operation using a given file input, the expected error is written to `stderr`.
No `stdout` output is expected.

| File path                       | Description |
| ---                             | ---         |
| {operation}/{test_name}.**in**  | File input. |
| {operation}/{test_name}.**err** | Expected error output. |

The operation to execute is derived the {operation} directory of the file path (**page-search** or **page-get**).

### Encoding tests

Approval tests for the **encode** and **decode** operations are divided into positive and negative tests.

#### Positive encoding tests

Positive tests ensure when encoding or decoding a given file, the expected output is written to `stdout`.
No `stderr` output is expected.

| File path                      | Description |
| ---                            | ---         |
| {test_name}.**src**            | Original file. |
| {test_name}.{encoding}.**enc** | File encoded using the method specified in the file name. |

For each pair of .src and .{encoding}.enc files, two tests will be executed:
1. **encode** operation: The .src file will be encoded using the method specified in the .enc file name. The encoded result is expected to match the .enc file.
2. **decode** operation: The .enc file will be decoded using the method specified in the .enc file name. The decoded result is expected to match the .src file.

#### Negative encoding tests

Negative tests ensure when performing a given operation using a given file input, the expected error is written to `stderr`.
No `stdout` output is expected.

| File path                                  | Description |
| ---                                        | ---         |
| {encoding}/{operation}/{test_name}.**in**  | File input. |
| {encoding}/{operation}/{test_name}.**err** | Expected error output. |

The operation to execute is derived from the file path:
- The {operation} directory specifies what operation to execute (**encode** or **decode**).
- The {encoding} directory specifies what file encoding to use (-e option).

## CLI usage

```
$ ./test.sh [-ap] <path>
```

| Option             | Description |
| ---                | ---         |
| `<path>`           | Path to `lobif` executable. |
| `-a`, `--ascii`    | Print ASCII-only text, do not print Unicode text. |
| `-p`, `--no-color` | Print uncolored text. |

### Output

On completion, the script prints the number of passed tests.

If any tests failed, the script will also print the number of failed tests and the path of each failed test.

### Exit statuses

| Value | Description |
| ---   | ---         |
| 0     | Tests passed. |
| 1     | Tests failed. |
| 2     | Invalid command options. |
| 3     | Invalid test structure. |

## Contributing

Please read [CONTRIBUTING.md](../../CONTRIBUTING.md) before making any contributions.
