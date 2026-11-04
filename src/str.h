#ifndef STR_H
#define STR_H

#include <stddef.h>

#define STR_CHARS(len) ((len) + 1)

/**
 * Copy string.
 *
 * @param dst String to write output, including null-terminator.
 * @param src Input string.
 * @param len Max length of string to read from input and write to output, excluding null-terminator.
 * @returns Length of string written to output, excluding null-terminator.
 */
size_t str_copy(char* dst, const char* src, const size_t len);

/**
 * Remove leading and trailing chars in string matching a given function.
 *
 * @param dst String to write output, including null-terminator.
 * @param src Input string.
 * @param len Max length of string to read from input and write to output, excluding null-terminator.
 * @param is Function to test whether a char should be removed. If NULL, no chars will be removed.
 * @returns Length of string written to output, excluding null-terminator.
 */
size_t str_trim(char* dst, const char* src, const size_t len, int (*is)(const int));

/**
 * Compare two strings using a given function.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param len Max length of strings to compare, excluding null-terminator.
 * @param to Function used to convert each char before comparing. If NULL, no char conversions will be performed.
 * @returns Whether s1 is equal (0), greater than (>0), or less than (<0) s2.
 */
int str_comp(const char* s1, const char* s2, const size_t len, int (*to)(const int));

#endif
