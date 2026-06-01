# lobif

Locate/'write' files to, and read files from, pages in [Jonathan Basile's Library of Babel](https://libraryofbabel.info/).

A page in the Library of Babel can store a maximum of 1920 bytes using the default encoding method (Babel28).

Written in C99 and complies with [POSIX.1-2001](https://pubs.opengroup.org/onlinepubs/000095399/) through to [POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/).

## Installation

Ensure the following is available on your system:

- GCC or clang. To use clang, `CC=clang` must be passed to make commands.
- curl

Clone and build:
(Replace `github.com` with `gitlab.com` if using GitLab)
```
$ git clone https://github.com/n-ivkovic/lobif
cd lobif
$ make
```

Run without installing:
```
$ ./lobif ...
```

Install and run:
```
# make install
$ lobif ...
```

Update after installing:
```
$ git pull master
$ make
# make install
```

Additional options:
```
$ make help
```

## Usage

```
$ lobif <operation> [-e <encoding>] [-o <path>] [<path>]
```

| Operation | Description |
| ---       | ---         |
| read      | Read file contents from a page in the Library of Babel. Outputs the decoded file contents. |
| write     | Locate/'write' a file to the Library of Babel. Outputs the page address the encoded file contents are located at. |
| encode    | Encode a file for storage in the Library of Babel. Outputs the encoded file contents. |
| decode    | Decode an encoded file. Outputs the decoded file contents. |

| Option          | Description |
| ---             | ---         |
| -e `<encoding>` | File encoding method to use when encoding or decoding files. |
| -o `<path>`     | Path to output result of the operation. |

## File encoding methods

| Short name | Full name         | Bytes per page | Description |
| ---        | ---               | ---            | ---         |
| bhex, b16  | BabelHex, Babel16 | 1600           | Hexadecimal using characters `a-p` instead of the conventional `0-f`. Encodes each byte as 2 characters. |
| b28        | Babel28           | 1920           | Default encoding method. Based on [Ascii85](https://en.wikipedia.org/wiki/Ascii85). Encodes blocks of 3 bytes as 5 characters using the `.`, `,`, and `a-z` characters. |

## Contributing

Please read [CONTRIBUTING.md](./CONTRIBUTING.md) before making any contributions.

## License

Copyright &copy; 2026 Nicholas Ivkovic.

Licensed under the GNU General Public License version 3 or later.
See [LICENSE](./LICENSE), or [https://gnu.org/licenses/gpl.html](https://gnu.org/licenses/gpl.html) if more recent, for details.

This is free software: you are free to change and redistribute it. There is NO WARRANTY, to the extent permitted by law.
