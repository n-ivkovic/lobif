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

#define OPERATION_LEN 2
#define ENCODING_LEN 2

#define ARG_STDIN "-"
#define ARG_STDOUT "-"

/**
 * CLI operation.
 */
enum operation {
    LOB_PAGE_ENCODE_E,
    LOB_PAGE_DECODE_E,
    LOB_PAGE_GET_E,
    LOB_PAGE_SEARCH_EXACT_E,
    INPUT_ENCODE_E,
    INPUT_DECODE_E
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
        char str_lower[STR_CHARS(6)];
    };

    static struct operation_str opr_strs[] = {
        { .opr = LOB_PAGE_ENCODE_E, .str_lower = "write" },
        { .opr = LOB_PAGE_DECODE_E, .str_lower = "read" },
        { .opr = LOB_PAGE_GET_E, .str_lower = "get" },
        { .opr = LOB_PAGE_SEARCH_EXACT_E, .str_lower = "search" },
        { .opr = INPUT_ENCODE_E, .str_lower = "encode" },
        { .opr = INPUT_DECODE_E, .str_lower = "decode" }
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

    static struct encoding_str enc_strs[] = {
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
 * @param len Number of chars in encoded string to decode, excluding null-terminator.
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
    size_t str_len;

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

    str_len = strlen(str_tr);
    str_len = str_len > len ? len : str_len;

    switch (enc) {
        case BHEX_E:
            if (!bhex_decode_len(&bytes_n, len)) {
                if (err) sprintf(err, "Invalid BabelHex input given");
                goto exit;
            }

            if (bytes_unalloc)
                DECODE_ALLOC(bytes, bytes_n);

            if (!bhex_decode(bytes, str_tr, len)) {
                if (err) sprintf(err, "Failed to decode BabelHex input");
                goto exit;
            }

            success = true;
            goto exit;

        case B28_E:
            if (!b28_decode_len(&bytes_n, len)) {
                if (err) sprintf(err, "Invalid Babel28 input given");
                goto exit;
            }

            if (bytes_unalloc)
                DECODE_ALLOC(bytes, bytes_n);

            if (!b28_decode(bytes, str_tr, len)) {
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
 * Read bytes from file and encode as string.
 *
 * @param err Buffer to write error messages to.
 * @param str Struct to output encoded string to, including null-terminator.
 * @param fp File containing bytes to encode.
 * @param n Number of bytes to encode.
 * @param enc Encoding method.
 * @returns Whether bytes were read and encoded successfully.
 */
static bool encode_fp(char* err, struct dynarr* str, FILE* fp, const size_t n, const enum encoding enc)
{
    if (!fp) {
        if (err) sprintf(err, "No file input given");
        return false;
    }

    bool success = false;

    // Alloc space to read input bytes
    uint8_t* buffer = calloc(n, sizeof(uint8_t));
    if (!buffer) {
        if (err) sprintf(err, "Failed to allocate memory");
        goto exit;
    }

    // Read input bytes
    size_t buffer_n = fread(buffer, sizeof(buffer[0]), n, fp);

    // Encode bytes
    if (!encode(err, str, buffer, buffer_n, enc))
        goto exit;

    success = true;

    exit:
    if (buffer) free(buffer);
    return success;
}

/**
 * Read encoded string from file and decode bytes.
 *
 * @param err Buffer to write error messages to.
 * @param bytes Struct to output decoded bytes to.
 * @param fp File containing encoded string to decode.
 * @param len Length of encoded string to decode, excluding null-terminator.
 * @param enc Encoding method.
 * @returns Whether encoded string was read and decoded successfully.
 */
static bool decode_fp(char* err, struct dynarr* bytes, FILE* fp, const size_t len, const enum encoding enc)
{
    if (!bytes || !fp)
        return false;

    bool success = false;
    char* str = NULL;
    size_t str_len;

    // Alloc space to read input string
    str = calloc(STR_CHARS(len), sizeof(char));
    if (!str) {
        if (err) sprintf(err, "Failed to allocate memory");
        goto exit;
    }

    // Read input string
    fread(str, sizeof(str[0]), len, fp);
    str_len = strlen(str);
    str_len = str_len > len ? len : str_len;

    if (!decode(err, bytes, str, str_len, enc))
        goto exit;

    success = true;

    exit:
    if (str) free(str);
    return success;
}

static enum lobi_result page_search_exact_fp(char* err, struct dynarr* addr, FILE* fp)
{
    if (!fp) {
        if (err) sprintf(err, "No file input given");
        return FAILURE_INPUT_E;
    }

    // Read input string
    char str_buffer[STR_CHARS(LOBI_PAGE_TEXT_LEN)] = {0};
    fread(str_buffer, sizeof(str_buffer[0]), LOBI_PAGE_TEXT_LEN, fp);

    // Return result
    return lobi_page_search_exact(err, addr, str_buffer, strlen(str_buffer));
}

/**
 * Find Library of Babel page at address given in file.
 *
 * @param err Buffer to write error messages to.
 * @param text Struct to output page content to, including null-terminator.
 * @param fp File containing address of page to read.
 * @param len Length of page address.
 * @return Whether page at address was read successfully.
 */
static enum lobi_result page_get_fp(char* err, struct dynarr* text, FILE* fp)
{
    #define LOBI_PAGE_ADDR_BUFFER_LEN (LOBI_PAGE_ADDR_LEN + 16)

    if (!fp) {
        if (err) sprintf(err, "No file input given");
        return FAILURE_INPUT_E;
    }

    // Read input string
    char str_buffer[STR_CHARS(LOBI_PAGE_ADDR_BUFFER_LEN)] = {0};
    fread(str_buffer, sizeof(str_buffer[0]), LOBI_PAGE_ADDR_BUFFER_LEN, fp);

    // Trim whitespace from input string
    char str_tr[STR_CHARS(LOBI_PAGE_ADDR_BUFFER_LEN)] = {0};
    if (!str_trim(str_tr, str_buffer, strlen(str_buffer))) {
        if (err) sprintf(err, "Failed to trim string");
        return FAILURE_GENERAL_E;
    }

    // Return result
    return lobi_page_get(err, text, str_tr, strlen(str_tr));
}

/**
 * Output bytes result.
 *
 * @param err Buffer to write error messages to.
 * @param bytes Bytes to output.
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
 * Output string result.
 *
 * @param err Buffer to write error messages to.
 * @param bytes String to output.
 * @param path Path to write bytes to. Will write to stdout if NULL.
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

    FILE* in_fp = NULL;

    struct dynarr string_da = {0};
    struct dynarr bytes_da = {0};

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
				if (!encoding_parse(&enc, optarg, optarg_len)) {
                    sprintf(err, "Unknown encoding given: %s", optarg);
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

    bool in_stdin = !in_path || strcmp(in_path, ARG_STDIN) == 0;
    bool out_stdout = !out_path || strcmp(out_path, ARG_STDOUT) == 0;

    in_fp = in_stdin ? stdin : fopen(in_path, "rb");
    if (!in_fp) {
        sprintf(err, "Failed to open input file: %s", in_stdin ? "stdin" : in_path);
        goto exit;
    }

    switch (opr) {
        case LOB_PAGE_DECODE_E:
            // Read Library of Babel page at address in given file
            if (page_get_fp(err, &string_da, in_fp) != SUCCESS_E)
                goto exit;

            fclose(in_fp);
            in_fp = NULL;

            // Decode page content
            if (!decode(err, &bytes_da, string_da.vals, string_da.len, enc))
                goto exit;

            // Output decoded bytes
            if (!output_bytes(err, bytes_da, out_stdout ? NULL : out_path))
                goto exit;

            success = true;
            break;

        case LOB_PAGE_SEARCH_EXACT_E:
            // Find Library of Babel page matching content in given file
            if (page_search_exact_fp(err, &string_da, in_fp) != SUCCESS_E)
                goto exit;

            fclose(in_fp);
            in_fp = NULL;

            // Output page address
            if (!output_str(err, (char*)string_da.vals, out_stdout ? NULL : out_path))
                goto exit;

            success = true;
            break;

        case LOB_PAGE_GET_E:
            // Read Library of Babel page at address in given file
            if (page_get_fp(err, &string_da, in_fp) != SUCCESS_E)
                goto exit;

            fclose(in_fp);
            in_fp = NULL;

            // Output page content
            if (!output_str(err, (char*)string_da.vals, out_stdout ? NULL : out_path))
                goto exit;

            success = true;
            break;

        case INPUT_ENCODE_E:
            // Encode input file to string
            if (!encode_fp(err, &string_da, in_fp, LOBI_PAGE_TEXT_LEN, enc))
                goto exit;

            fclose(in_fp);
            in_fp = NULL;

            // Output encoded string
            if (!output_str(err, (char*)string_da.vals, out_stdout ? NULL : out_path))
                goto exit;

            success = true;
            break;

        case INPUT_DECODE_E:
            // Decode input file
            if (!decode_fp(err, &bytes_da, in_fp, LOBI_PAGE_TEXT_LEN, enc))
                goto exit;

            fclose(in_fp);
            in_fp = NULL;

            // Output decoded bytes
            if (!output_bytes(err, bytes_da, out_stdout ? NULL : out_path))
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

    if (in_fp) fclose(in_fp);
    dynarr_empty(&bytes_da);
    dynarr_empty(&string_da);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
