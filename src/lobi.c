#include "lobi.h"

#include "str.h"

#include <curl/curl.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define LOBI_PAGE_ADDR_WALL_MAX 4
#define LOBI_PAGE_ADDR_SHELF_MAX 5
#define LOBI_PAGE_ADDR_VOLUME_MAX 32
#define LOBI_PAGE_ADDR_PAGE_MAX 410
#define LOBI_PAGE_ADDR_STR_LEN_MIN 13 // "0-w1-s1-v01:1"

#define LOBI_PAGE_ADDR_PARTS 5

enum lobi_page_addr_parts {
	HEXAGON_E,
	WALL_E,
	SHELF_E,
	VOLUME_E,
	PAGE_E
};

static const char lobi_page_addr_part_suffixes[LOBI_PAGE_ADDR_PARTS][3] = {"-w", "-s", "-v", ":", ""};

/**
 * Parse and validate unsigned integer from string.
 */
static bool parse_uint(unsigned int* uint, const char* str, const size_t len, const unsigned int min, const unsigned int max)
{
	if (uint) *uint = 0;

	if (!str)
		return false;

	bool success = false;

	// Allocate space to ensure input is null-terminated
	char* str_buffer = calloc(STR_CHARS(len), sizeof(char));
	if (!str_buffer)
		goto exit;

	// Ensure input is null-terminated
	snprintf(str_buffer, STR_CHARS(len), "%s", str);

	errno = 0;
	char* end;
	long result = strtol(str_buffer, &end, 10);

	if (str_buffer == end || errno == ERANGE)
		goto exit;

	if (result < (long)min || result > (long)max)
		goto exit;

	success = true;
	if (uint) *uint = (unsigned int)result;

	exit:
	if (str_buffer) free(str_buffer);
	return success;
}

/**
 * Parse and validate unsigned char from string.
 */
static bool parse_uchar(unsigned char* uchar, const char* str, const size_t len, const unsigned char min, const unsigned char max)
{
	unsigned int result = 0;
	if (!parse_uint(&result, str, len, min, max))
		return false;

	if (uchar) *uchar = (unsigned char)result;
	return true;
}

/**
 * Parse and validate wall number of Library of Babel page address.
 */
static inline bool lobi_page_addr_wall_parse_str(unsigned char* wall, const char* str, const size_t len)
{
	return parse_uchar(wall, str, len, 1, LOBI_PAGE_ADDR_WALL_MAX);
}

/**
 * Parse and validate shelf number of Library of Babel page address.
 */
static inline bool lobi_page_addr_shelf_parse_str(unsigned char* shelf, const char* str, const size_t len)
{
	return parse_uchar(shelf, str, len, 1, LOBI_PAGE_ADDR_SHELF_MAX);
}

/**
 * Parse and validate volume number of Library of Babel page address.
 */
static inline bool lobi_page_addr_volume_parse_str(unsigned char* volume, const char* str, const size_t len)
{
	return parse_uchar(volume, str, len, 1, LOBI_PAGE_ADDR_VOLUME_MAX);
}

/**
 * Parse and validate page number of Library of Babel page address.
 */
static inline bool lobi_page_addr_page_parse_str(unsigned int* page, const char* str, const size_t len)
{
	return parse_uint(page, str, len, 1, LOBI_PAGE_ADDR_PAGE_MAX);
}

/**
 * Validate Library of Babel hexagon address.
 */
static bool lobi_hexagon_addr_validate(char* err, const char* hex, const size_t len)
{
	if (!hex)
		return false;

	if (len > LOBI_HEXAGON_ADDR_LEN) {
		if (err) sprintf(err, "Hexagon address exceeds maximum length (max %zu)", (size_t)LOBI_HEXAGON_ADDR_LEN);
		return false;
	}

	if (len < 1) {
		if (err) sprintf(err, "Hexagon address is below minimum length (min 1)");
		return false;
	}

	for (size_t ind = 0; ind < len && hex[ind]; ind++) {
		if (hex[ind] >= 'a' && hex[ind] <= 'z')
			continue;

		if (hex[ind] >= '0' && hex[ind] <= '9')
			continue;

		if (err) sprintf(err, "Hexagon address contains invalid character at position %zu", ind);
		return false;
	}

	return true;
}

enum lobi_result lobi_page_addr_parse_str(char* err, struct lobi_page_addr* addr, const char* str, const size_t len)
{
	if (!addr) {
		if (err) sprintf(err, "No struct to output page address given");
		return FAILURE_INPUT_E;
	}

	if (!str) {
		if (err) sprintf(err, "No input string given");
		return FAILURE_INPUT_E;
	}

	if (len > LOBI_PAGE_ADDR_STR_LEN) {
		if (err) sprintf(err, "Page address exceeds maximum length (max %zu)", (size_t)LOBI_PAGE_ADDR_STR_LEN);
		return FAILURE_INPUT_E;
	}

	if (len < LOBI_PAGE_ADDR_STR_LEN_MIN) {
		if (err) sprintf(err, "Page address is below minimum length (min %zu)", (size_t)LOBI_PAGE_ADDR_STR_LEN_MIN);
		return FAILURE_INPUT_E;
	}

	enum lobi_result result = FAILURE_GENERAL_E;

	// Allocate space to ensure input is null-terminated
	char* str_buffer = calloc(STR_CHARS(len), sizeof(char));
	if (!str_buffer) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto exit;
	}

	// Ensure input is null-terminated
	snprintf(str_buffer, STR_CHARS(len), "%s", str);

	// Loop through page address parts of formatted string
	char* addr_part_ptr = str_buffer;
	for (enum lobi_page_addr_parts page_addr_part = 0; page_addr_part < LOBI_PAGE_ADDR_PARTS; page_addr_part++) {;
		// Get suffix of page address part / prefix of next page address part
		const char* addr_part_suffix = lobi_page_addr_part_suffixes[page_addr_part];
		size_t addr_part_suffix_len = strlen(addr_part_suffix);

		// Use number of chars to page address suffix as length of page address part
		size_t addr_part_len = 0;
		if (addr_part_suffix_len > 0) {
			char* addr_part_suffix_ptr = strstr(addr_part_ptr, addr_part_suffix);
			if (addr_part_suffix_ptr)
				addr_part_len = addr_part_suffix_ptr - addr_part_ptr;
		} else {
			addr_part_len = strlen(addr_part_ptr);
		}

		// Parse + validate page address part
		switch (page_addr_part) {
			case HEXAGON_E:
				if (!lobi_hexagon_addr_validate(NULL, addr_part_ptr, addr_part_len)) {
					if (err) sprintf(err, "Hexagon address of page address is invalid");
					result = FAILURE_INPUT_E;
					goto exit;
				}

				addr->hexagon_len = addr_part_len;
				snprintf(addr->hexagon, STR_CHARS(addr_part_len), "%s", addr_part_ptr);
				break;

			case WALL_E:
				if (!lobi_page_addr_wall_parse_str(&addr->wall, addr_part_ptr, addr_part_len)) {
					if (err) sprintf(err, "Wall number of page address is invalid");
					result = FAILURE_INPUT_E;
					goto exit;
				}
				break;

			case SHELF_E:
				if (!lobi_page_addr_shelf_parse_str(&addr->shelf, addr_part_ptr, addr_part_len)) {
					if (err) sprintf(err, "Shelf number of page address is invalid");
					result = FAILURE_INPUT_E;
					goto exit;
				}
				break;

			case VOLUME_E:
				if (!lobi_page_addr_volume_parse_str(&addr->volume, addr_part_ptr, addr_part_len)) {
					if (err) sprintf(err, "Volume number of page address is invalid");
					result = FAILURE_INPUT_E;
					goto exit;
				}
				break;

			case PAGE_E:
				if (!lobi_page_addr_page_parse_str(&addr->page, addr_part_ptr, addr_part_len)) {
					if (err) sprintf(err, "Page number of page address is invalid");
					result = FAILURE_INPUT_E;
					goto exit;
				}
				break;

			default:
				if (err) sprintf(err, "Unknown page address part: %d", page_addr_part);
				goto exit;
		}

		// Advance pointer to next page address part
		addr_part_ptr = &addr_part_ptr[addr_part_len + addr_part_suffix_len];
	}

	result = SUCCESS_E;

	exit:
	if (str_buffer) free(str_buffer);
	return result;
}

/**
 * Output Library of Babel page address to string format.
 *
 * @param str Buffer to write formatted string to, including null-terminator.
 * @param addr Page address to format.
 * @returns Length of formatted string, excluding null-terminator.
 */
static size_t lobi_page_addr_sprint(char* str, const struct lobi_page_addr addr)
{
	if (!str)
		return 0;

	size_t str_len = 0;
	str_len += snprintf(&str[str_len], STR_CHARS(addr.hexagon_len), "%s", addr.hexagon);
	str_len += snprintf(
		&str[str_len],
		STR_CHARS(LOBI_PAGE_ADDR_STR_LEN - addr.hexagon_len),
		"%s%d%s%d%s%02d%s%d%s",
		lobi_page_addr_part_suffixes[HEXAGON_E],
		addr.wall,
		lobi_page_addr_part_suffixes[WALL_E],
		addr.shelf,
		lobi_page_addr_part_suffixes[SHELF_E],
		addr.volume,
		lobi_page_addr_part_suffixes[VOLUME_E],
		addr.page,
		lobi_page_addr_part_suffixes[PAGE_E]
	);

	return str_len;
}

enum lobi_result lobi_page_addr_fmt_str(char* err, struct dynarr* str, const struct lobi_page_addr addr)
{
	if (!str) {
		if (err) sprintf(err, "No struct to output string given");
		return FAILURE_INPUT_E;
	}

	enum lobi_result result = FAILURE_GENERAL_E;
	char* str_buffer = NULL;

	// Allocate space to print formatted page address
	str_buffer = calloc(STR_CHARS(LOBI_PAGE_ADDR_STR_LEN), sizeof(char));
	if (!str_buffer) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto exit;
	}

	// Print formatted page address
	size_t str_buffer_len = lobi_page_addr_sprint(str_buffer, addr);

	// Copy formatted page address to dynamic array
	if (!dynarr_set(str, 0, str_buffer, STR_CHARS(str_buffer_len), sizeof(char))) {
		if (err) sprintf(err, "Failed to update dynamic array");
		goto exit;
	}

	result = SUCCESS_E;

	exit:
	if (str_buffer) free(str_buffer);
	return result;
}

/**
 * Validate Library of Babel page content.
 *
 * @param err Buffer to write error messages to.
 * @param text Page content to validate.
 * @param len Length of page content, excluding null-terminator.
 */
static bool lobi_page_text_validate(char* err, const char* text, const size_t len)
{
	if (!text)
		return false;

	if (len > LOBI_PAGE_TEXT_LEN) {
		if (err) sprintf(err, "Page content exceeds maximum length (max %zu)", (size_t)LOBI_PAGE_TEXT_LEN);
		return false;
	}

	if (len < 1) {
		if (err) sprintf(err, "Page content is below minimum length (min 1)");
		return false;
	}

	for (size_t ind = 0; ind < len && text[ind]; ind++) {
		if (text[ind] >= 'a' && text[ind] <= 'z')
			continue;

		if (text[ind] == ' ' || text[ind] == ',' || text[ind] == '.')
			continue;

		if (err) sprintf(err, "Page content contains invalid character at position %zu", ind);
		return false;
	}

	return true;
}

/**
 * Write cURL response body to dynamic array.
 */
static size_t curl_write_dynarr(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	struct dynarr* da = userdata;
	size_t len_prev = da->len;

	if (!dynarr_set(da, da->len, ptr, nmemb, size))
		return 0;

	return da->len - len_prev;
}

/**
 * Initialize cURL handle for request to https://libraryofbabel.info.
 *
 * @param err Buffer to write error messages to.
 * @param curl cURL handle to initialize.
 * @param path URL path of request.
 * @param len Length of URL path, excluding null-terminator.
 * @param response_body Struct to output response body to.
 */
static CURL* curl_init_lobi(char* err, CURL** curl, const char* path, const size_t len, struct dynarr* response_body)
{
	#define URL_BASE "https://libraryofbabel.info"
	#define URL_BASE_LEN 27

	if (!curl)
		return NULL;

	*curl = curl_easy_init();
	if (!*curl) {
		if (err) sprintf(err, "Failed to initialize cURL handle");
		return NULL;
	}

	// Allocate space to build request URL
	char* url = calloc(STR_CHARS(URL_BASE_LEN + len), sizeof(char));
	if (!url) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto error;
	}

	// Build request URL
	strcat(url, URL_BASE);
	snprintf(&url[URL_BASE_LEN], STR_CHARS(len), "%s", path);

	curl_easy_setopt(*curl, CURLOPT_URL, url);
	curl_easy_setopt(*curl, CURLOPT_USERAGENT, "lobi/1.0");
	curl_easy_setopt(*curl, CURLOPT_FOLLOWLOCATION, 1);

	if (response_body) {
		curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, curl_write_dynarr);
		curl_easy_setopt(*curl, CURLOPT_WRITEDATA, response_body);
	}

	free(url);
	return *curl;

	error:
	if (url) free(url);
	if (*curl) curl_easy_cleanup(*curl);
	return NULL;
}

/**
 * Search Library of Babel for page with given content.
 *
 * @param err Buffer to write error messages to.
 * @param addr Struct to output page address to, including null-terminator.
 * @param text Page content to search for.
 * @param len Length of page content, excluding null-terminator.
 * @param result_title HTML to locate type of search result to return.
 * @returns SUCCESS_E if page with exact given content was located, otherwise error value.
 */
static enum lobi_result lobi_page_search(char* err, struct lobi_page_addr* addr, const char* text, const size_t len, const char* result_title)
{
	#define SEARCH_URL_PATH "/search.cgi"
	#define SEARCH_URL_PATH_LEN 11
	#define SEARCH_FORM_PREFIX "btnSubmit=Search&method=x&find="
	#define SEARCH_FORM_PREFIX_LEN 31

	#define SEARCH_HTML_POSTFORM_PREFIX "postform("
	#define SEARCH_HTML_POSTFORM_PREFIX_LEN 9
	#define SEARCH_HTML_POSTFORM_PARAM_PREFIX_LEN 2 // Can be either "('" or ",'"
	#define SEARCH_HTML_POSTFORM_PARAM_SUFFIX "'"
	#define SEARCH_HTML_POSTFORM_PARAM_SUFFIX_LEN 1

	if (!addr) {
		if (err) sprintf(err, "No struct to output page address given");
		return FAILURE_INPUT_E;
	}

	if (!lobi_page_text_validate(err, text, len))
		return FAILURE_INPUT_E;

	enum lobi_result result = FAILURE_GENERAL_E;
	char* text_escaped = NULL;
	char* search_form = NULL;

	// Init search cURL request
	CURL* search_curl = NULL;
	struct dynarr search_response_body = {0};
	if (!curl_init_lobi(err, &search_curl, SEARCH_URL_PATH, SEARCH_URL_PATH_LEN, &search_response_body))
		goto exit;

	// URI-escape text
	text_escaped = curl_easy_escape(search_curl, text, len);
	if (!text_escaped) {
		if (err) sprintf(err, "Failed to URI-escape search text");
		goto exit;
	}
	size_t text_escaped_len = strlen(text_escaped);

	// Allocate space to build HTTP form string
	search_form = calloc(STR_CHARS(SEARCH_FORM_PREFIX_LEN + text_escaped_len), sizeof(char));
	if (!search_form) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto exit;
	}

	// Build HTTP form string
	strcat(search_form, SEARCH_FORM_PREFIX);
	strncat(search_form, text_escaped, text_escaped_len);

	free(text_escaped);
	text_escaped = NULL;

	// Add HTTP form to search cURL request
	curl_easy_setopt(search_curl, CURLOPT_POSTFIELDSIZE, strlen(search_form));
	curl_easy_setopt(search_curl, CURLOPT_POSTFIELDS, search_form);

	// Perform search cURL request
	CURLcode search_curl_result = curl_easy_perform(search_curl);
	if (search_curl_result != CURLE_OK) {
		if (err) sprintf(err, "Failed HTTP request: %s", curl_easy_strerror(search_curl_result));
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	// HTTP response other than 200 OK is failure
	long search_response_code;
	curl_easy_getinfo(search_curl, CURLINFO_RESPONSE_CODE, &search_response_code);
	if (search_response_code != 200) {
		if (err) sprintf(err, "HTTP request returned response code: %ld", search_response_code);
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	// Cleanup completed search cURL request
	curl_easy_cleanup(search_curl);
	curl_global_cleanup();
	search_curl = NULL;
	free(search_form);
	search_form = NULL;

	// Ensure response body is null-terminated string
	if (*(char*)dynarr_get(search_response_body, search_response_body.len - 1) != '\0' && !dynarr_push(&search_response_body, &"\0", sizeof(char))) {
		if (err) sprintf(err, "Failed to update dynamic array");
		goto exit;
	}

	// Init HTML pointer at title of search result section
	char* html_ptr = strstr(search_response_body.vals, result_title);
	if (!html_ptr) {
		if (err) sprintf(err, "Failed to find expected HTML in response body");
		goto exit;
	}

	// Advance HTML pointer to postform() function call within search result section
	html_ptr = strstr(html_ptr, SEARCH_HTML_POSTFORM_PREFIX);
	if (!html_ptr) {
		if (err) sprintf(err, "Failed to find expected HTML in response body");
		goto exit;
	}

	// Advance HTML pointer to parameters within postform() function call
	html_ptr = &html_ptr[SEARCH_HTML_POSTFORM_PREFIX_LEN - 1]; // -1 to point to '('

	// Loop through postform() parameters to build page address
	for (enum lobi_page_addr_parts page_addr_part = 0; page_addr_part < LOBI_PAGE_ADDR_PARTS; page_addr_part++) {
		// Advance HTML pointer to start of parameter
		html_ptr = &html_ptr[SEARCH_HTML_POSTFORM_PARAM_PREFIX_LEN];

		// Use length of parameter as length of page address part
		size_t addr_part_len = strcspn(html_ptr, SEARCH_HTML_POSTFORM_PARAM_SUFFIX);

		// Parse + validate parameter as page address part
		switch (page_addr_part) {
			case HEXAGON_E:
				if (!lobi_hexagon_addr_validate(NULL, html_ptr, addr_part_len)) {
					if (err) sprintf(err, "Failed to parse hexagon address of page address");
					goto exit;
				}

				addr->hexagon_len = addr_part_len;
				snprintf(addr->hexagon, STR_CHARS(addr_part_len), "%s", html_ptr);
				break;

			case WALL_E:
				if (!lobi_page_addr_wall_parse_str(&addr->wall, html_ptr, addr_part_len)) {
					if (err) sprintf(err, "Failed to parse wall number of page address");
					goto exit;
				}
				break;

			case SHELF_E:
				if (!lobi_page_addr_shelf_parse_str(&addr->shelf, html_ptr, addr_part_len)) {
					if (err) sprintf(err, "Failed to parse shelf number of page address");
					goto exit;
				}
				break;

			case VOLUME_E:
				if (!lobi_page_addr_volume_parse_str(&addr->volume, html_ptr, addr_part_len)) {
					if (err) sprintf(err, "Failed to parse volume number of page address");
					goto exit;
				}
				break;

			case PAGE_E:
				if (!lobi_page_addr_page_parse_str(&addr->page, html_ptr, addr_part_len)) {
					if (err) sprintf(err, "Failed to parse page number of page address");
					goto exit;
				}
				break;

			default:
				if (err) sprintf(err, "Unknown page address part: %d", page_addr_part);
				goto exit;
		}

		// Advance pointer to end of parameter
		html_ptr = &html_ptr[addr_part_len + SEARCH_HTML_POSTFORM_PARAM_SUFFIX_LEN];
	}

	result = SUCCESS_E;

	exit:
	if (search_curl) {
		curl_easy_cleanup(search_curl);
		curl_global_cleanup();
	}
	if (search_form) free(search_form);
	if (text_escaped) curl_free(text_escaped);
	dynarr_empty(&search_response_body);
	return result;
}

enum lobi_result lobi_page_search_exact(char* err, struct lobi_page_addr* addr, const char* text, const size_t len)
{
	return lobi_page_search(err, addr, text, len, "<h3>exact match:</h3>");
}

enum lobi_result lobi_page_get(char* err, struct dynarr* text, const struct lobi_page_addr addr)
{
	#define PAGE_URL_PATH_BASE "/book.cgi?"
	#define PAGE_URL_PATH_BASE_LEN 10

	if (!text) {
		if (err) sprintf(err, "No dynamic array to output page content given");
		return FAILURE_INPUT_E;
	}

	enum lobi_result result = FAILURE_GENERAL_E;

	// Build page URL path
	char url_path[STR_CHARS(PAGE_URL_PATH_BASE_LEN + LOBI_PAGE_ADDR_STR_LEN)] = {0};
	strcat(url_path, PAGE_URL_PATH_BASE);
	lobi_page_addr_sprint(&url_path[PAGE_URL_PATH_BASE_LEN], addr);

	// Init page cURL request
	CURL* page_curl = NULL;
	struct dynarr page_response_body = {0};
	if (!curl_init_lobi(err, &page_curl, url_path, sizeof(url_path) / sizeof(url_path[0]), &page_response_body))
		goto exit;

	// Perform page cURL request
	CURLcode page_curl_result = curl_easy_perform(page_curl);
	if (page_curl_result != CURLE_OK) {
		if (err) sprintf(err, "Failed HTTP request: %s", curl_easy_strerror(page_curl_result));
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	// HTTP response other than 200 OK is failure
	long page_response_code;
	curl_easy_getinfo(page_curl, CURLINFO_RESPONSE_CODE, &page_response_code);
	if (page_response_code != 200) {
		if (err) sprintf(err, "HTTP request returned invalid response code: %ld", page_response_code);
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	// Cleanup completed page cURL request
	curl_easy_cleanup(page_curl);
	curl_global_cleanup();
	page_curl = NULL;

	// Ensure response body is null-terminated string
	if (*(char*)dynarr_get(page_response_body, page_response_body.len - 1) != '\0' && !dynarr_push(&page_response_body, &"\0", sizeof(char))) {
		if (err) sprintf(err, "Failed to update dynamic array");
		goto exit;
	}

	// Advance HTML pointer to first <PRE> tag - this element contains the page text
	char* html_ptr = strstr(page_response_body.vals, "<PRE");
	if (!html_ptr) {
		if (err) sprintf(err, "Failed to find expected HTML tag in HTTP response body");
		goto exit;
	}

	// Advance HTML pointer to end of opening <PRE> tag
	html_ptr = strchr(html_ptr, '>');
	if (!html_ptr) {
		if (err) sprintf(err, "Failed to find expected HTML tag in HTTP response body");
		goto exit;
	}

	// Advance HTML pointer to first char within <PRE> tag
	html_ptr = &html_ptr[1];

	// Use number of chars up to closing </PRE> tag as length <PRE> content length
	size_t pre_text_len = 0;
	char* pre_text_suffix = strstr(html_ptr, "</PRE");
	if (pre_text_suffix)
		pre_text_len = pre_text_suffix - html_ptr;

	// Write page text to dynamic array
	for (size_t pre_text_ind = 0; pre_text_ind < pre_text_len; pre_text_ind++) {
		char c = html_ptr[pre_text_ind];
		if (c == '\n')
			continue;

		if (!dynarr_push(text, &c, sizeof(c))) {
			if (err) sprintf(err, "Failed to update dynamic array");
			goto exit;
		}
	}

	// Ensure page content is null-terminated string
	if (*(char*)dynarr_get(*text, text->len - 1) != '\0' && !dynarr_push(text, &"\0", sizeof(char))) {
		if (err) sprintf(err, "Failed to update dynamic array");
		goto exit;
	}

	// Validate built page content
	if (!lobi_page_text_validate(err, (char*)text->vals, text->len - 1))
		goto exit;

	result = SUCCESS_E;

	exit:
	if (page_curl) {
		curl_easy_cleanup(page_curl);
		curl_global_cleanup();
	}
	dynarr_empty(&page_response_body);
	return result;
}
