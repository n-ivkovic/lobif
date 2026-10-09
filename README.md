# lobif

lobif is a CLI to store and retrieve files from pages in [Jonathan Basile's Library of Babel](https://libraryofbabel.info/).

A page in the Library of Babel contains 3200 characters, consisting of `a-z`, `.`, and `,`.
Using the default Babel28 encoding scheme, 1920 bytes of data can be stored and retrieved from a page in the Library of Babel.

```
$ cat my_file
Hello, world!
This is my file.

$ lobif write my_file > my_addr
$ lobif read my_addr
Hello, world!
This is my file.

$ lobif page-get my_addr
fsbskjnsecbkxyfkdowcbmpxgjbrznbktijbkurdbksipjnoyu..zd

$ lobif page-get my_addr | lobif decode
Hello, world!
This is my file.

```

Written in C99 and complies with [POSIX.1-2001](https://pubs.opengroup.org/onlinepubs/000095399/) through to [POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/).

## Usage

```
$ lobif <operation> [-e <encoding-scheme>] [-f <addr-format>] [-o <path>] [<path>]
```

| Argument               | Description |
| ---                    | ---         |
| `<operation>`          | The operation to be performed. Required, must be the first provided argument. |
| `<path>`               | Path to read operation input from. If path is `-` or not given, input will be read from `stdin`. |
| -o `<path>`            | Path to write operation output to. If path is `-` or not given, output will be written to `stdout`. |
| -e `<encoding-scheme>` | Binary-to-text encoding scheme to use when encoding or decoding data as part of an operation. Usable only by the **write**, **read**, **encode**, and **decode** operations. If not given, the Babel28 encoding scheme will be used. |
| -f `<addr-format>`     | Page address format to use when loading or outputting a Library of Babel page address as part of an operation. Usable only by the **write**, **read**, **page-search**, and **page-get** operations. If not given, the string page address format will be used. |

### Operations

#### write

The **write** operation:

1. Performs binary-to-text encoding on the given data (`<path>` or `stdin`).
2. Locates the Library of Babel page whose contents matches the encoded data.

The operation outputs the address of the Library of Babel page where the encoded data can be found.

> [!WARNING]
> Input data that exceeds the Library of Babel page size (1920 bytes when using the Babel28 encoding scheme) will be silently truncated.

#### read

The **read** operation:

1. Fetches the Library of Babel page at the given address (`<path>` or `stdin`).
2. Performs text-to-binary decoding on the page's contents.

The operation outputs the data decoded from the fetched Library of Babel page.

#### page-search

The **page-search** operation locates the Library of Babel page whose contents matches the given text (`<path>` or `stdin`).
The operation is equivalent to step #2 of the **write** operation.

The operation outputs the address of the located Library of Babel page.

#### page-get

The **page-get** operation fetches the Library of Babel page at the given address (`<path>` or `stdin`).
The operation is equivalent to step #1 of the **read** operation.

The operation outputs the contents of the fetched the Library of Babel page.

#### encode

The **encode** operation performs binary-to-text encoding on the given data (`<path>` or `stdin`).
The operation is equivalent to step #1 of the **write** operation.

The operation outputs the encoded string.

#### decode

The **decode** operation performs text-to-binary decoding on the given string (`<path>` or `stdin`).
The operation is equivalent to step #2 of the **read** operation.

The operation outputs the decoded data.

### Data encoding schemes

Multiple binary-to-text encoding schemes for storing data within the Library of Babel are supported.

| Short name | Full name         | Bytes per page | Description |
| ---        | ---               | ---            | ---         |
| bhex, b16  | BabelHex, Babel16 | 1600           | Hexadecimal using characters `a-p` instead of the conventional `0-f`. |
| b28        | Babel28           | 1920           | Default encoding scheme. Encodes data in blocks of 3 bytes, outputting each block as 5 characters of `a-z`, `.`, or `,`. Based on the [Ascii85](https://en.wikipedia.org/wiki/Ascii85) encoding scheme. |

Either the short or full encoding scheme name can be provided to the `-e` argument.

### Page address formats

Multiple formats for Library of Babel page addresses are supported.

| Short name | Full name    | Bytes per address | Description |
| ---        | ---          | ---               | ---         |
| str, txt   | String, Text | 3274              | Default page address format. Original format used by the Library of Babel. |
| bin        | Binary       | 2110              | Formats page addresses as two little-endian integer values: the hexagon number and the page number within the hexagon. |

Either the short or full format name can be provided to the `-f` argument.

## Build

Ensure the following is available on your system:

- GCC or clang. To use clang, `CC=clang` must be passed to make commands.
- curl

Clone and build:
(Replace `github.com` with `gitlab.com` if using GitLab)
```
$ git clone https://github.com/n-ivkovic/lobif.git
$ cd lobif
$ make
```

Run:
```
$ ./lobif ...
```

Additional options:
```
$ make help
```

## Contributing

Please read [CONTRIBUTING.md](./CONTRIBUTING.md) before making any contributions.

## License

Copyright &copy; 2026 Nicholas Ivkovic.

Licensed under the GNU General Public License version 3 or later.
See [LICENSE](./LICENSE), or [https://gnu.org/licenses/gpl.html](https://gnu.org/licenses/gpl.html) if more recent, for details.

This is free software: you are free to change and redistribute it. There is NO WARRANTY, to the extent permitted by law.
