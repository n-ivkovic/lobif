#include "encoding_bhex.h"

#include "str.h"

#include <inttypes.h>
#include <stdint.h>

#define BHEX_CHAR_OFFSET 'a'
#define BHEX_CHARS_PER_BYTE 2
#define BHEX_BITS_PER_CHAR 4 // (8 / BHEX_CHARS_PER_BYTE)

bool bhex_encode(struct dynarr* str, const void* bytes, const size_t n)
{
	if (!str || !bytes)
		return false;

	static const char null = '\0';

	bool success = false;

	// Encode one byte at a time
	for (size_t byte_ind = 0; byte_ind < n; byte_ind++) {
		char chars[BHEX_CHARS_PER_BYTE] = {
			BHEX_CHAR_OFFSET + (((uint8_t*)bytes)[byte_ind] >> BHEX_BITS_PER_CHAR),
			BHEX_CHAR_OFFSET + (((uint8_t*)bytes)[byte_ind] & (0xFF >> BHEX_BITS_PER_CHAR))
		};

		if (!dynarr_set(str, str->len, &chars, BHEX_CHARS_PER_BYTE, sizeof(chars[0])))
			goto exit;
	}

	success = true;

	exit:
	
	// Append null-terminator
	if (!dynarr_push(str, &null, sizeof(null)))
		success = false;

	return success;
}

size_t bhex_encode_len(const size_t n)
{
	return STR_CHARS(n * BHEX_CHARS_PER_BYTE);
}

bool bhex_decode(struct dynarr* bytes, const char* str, const size_t len)
{
	if (!bytes || !str)
		return false;

	size_t bytes_n;
	if (!bhex_decode_len(&bytes_n, len))
		return false;

	// Decode one byte at a time
	for (size_t str_ind = 0, byte_ind = 0; byte_ind < bytes_n && str[str_ind] != '\0'; str_ind += BHEX_CHARS_PER_BYTE, byte_ind++) {
		uint8_t byte = 0;

		// Build byte value from each char
		for (size_t byte_str_ind = 0; byte_str_ind < BHEX_CHARS_PER_BYTE; byte_str_ind++) {
			char ch = str[str_ind + byte_str_ind];
			if (ch < BHEX_CHAR_OFFSET || ch > (BHEX_CHAR_OFFSET + (0xFF >> BHEX_BITS_PER_CHAR)))
				return false;

			byte += (ch - BHEX_CHAR_OFFSET) << (BHEX_BITS_PER_CHAR * (BHEX_CHARS_PER_BYTE - byte_str_ind - 1));
		}

		if (!dynarr_push(bytes, &byte, sizeof(byte)))
			return false;
	}

	return true;
}

bool bhex_decode_len(size_t* n, const size_t len)
{
	imaxdiv_t len_div_chars = imaxdiv(len, BHEX_CHARS_PER_BYTE);
	if (len_div_chars.rem > 0)
		return false;

	if (n)
		*n = len_div_chars.quot;

	return true;
}
