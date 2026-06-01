#ifndef ENCODING_BHEX_H
#define ENCODING_BHEX_H

#include "dynarr.h"

#include <stdbool.h>

/**
 * Encode bytes as BabelHex string.
 *
 * @param str Struct to write encoded string to, including null-terminator.
 * @param bytes Array of bytes to encode.
 * @param n Number of bytes to encode.
 * @returns Whether bytes were successfully encoded.
 */
bool bhex_encode(struct dynarr* str, const void* bytes, const size_t n);

/**
 * Get number of chars required to encode a given number of bytes as a BabelHex string.
 *
 * @param n Number of bytes to encode.
 * @returns Number of chars required to encode as BabelHex string, including null-terminator.
 */
size_t bhex_encode_len(const size_t n);

/**
 * Decode bytes from BabelHex string.
 *
 * @param bytes Struct to write decoded bytes to.
 * @param str BabelHex string to decode.
 * @param len Length of BabelHex string to decode, excluding null-terminator.
 * @returns Whether BabelHex string was successfully decoded.
 */
bool bhex_decode(struct dynarr* bytes, const char* str, const size_t len);

/**
 * Get number of bytes to decode from a BabelHex string of a given length.
 *
 * @param n Result to write number of bytes to decode from BabelHex string of given length.
 * @param len Length of BabelHex string to decode, excluding null-terminator.
 * @returns Whether BabelHex string of given length can be decoded.
 */
bool bhex_decode_len(size_t* n, const size_t len);

#endif
