#ifndef LOBI_H
#define LOBI_H

#include "dynarr.h"

#define LOBI_HEXAGON_ADDR_LEN 3260
#define LOBI_PAGE_ADDR_STR_LEN (LOBI_HEXAGON_ADDR_LEN + 14) // "{hexagon}-w4-s5-v32:410"
#define LOBI_PAGE_TEXT_LEN 3200

/**
 * Library of Babel page address.
 */
struct lobi_page_addr {
	char hexagon[LOBI_HEXAGON_ADDR_LEN + 1];
	size_t hexagon_len;
	unsigned char wall;
	unsigned char shelf;
	unsigned char volume;
	unsigned int page;
};

/**
 * Result of Library of Babel operation.
 */
enum lobi_result {
	SUCCESS_E,
	FAILURE_GENERAL_E,
	FAILURE_INPUT_E,
	FAILURE_NETWORK_E
};

/**
 * Parse Library of Babel page address from string format.
 *
 * @param err Buffer to write error messages to.
 * @param addr Struct to output page address to.
 * @param str String to parse page address from.
 * @param len Lengh of string to parse, excluding null-terminator.
 * @returns SUCCESS_E if page address was parsed successfully, otherwise error value.
 */
enum lobi_result lobi_page_addr_parse_str(char* err, struct lobi_page_addr* addr, const char* str, const size_t len);

/**
 * Output Library of Babel page address to string format.
 *
 * @param err Buffer to write error messages to.
 * @param str Struct to output formatted string to, including null-terminator.
 * @param addr Page address to format.
 * @returns SUCCESS_E if formatted page address was output successfully, otherwise error value.
 */
enum lobi_result lobi_page_addr_fmt_str(char* err, struct dynarr* str, const struct lobi_page_addr addr);

/**
 * Locate page in Library of Babel with exact given content.
 *
 * @param err Buffer to write error messages to.
 * @param addr Struct to output page address to.
 * @param text Content of page to locate.
 * @param len Length of page content to locate, excluding null-terminator.
 * @returns SUCCESS_E if page with exact given content was located, otherwise error value.
 */
enum lobi_result lobi_page_search_exact(char* err, struct lobi_page_addr* addr, const char* text, const size_t len);

/**
 * Get page content from Library of Babel.
 *
 * @param err Buffer to write error messages to.
 * @param text Struct to output page content to, including null-terminator.
 * @param addr Address of page to get.
 * @returns SUCCESS_E if page at given address was read, otherwise error value.
 */
enum lobi_result lobi_page_get(char* err, struct dynarr* text, const struct lobi_page_addr addr);

#endif
