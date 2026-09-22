#ifndef LOBI_H
#define LOBI_H

#include "dynarr.h"

#define LOBI_HEXAGON_ADDR_LEN 3260
#define LOBI_PAGE_ADDR_LEN (LOBI_HEXAGON_ADDR_LEN + 14) // "{hexagon}-w4-s5-v32:410"
#define LOBI_PAGE_TEXT_LEN 3200

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
 * Locate page in Library of Babel with exact given content.
 *
 * @param err Buffer to write error messages to.
 * @param addr Struct to output page address to, including null-terminator.
 * @param text Content of page to locate.
 * @param len Length of page content to locate, excluding null-terminator.
 * @returns SUCCESS_E if page with exact given content was located, otherwise error value.
 */
enum lobi_result lobi_page_search_exact(char* err, struct dynarr* addr, const char* text, const size_t len);

/**
 * Get page content from Library of Babel.
 *
 * @param err Buffer to write error messages to.
 * @param text Struct to output page content to, including null-terminator.
 * @param addr Address of page to get.
 * @param len Length of page address to get, excluding null-terminator.
 * @returns SUCCESS_E if page at given address was read, otherwise error value.
 */
enum lobi_result lobi_page_get(char* err, struct dynarr* text, const char* addr, const size_t len);

#endif