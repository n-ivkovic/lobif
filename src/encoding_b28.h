#ifndef ENCODING_B28_H
#define ENCODING_B28_H

#include "dynarr.h"

#include <stdbool.h>

/**
 * Encode bytes as Babel28 string.
 *
 * @param str Struct to write encoded string to, including null-terminator.
 * @param bytes Array of bytes to encode.
 * @param n Number of bytes to encode.
 * @returns Whether bytes were successfully encoded.
 */
bool b28_encode(struct dynarr* str, const void* bytes, const size_t n);

/**
 * Get number of chars required to encode a given number of bytes as a Babel28 string.
 *
 * @param n Number of bytes to encode.
 * @returns Number of chars required to encode as Babel28 string, including null-terminator. 
 */
size_t b28_encode_len(const size_t n);

/**
 * Decode bytes from Babel28 string.
 *
 * @param bytes Struct to write decoded bytes to.
 * @param str Babel28 string to decode.
 * @param len Length of Babel28 string to decode, excluding null-terminator.
 * @returns Whether Babel28 string was successfully decoded.
 */
bool b28_decode(struct dynarr* bytes, const char* str, const size_t len);

/**
 * Get number of bytes to decode from a Babel28 string of a given length.
 *
 * @param n Result to write number of bytes to decode from Babel28 string of given length.
 * @param len Length of Babel28 string to decode, excluding null-terminator.
 * @returns Whether Babel28 string of given length can be decoded.
 */
bool b28_decode_len(size_t* n, const size_t len);

#endif
