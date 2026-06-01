#define _XOPEN_SOURCE 600

#include "encoding_bhex.h"
#include "encoding_b28.h"
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

enum operation {
    ENCODE_E,
    DECODE_E
};

enum encoding {
    BHEX_E,
    B28_E
};

static void print_err(const char* str, ...)
{
	va_list args;
	va_start(args, str);

	vfprintf(stderr, str, args);

	va_end(args);

	fprintf(stderr, EOL);
}

static bool operation_parse(enum operation* opr, const char* str, const size_t len)
{
    struct operation_str {
        enum operation opr;
        char str_lower[STR_CHARS(6)];
    };

    static struct operation_str opr_strs[] = {
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

static bool encoding_parse(enum encoding* enc, const char* str, const size_t len)
{
    struct encoding_str {
        enum encoding enc;
        char str_lower[STR_CHARS(8)];
    };

    static struct encoding_str enc_strs[] = {
        { .enc = B28_E, .str_lower = "b28" },
        { .enc = B28_E, .str_lower = "babel28" },
        { .enc = BHEX_E, .str_lower = "bhex" },
        { .enc = BHEX_E, .str_lower = "babelhex" },
        { .enc = BHEX_E, .str_lower = "b16" },
        { .enc = BHEX_E, .str_lower = "babel16" }
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

static void fp_close(FILE** fp)
{
    fclose(*fp);
    *fp = NULL;
}

static bool encode_fp(struct dynarr* str, FILE* fp, const size_t n, const enum encoding enc)
{
    #define ENCODE_ALLOC(str, n) dynarr_alloc(str, n, sizeof(char))

    if (!str || !fp)
        return false;

    bool success = false;

    uint8_t* buffer = calloc(n, sizeof(uint8_t));
    if (!buffer) {
        print_err("Failed to allocate memory");
        goto exit;
    }

    size_t buffer_n = fread(buffer, sizeof(buffer[0]), n, fp);

    switch (enc) {
        case BHEX_E:
            ENCODE_ALLOC(str, bhex_encode_len(buffer_n));
            if (!bhex_encode(str, buffer, buffer_n)) {
                print_err("Failed to encode input as BabelHex");
                goto exit;
            }
            break;

        case B28_E:
            ENCODE_ALLOC(str, b28_encode_len(buffer_n));
            if (!b28_encode(str, buffer, buffer_n)) {
                print_err("Failed to encode input as Babel28");
                goto exit;
            }
            break;

        default:
            print_err("Unknown encoding: %d", enc);
            goto exit;
    }

    success = true;

    exit:
    if (buffer) free(buffer);
    return success;
}

static bool decode_fp(struct dynarr* bytes, FILE* fp, const size_t len, const enum encoding enc)
{
    #define DECODE_ALLOC(bytes, n) dynarr_alloc(bytes, n, sizeof(uint8_t))

    if (!bytes || !fp)
        return false;

    bool success = false;
    char* str_fmt = NULL;
    char* str_buffer = NULL;
    size_t str_len;

    // Alloc space to read input string
    str_buffer = calloc(STR_CHARS(len), sizeof(char));
    if (!str_buffer) {
        print_err("Failed to allocate memory");
        goto exit;
    }

    // Read input string
    fread(str_buffer, sizeof(str_buffer[0]), len, fp);
    str_len = strlen(str_buffer);

    // Alloc space to formatted input string
    str_fmt = calloc(STR_CHARS(str_len), sizeof(char));
    if (!str_buffer) {
        print_err("Failed to allocate memory");
        goto exit;
    }

    // Format input string
    if (!str_trim(str_fmt, str_buffer, str_len)) {
        print_err("Failed to trim input string");
        goto exit;
    }

    str_len = strlen(str_fmt);

    free(str_buffer);
    str_buffer = NULL;

    size_t bytes_n;

    switch (enc) {
        case BHEX_E:
            if (!bhex_decode_len(&bytes_n, str_len)) {
                print_err("Invalid BabelHex input");
                goto exit;
            }

            DECODE_ALLOC(bytes, bytes_n);
            if (!bhex_decode(bytes, str_fmt, str_len)) {
                print_err("Failed to decode BabelHex input");
                goto exit;
            }

            break;

        case B28_E:
            if (!b28_decode_len(&bytes_n, str_len)) {
                print_err("Invalid Babel28 input");
                goto exit;
            }

            DECODE_ALLOC(bytes, bytes_n);
            if (!b28_decode(bytes, str_fmt, str_len)) {
                print_err("Failed to decode Babel28 input");
                goto exit;
            }

            break;

        default:
            print_err("Unknown encoding: %d", enc);
            goto exit;
    }

    success = true;

    exit:
    if (str_buffer) free(str_buffer);
    if (str_fmt) free(str_fmt);
    return success;
}

int main(int argc, char* argv[])
{
    char* in_path = NULL;
	char* out_path = NULL;
    enum operation opr;
    enum encoding enc = B28_E;

    if (argc < 2) {
        print_err("No operation given");
        return EXIT_FAILURE;
    }

    int optc;
	extern char *optarg;
	extern int optind, optopt;

    // Parse operation arg
    if (!operation_parse(&opr, argv[1], strlen(argv[1]))) {
        print_err("Unknown operation: %s", argv[1]);
        return EXIT_FAILURE;
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
                    print_err("Unknown encoding: %s", optarg);
                    return EXIT_FAILURE;
                }
				break;
			case ':':
				print_err("No option argument given: -%c", optopt);
				return EXIT_FAILURE;
			case '?':
				print_err("Unknown option: -%c", optopt);
				return EXIT_FAILURE;
		}
	}

    // Parse input file arg
    optind += 1;
    for (; optind < argc; optind++) {
		if (in_path) {
			print_err("Multiple input files given");
			return EXIT_FAILURE;
		}

		in_path = argv[optind];
	}

    bool in_stdin = !in_path || strcmp(in_path, ARG_STDIN) == 0;
    bool out_stdout = !out_path || strcmp(out_path, ARG_STDOUT) == 0;
    FILE* in_fp = NULL;
    FILE* out_fp = NULL;

    struct dynarr result_da = {0};

    bool success = false;

    in_fp = in_stdin ? stdin : fopen(in_path, "rb");
    if (!in_fp) {
        print_err("Failed to open input file: %s", in_stdin ? "stdin" : in_path);
        goto exit;
    }

    switch (opr) {
        case ENCODE_E:
            // Encode input file to string
            if (!encode_fp(&result_da, in_fp, 3200, enc))
                goto exit;

            // Open output file
            out_fp = out_stdout ? stdout : fopen(out_path, "w");
            if (!out_fp) {
                print_err("Failed to open output file: %s", out_stdout ? "stdout" : out_path);
                goto exit;
            }
                
            // Write encoded string to output file
            fprintf(out_fp, "%s", (char*)result_da.vals);

            // Append newline if output file is terminal (not piped)
            if (out_stdout && isatty(STDOUT_FILENO))
                fprintf(out_fp, EOL);

            fp_close(&out_fp);

            success = true;
            break;

        case DECODE_E:
            // Decode input file
            if (!decode_fp(&result_da, in_fp, 3200, enc))
                goto exit;

            fp_close(&in_fp);

            // Open output file
            out_fp = out_stdout ? stdout : fopen(out_path, "wb");
            if (!out_fp) {
                print_err("Failed to open output file: %s", out_stdout ? "stdout" : out_path);
                goto exit;
            }

            // Write decoded bytes to output file
            fwrite(result_da.vals, result_da.val_size, result_da.len, out_fp);
            fp_close(&out_fp);

            success = true;
            break;

        default:
            print_err("Unknown operation: %d", opr);
            goto exit;
    }

    exit:
    if (in_fp) fp_close(&in_fp);
    if (out_fp) fp_close(&out_fp);
    dynarr_empty(&result_da);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
