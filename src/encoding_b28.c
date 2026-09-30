#include "encoding_b28.h"

#include "str.h"

#include <inttypes.h>
#include <math.h>
#include <stdint.h>

#define B28_BYTES_PER_BLOCK 3
#define B28_CHARS_PER_BLOCK 5
#define B28_CHARS ",.abcdefghijklmnopqrstuvwxyz"
#define B28_CHARS_LEN 28

/**
 * Calculate number of blocks required to encode a given number of bytes as a Babel28 string.
 *
 * @param blocks Number of blocks to encode.
 * @param padding Length of padding applied to last block to encode.
 * @param n Number of bytes to encode.
 */
static void b28_encode_calc_blocks(size_t* blocks, size_t* padding, const size_t n)
{
	if (!blocks || !padding)
		return;

	imaxdiv_t n_div_bytes = imaxdiv(n, B28_BYTES_PER_BLOCK);
	*blocks = (size_t)n_div_bytes.quot;
	*padding = n_div_bytes.rem > 0 ? B28_BYTES_PER_BLOCK - (size_t)n_div_bytes.rem : 0;

	if (*padding > 0)
		(*blocks)++;
}

bool b28_encode(struct dynarr* str, const void* bytes, const size_t n)
{
 	// Babel28 encoding based on ASCII85.
 	// Encode in blocks of 3 bytes as 5 chars.
 	// If the last block contains less than 3 bytes, a number of chars equal to the number of remainder bytes will be truncated.

	if (!str || !bytes)
		return false;

	bool success = false;

	size_t blocks, padding;
	b28_encode_calc_blocks(&blocks, &padding, n);

	// Encode one block at a time
	for (size_t block_ind = 0; block_ind < blocks; block_ind++) {
		unsigned long block_val = 0;
		char block_chars[B28_CHARS_PER_BLOCK] = {0};
		size_t block_chars_len = B28_CHARS_PER_BLOCK;

		// Subtract padding from number of chars in last block
		if (block_ind == blocks - 1)
			block_chars_len -= padding;

		// Build value equivalent to reading all bytes in block as a single big-endian value
		// Pad value with null bytes if not enough to fill block
		// E.g.
		// 0x48 0x65 0x6C -> 0x48656C
		// 0x6C 0x6F      -> 0x6C6F00
		for (size_t byte_ind = block_ind * B28_BYTES_PER_BLOCK; byte_ind < block_ind * B28_BYTES_PER_BLOCK + B28_BYTES_PER_BLOCK; byte_ind++) {
			block_val <<= 8;

			if (byte_ind < n)
				block_val += ((uint8_t*)bytes)[byte_ind];
		}

		// Encode block value to string
		// E.g.
		// 0x48656C -> "fsbsk"
		// 0x6C6F00 -> "jnsc"
		for (int block_char_ind = B28_CHARS_PER_BLOCK - 1; block_char_ind >= 0; block_char_ind--) {
			if ((size_t)block_char_ind < block_chars_len)
				block_chars[block_char_ind] = B28_CHARS[block_val % B28_CHARS_LEN];

			block_val /= B28_CHARS_LEN;
		}

		// Write block chars to result
		if (!dynarr_set(str, str->len, &block_chars, block_chars_len, sizeof(block_chars[0])))
			goto exit;
	}

	success = true;

	exit:

	// Append null-terminator
	if (!dynarr_push(str, &"\0", sizeof(char)))
		success = false;

	return success;
}

size_t b28_encode_chars(const size_t n)
{
	size_t blocks, padding;
	b28_encode_calc_blocks(&blocks, &padding, n);

	return STR_CHARS((blocks * B28_CHARS_PER_BLOCK) - padding);
}

/**
 * Calculate number of blocks to decode from a Babel28 string of a given length.
 *
 * @param blocks Number of blocks to decode.
 * @param padding Length of padding applied to last block to decode.
 * @param len Length of Babel28 string to decode, excluding null-terminator.
 * @returns Whether Babel28 string of given length can be decoded.
 */
static bool b28_decode_calc_blocks(size_t* blocks, size_t* padding, const size_t len)
{
	if (!blocks || !padding)
		return false;

	imaxdiv_t len_div_chars = imaxdiv(len, B28_CHARS_PER_BLOCK);
	*blocks = (size_t)len_div_chars.quot;
	*padding = len_div_chars.rem > 0 ? B28_CHARS_PER_BLOCK - (size_t)len_div_chars.rem : 0;

	if (*padding >= B28_BYTES_PER_BLOCK) {
		*blocks = 0;
		*padding = 0;
		return false;
	}

	if (*padding > 0)
		(*blocks)++;

	return true;
}

/**
 * Get index of Babel28 char.
 *
 * @param ch Char to get index of.
 * @returns Index of Babel28 char. -1 if invalid.
 */
static int b28_char_ind(const char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (int)ch - 'a' + 2;

	switch (ch) {
		case ',':
			return 0;
		case '.':
			return 1;
		default:
			return -1;
	}
}

bool b28_decode(struct dynarr* bytes, const char* str, const size_t len)
{
	if (!bytes || !str)
		return false;

	size_t blocks, padding;
	if (!b28_decode_calc_blocks(&blocks, &padding, len))
		return false;

	bool str_ended = false;

	// Decode one block at a time
	for (size_t block_ind = 0; block_ind < blocks && !str_ended; block_ind++) {
		unsigned long block_val = 0;
		uint8_t block_bytes[B28_BYTES_PER_BLOCK] = {0};
		size_t block_bytes_len = B28_BYTES_PER_BLOCK;

		// Subtract padding from number of bytes in last block
		if (block_ind == blocks - 1)
			block_bytes_len -= padding;

		// Convert encoded chars to value equivalent to all block bytes as a single big-endian value.
		// If the last block does not have enough chars, this indicates the block has padding applied.
		// Convert last block with missing chars as if missing chars are 'z'
		// E.g.
		// "fsbsk" -> 0x48656C
		// "jnsc" ("jnscz") -> 0x6C6F0B
		for (size_t str_ind = block_ind * B28_CHARS_PER_BLOCK, block_char_ind = 0; block_char_ind < B28_CHARS_PER_BLOCK; str_ind++, block_char_ind++) {
			if (str_ind >= len || str[str_ind] == '\0')
				str_ended = true;

			int char_ind = !str_ended ? b28_char_ind(str[str_ind]) : B28_CHARS_LEN - 1;
			if (char_ind < 0)
				return false;

			block_val += char_ind * (unsigned long)pow(B28_CHARS_LEN, B28_CHARS_PER_BLOCK - block_char_ind - 1);
		}

		// Split value into its constituent bytes
		// E.g.
		// 0x48656C -> 0x48 0x65 0x6C
		// 0x6C6F0B -> 0x6C 0x6F
		for (int block_byte_ind = B28_BYTES_PER_BLOCK - 1; block_byte_ind >= 0; block_byte_ind--) {
			if ((size_t)block_byte_ind < block_bytes_len)
				block_bytes[block_byte_ind] = block_val & 0xFF;

			block_val >>= 8;
		}

		// Write block bytes to result
		if (!dynarr_set(bytes, bytes->len, &block_bytes, block_bytes_len, sizeof(block_bytes[0])))
			return false;
	}

	return true;
}

bool b28_decode_len(size_t* n, const size_t len)
{
	size_t blocks, padding;
	if (!b28_decode_calc_blocks(&blocks, &padding, len))
		return false;

	if (n)
		*n = (blocks * B28_BYTES_PER_BLOCK) - padding;

	return true;
}
