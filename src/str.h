#ifndef STR_H
#define STR_H

#include <stddef.h>

#define STR_CHARS(len) ((len) + 1)
#define STR_SIZE(len) ((len) * sizeof(char))

/**
 * Copy string.
 *
 * @param dst Output string.
 * @param src Input string.
 * @param len Max number of chars to copy.
 * @returns Pointer to output string. NULL if error.
 */
char* str_copy(char* dst, const char* src, const size_t len);

/**
 * Remove leading and trailing whitespace chars from string.
 *
 * @param dst Output string.
 * @param src Input string.
 * @param len Max number of chars to look at.
 * @returns Pointer to output string. NULL if error.
 */
char* str_trim(char* dst, const char* src, const size_t len);

/**
 * Compare two strings using the given function.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param len Max number of chars to look at.
 * @param to Function used to convert each char compared.
 * @returns Whether s1 is equal (0), greater than (>0), or less than (<0) s2.
 */
int str_comp(const char* s1, const char* s2, const size_t len, int (*to)(const int));

#endif
