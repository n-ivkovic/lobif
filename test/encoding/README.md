# lobif Encoding Tests

End-to-end approval tests for encoding and decoding files which are run aginst a compiled `lobif` executable.

## Test structure

Approval tests are divided into positive and negative tests.

### Positive tests

Positive tests ensure when encoding or decoding a given file, the expected output is written to `stdout`.
No `stderr` output is expected.

| File path                      | Description |
| ---                            | ---         |
| {test_name}.**src**            | Unencoded file. |
| {test_name}.{encoding}.**enc** | File encoded using the method specified in the file name. |

For each pair of .src and .{encoding}.enc files, two tests will be executed:
- The .src will be encoded (using the method specified in the .enc file name) and is expected to match the .enc file.
- The .enc file will be decoded (using the method specified in the .enc file name) and is expected to match the .src file.

### Negative tests

Negative tests ensure when performing a given operation using a given file input, the expected error is written to `stderr`.
No `stdout` output is expected.

| File path           | Description |
| ---                 | ---         |
| {encoding}/{operation}/{test_name}.**in**  | File input. |
| {encoding}/{operation}/{test_name}.**err** | Expected error output. |

The operation to execute is derived from the file path:
- The {operation} directory specifies what `lobif` operation to execute.
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
