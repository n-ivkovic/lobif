#define _XOPEN_SOURCE 600

#include "encoding_bhex.h"
#include "encoding_b28.h"
#include "lobi.h"
#include "str.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define EOL "\n"
#define ARG_STDIN "-"
#define ARG_STDOUT "-"

#define FP_READ_MAX 0xFFFFF
#define FP_READ_ADDR (LOBI_PAGE_ADDR_LEN + 0x10) // Address length + some buffer for whitespace

#define DYNARR_STR(da) (char*)da.vals
#define DYNARR_STR_LEN(da) (da.len - 1)

/**
 * CLI operation.
 */
enum operation {
    PAGE_SEARCH_ENCODE_E,
    PAGE_GET_DECODE_E,
    PAGE_SEARCH_E,
    PAGE_GET_E,
    ENCODE_E,
    DECODE_E
};

/**
 * Library of Babel encoding method.
 */
enum encoding {
    BHEX_E,
    B28_E
};

/**
 * Parse CLI operation from string.
 *
 * @param opr Parsed CLI operation.
 * @param str String to parse.
 * @param len Length of string to parse.
 */
static bool operation_parse(enum operation* opr, const char* str, const size_t len)
{
    struct operation_str {
        enum operation opr;
        char str_lower[STR_CHARS(11)];
    };

    static const struct operation_str opr_strs[] = {
        { .opr = PAGE_SEARCH_ENCODE_E, .str_lower = "write" },
        { .opr = PAGE_GET_DECODE_E, .str_lower = "read" },
        { .opr = PAGE_SEARCH_E, .str_lower = "page-search" },
        { .opr = PAGE_GET_E, .str_lower = "page-get" },
        { .opr = ENCODE_E, .str_lower = "encode" },
        { .opr = DECODE_E, .str_lower = "decode" }
    };

    if (!opr || !str)
        return false;

    for (size_t ind = 0; ind < sizeof(opr_strs) / sizeof(opr_strs[0]); ind++) {
        struct operation_str opr_str = opr_strs[ind];
        if (str_comp(str, opr_str.str_lower, STR_CHARS(len), tolower) == 0) {
            *opr = opr_str.opr;
            return true;
        }
    }

    return false;
}

/**
 * Parse encoding method from string.
 *
 * @param enc Parsed encoding method.
 * @param str String to parse.
 * @param len Length of string to parse.
 */
static bool encoding_parse(enum encoding* enc, const char* str, const size_t len)
{
    struct encoding_str {
        enum encoding enc;
        char str_lower[STR_CHARS(8)];
    };

    static const struct encoding_str enc_strs[] = {
        { .enc = BHEX_E, .str_lower = "bhex" },
        { .enc = BHEX_E, .str_lower = "babelhex" },
        { .enc = BHEX_E, .str_lower = "b16" },
        { .enc = BHEX_E, .str_lower = "babel16" },
        { .enc = B28_E, .str_lower = "b28" },
        { .enc = B28_E, .str_lower = "babel28" },
    };

    if (!enc || !str)
        return false;

    for (size_t ind = 0; ind < sizeof(enc_strs) / sizeof(enc_strs[0]); ind++) {
        struct encoding_str enc_str = enc_strs[ind];
        if (str_comp(str, enc_str.str_lower, STR_CHARS(len), tolower) == 0) {
            *enc = enc_str.enc;
            return true;
        }
    }

    return false;
}

/**
 * Encode bytes as string.
 *
 * @param err Buffer to write error messages to.
 * @param str Struct to output encoded string to, including null-terminator.
 * @param bytes Bytes to encode.
 * @param n Number of bytes to encode.
 * @param enc Encoding method.
 * @returns Whether bytes were encoded successfully.
 */
static bool encode(char* err, struct dynarr* str, const uint8_t* bytes, const size_t n, const enum encoding enc)
{
    #define ENCODE_ALLOC(str, n) dynarr_alloc(str, n, sizeof(char))

    if (!str) {
        if (err) sprintf(err, "Struct to output encoded string not given");
        return false;
    }

    bool success = false;
    bool str_unalloc = !str->vals;

    switch (enc) {
        case BHEX_E:
            if (str_unalloc)
                ENCODE_ALLOC(str, bhex_encode_len(n));

            if (!bhex_encode(str, bytes, n)) {
                if (err) sprintf(err, "Failed to encode input as BabelHex string");
                goto exit;
            }

            success = true;
            goto exit;

        case B28_E:
            if (str_unalloc)
                ENCODE_ALLOC(str, b28_encode_len(n));

            if (!b28_encode(str, bytes, n)) {
                if (err) sprintf(err, "Failed to encode input as Babel28 string");
                goto exit;
            }

            success = true;
            goto exit;

        default:
            if (err) sprintf(err, "Unknown encoding given: %d", enc);
            goto exit;
    }

    exit:
    if (!success && str_unalloc)
        dynarr_empty(str);

    return success;
}

/**
 * Decode bytes from encoded string.
 *
 * @param err Buffer to write error messages to.
 * @param bytes Struct to output decoded bytes to.
 * @param str Encoded string to decode.
 * @param len Length of encoded string to decode, excluding null-terminator.
 * @param enc Encoding method.
 * @returns Whether string was decoded successfully.
 */
static bool decode(char* err, struct dynarr* bytes, const char* str, const size_t len, const enum encoding enc)
{
    #define DECODE_ALLOC(bytes, n) dynarr_alloc(bytes, n, sizeof(uint8_t))

    if (!bytes) {
        if (err) sprintf(err, "Struct to output decoded bytes not given");
        return false;
    }

    bool success = false;

    bool bytes_unalloc = !bytes->vals;
    size_t bytes_n;

    char* str_tr = NULL;
    size_t str_tr_len;

    // Alloc space to trim whitespace from input string
    str_tr = calloc(STR_CHARS(len), sizeof(char));
    if (!str_tr) {
        if (err) sprintf(err, "Failed to allocate memory");
        goto exit;
    }

    // Trim whitespace from input string
    if (!str_trim(str_tr, str, len)) {
        if (err) sprintf(err, "Failed to trim input string");
        goto exit;
    }

    str_tr_len = strlen(str_tr);
    str_tr_len = str_tr_len > len ? len : str_tr_len;

    switch (enc) {
        case BHEX_E:
            if (!bhex_decode_len(&bytes_n, str_tr_len)) {
                if (err) sprintf(err, "Invalid BabelHex input given");
                goto exit;
            }

            if (bytes_unalloc)
                DECODE_ALLOC(bytes, bytes_n);

            if (!bhex_decode(bytes, str_tr, str_tr_len)) {
                if (err) sprintf(err, "Failed to decode BabelHex input");
                goto exit;
            }

            success = true;
            goto exit;

        case B28_E:
            if (!b28_decode_len(&bytes_n, str_tr_len)) {
                if (err) sprintf(err, "Invalid Babel28 input given");
                goto exit;
            }

            if (bytes_unalloc)
                DECODE_ALLOC(bytes, bytes_n);

            if (!b28_decode(bytes, str_tr, str_tr_len)) {
                if (err) sprintf(err, "Failed to decode Babel28 input");
                goto exit;
            }

            success = true;
            goto exit;

        default:
            if (err) sprintf(err, "Unknown encoding given: %d", enc);
            goto exit;
    }

    exit:
    if (!success && bytes_unalloc)
        dynarr_empty(bytes);

    if (str_tr) free(str_tr);
    return success;
}

/**
 * Get number of bytes to decode from an encoded string of a given length.
 *
 * @param n Result to write number of bytes to decode from encoded string of given length.
 * @param len Length of encoded string to decode, excluding null-terminator.
 * @param enc Encoding method.
 */
static bool decode_len(size_t* n, const size_t len, const enum encoding enc)
{
    switch (enc) {
        case BHEX_E:
            return bhex_decode_len(n, len);
        case B28_E:
            return b28_decode_len(n, len);
        default:
            return false;
    }
}

/**
 * Read bytes from input file.
 *
 * @param err Buffer to write error messages to.
 * @param bytes Dynamic arrays to set values of.
 * @param n Maximum number of bytes to read.
 * @param path Path to read string from. Will read from stdin if NULL.
 */
static uint8_t* input_bytes(char* err, struct dynarr* bytes, const size_t n, const char* path)
{
    FILE* fp = NULL;
    uint8_t* result = NULL;

    // Allocate space to read file into buffer
    uint8_t* buffer = calloc(n, sizeof(uint8_t));
    if (!buffer) {
        if (err) sprintf(err, "Failed to allocate memory");
        goto exit;
    }

    // Open input file
    fp = !path ? stdin : fopen(path, "rb");
    if (!fp) {
        if (err) sprintf(err, "Failed to open input file: %s", !path ? "stdin" : path);
        goto exit;
    }

    // Read input file into buffer
    size_t buffer_n = fread(buffer, sizeof(uint8_t), n, fp);
    if (buffer_n > n) {
        if (err) sprintf(err, "Failed to read file");
        goto exit;
    }

    // Close input file
    fclose(fp);
    fp = NULL;

    // Copy file buffer to dynamic array
    result = dynarr_set(bytes, 0, buffer, buffer_n, sizeof(uint8_t));
    if (!result) {
        if (err) sprintf(err, "Failed to update dynamic array");
        goto exit;
    }

    exit:
    if (fp) fclose(fp);
    if (buffer) free(buffer);
    return result;
}

/**
 * Read string from input file.
 *
 * @param err Buffer to write error messages to.
 * @param str String to set values of.
 * @param len Maximum length of string to read, excluding null-terminator.
 * @param trim Whether to trim whitespace from read string.
 * @param path Path to read string from. Will read from stdin if NULL.
 */
static char* input_str(char* err, struct dynarr* str, const size_t len, bool trim, const char* path)
{
    FILE* fp = NULL;
    char* result = NULL;

    // Allocate space to read file into buffer
    char* buffer = calloc(STR_CHARS(len), sizeof(char));
    if (!buffer) {
        if (err) sprintf(err, "Failed to allocate memory");
        goto exit;
    }

    // Open input file
    fp = !path ? stdin : fopen(path, "rb");
    if (!fp) {
        if (err) sprintf(err, "Failed to open input file: %s", !path ? "stdin" : path);
        goto exit;
    }

    // Read input file into buffer
    size_t buffer_n = fread(buffer, sizeof(char), len, fp);
    if (buffer_n > len) {
        if (err) sprintf(err, "Failed to read file");
        goto exit;
    }

    // Close input file
    fclose(fp);
    fp = NULL;

    if (trim) {
        // Alloc space to trim whitespace from file buffer
        char* buffer_tr = calloc(STR_CHARS(buffer_n), sizeof(char));
        if (!buffer_tr) {
            if (err) sprintf(err, "Failed to allocate memory");
            goto exit;
        }

        // Trim whitespace from file buffer
        if (!str_trim(buffer_tr, buffer, buffer_n)) {
            if (err) sprintf(err, "Failed to trim string");
            free(buffer_tr);
            goto exit;
        }

        // Replace existing file buffer with trimmed file buffer
        free(buffer);
        buffer = buffer_tr;
        buffer_n = strlen(buffer_tr);
    }

    // Copy file buffer to dynamic array
    result = dynarr_set(str, 0, buffer, STR_CHARS(buffer_n), sizeof(char));
    if (!result) {
        if (err) sprintf(err, "Failed to update dynamic array");
        goto exit;
    }

    exit:
    if (fp) fclose(fp);
    if (buffer) free(buffer);
    return result;
}

/**
 * Write bytes to output path.
 *
 * @param err Buffer to write error messages to.
 * @param bytes Bytes to write to output.
 * @param path Path to write bytes to. Will write to stdout if NULL.
 */
static bool output_bytes(char* err, const struct dynarr bytes, const char* path)
{
    // Use stdout if no path given
    bool std = !path;

    FILE* fp = std ? stdout : fopen(path, "wb");
    if (!fp) {
        sprintf(err, "Failed to open output file: %s", std ? "stdout" : path);
        return false;
    }

    fwrite(bytes.vals, bytes.val_size, bytes.len, fp);

    fclose(fp);
    return true;
}

/**
 * Write string to output path.
 *
 * @param err Buffer to write error messages to.
 * @param str String to write to output.
 * @param path Path to write string to. Will write to stdout if NULL.
 */
static bool output_str(char* err, const char* str, const char* path)
{
    // Use stdout if no path given
    bool std = !path;

    FILE* fp = std ? stdout : fopen(path, "w");
    if (!fp) {
        sprintf(err, "Failed to open output file: %s", std ? "stdout" : path);
        return false;
    }

    fprintf(fp, "%s", str);

    // Append newline to output if using stdout and stdout is terminal (not piped)
    if (std && isatty(STDOUT_FILENO))
        fprintf(fp, EOL);

    fclose(fp);
    return true;
}

int main(int argc, char* argv[])
{
    bool success = false;
    char err[1024] = {0};

    enum operation opr;
    enum encoding enc = B28_E;
    char* in_path = NULL;
	char* out_path = NULL;

    struct dynarr bytes_da = {0};
    struct dynarr string_da = {0};
    struct dynarr addr_da = {0};

    if (argc < 2) {
        sprintf(err, "No operation given");
        goto exit;
    }

    int optc;
	extern char *optarg;
	extern int optind, optopt;

    // Parse operation arg
    if (!operation_parse(&opr, argv[1], strlen(argv[1]))) {
        sprintf(err, "Unknown operation given: %s", argv[1]);
        goto exit;
    }

    // Parse option args
    while ((optc = getopt(argc - 1, &argv[1], ":o:e:")) != -1) {
        size_t optarg_len = strlen(optarg);

        switch (optc) {
            case 'o':
				out_path = optarg;
				break;
			case 'e':
                // Parse encoding arg only if operation utilizes encoding, otherwise treat as unknown arg
                switch (opr) {
                    case PAGE_SEARCH_ENCODE_E:
                    case PAGE_GET_DECODE_E:
                    case ENCODE_E:
                    case DECODE_E:
                        if (!encoding_parse(&enc, optarg, optarg_len)) {
                            sprintf(err, "Unknown encoding given: %s", optarg);
                            goto exit;
                        }
                        break;
                    default:
                        sprintf(err, "Unknown option given: -%c", optopt);
                        goto exit;
                }
				break;
			case ':':
				sprintf(err, "No option argument given: -%c", optopt);
				goto exit;
			case '?':
				sprintf(err, "Unknown option given: -%c", optopt);
				goto exit;
		}
	}

    // Parse input file arg
    optind += 1;
    for (; optind < argc; optind++) {
		if (in_path) {
			sprintf(err, "Multiple input files given");
			goto exit;
		}

		in_path = argv[optind];
	}

    // Set input/output file args to NULL if given arg indicates to use stdin/stdout
    in_path = !in_path || strcmp(in_path, ARG_STDIN) == 0 ? NULL : in_path;
    out_path = !out_path || strcmp(out_path, ARG_STDOUT) == 0 ? NULL : out_path;

    switch (opr) {
        case PAGE_SEARCH_ENCODE_E:
            ;
            // Based on encoding to use, get max number of bytes that should be read from input file
            size_t bytes_n;
            if (!decode_len(&bytes_n, LOBI_PAGE_TEXT_LEN, enc))
                bytes_n = LOBI_PAGE_TEXT_LEN;

            // Read bytes from input file
            if (!input_bytes(err, &bytes_da, bytes_n, in_path))
                goto exit;

            // Encode given bytes to string
            if (!encode(err, &string_da, bytes_da.vals, bytes_da.len, enc))
                goto exit;

            // Find Library of Babel page whose content matches encoded string
            size_t string_len = DYNARR_STR_LEN(string_da);
            string_len = string_len > LOBI_PAGE_TEXT_LEN ? LOBI_PAGE_TEXT_LEN : string_len;
            if (lobi_page_search_exact(err, &addr_da, DYNARR_STR(string_da), string_len) != SUCCESS_E)
                goto exit;

            // Output Library of Babel page address
            if (!output_str(err, DYNARR_STR(addr_da), out_path))
                goto exit;

            success = true;
            break;

        case PAGE_GET_DECODE_E:
            // Read Library of Babel page address from input file
            if (!input_str(err, &addr_da, FP_READ_ADDR, true, in_path))
                goto exit;

            // Get content of Library of Babel page at given address
            if (lobi_page_get(err, &string_da, DYNARR_STR(addr_da), DYNARR_STR_LEN(addr_da)) != SUCCESS_E)
                goto exit;

            // Decode Library of Babel page content
            if (!decode(err, &bytes_da, string_da.vals, string_da.len, enc))
                goto exit;

            // Output decoded bytes
            if (!output_bytes(err, bytes_da, out_path))
                goto exit;

            success = true;
            break;

        case PAGE_SEARCH_E:
            // Read string from input file
            if (!input_str(err, &string_da, LOBI_PAGE_TEXT_LEN, false, in_path))
                goto exit;

            // Find Library of Babel page whose content matches given string
            if (lobi_page_search_exact(err, &addr_da, DYNARR_STR(string_da), DYNARR_STR_LEN(string_da)) != SUCCESS_E)
                goto exit;

            // Output Library of Babel page address
            if (!output_str(err, DYNARR_STR(addr_da), out_path))
                goto exit;

            success = true;
            break;

        case PAGE_GET_E:
            // Read Library of Babel page address from input file
            if (!input_str(err, &addr_da, FP_READ_ADDR, true, in_path))
                goto exit;

            // Get content of Library of Babel page at given address
            if (lobi_page_get(err, &string_da, DYNARR_STR(addr_da), DYNARR_STR_LEN(addr_da)) != SUCCESS_E)
                goto exit;

            // Output Library of Babel page content
            if (!output_str(err, DYNARR_STR(string_da), out_path))
                goto exit;

            success = true;
            break;

        case ENCODE_E:
            // Read bytes from input file
            if (!input_bytes(err, &bytes_da, FP_READ_MAX, in_path))
                goto exit;

            // Encode given bytes to string
            if (!encode(err, &string_da, bytes_da.vals, bytes_da.len, enc))
                goto exit;

            // Output encoded string
            if (!output_str(err, DYNARR_STR(string_da), out_path))
                goto exit;

            success = true;
            break;

        case DECODE_E:
            // Read encoded string from input file
            if (!input_str(err, &string_da, FP_READ_MAX, false, in_path))
                goto exit;

            // Decode bytes from given encoded string
            if (!decode(err, &bytes_da, DYNARR_STR(string_da), DYNARR_STR_LEN(string_da), enc))
                goto exit;

            // Output decoded bytes
            if (!output_bytes(err, bytes_da, out_path))
                goto exit;

            success = true;
            break;

        default:
            sprintf(err, "Unknown operation: %d", opr);
            goto exit;
    }

    exit:
    if (err[0] != '\0') {
        fprintf(stderr, "lobif: %s", err);

        // Append stderr is terminal (not piped)
        if (isatty(STDERR_FILENO))
            fprintf(stderr, "%s", EOL);
    }

    dynarr_empty(&addr_da);
    dynarr_empty(&string_da);
    dynarr_empty(&bytes_da);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
