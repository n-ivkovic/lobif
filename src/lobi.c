#include "lobi.h"

#include "str.h"

#include <curl/curl.h>

#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

	char* url = calloc(STR_CHARS(URL_BASE_LEN + len), sizeof(char));
	if (!url) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto error;
	}

	strcat(url, URL_BASE);
	strncat(url, path, len);

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
 * Validate Library of Babel page address.
 *
 * @param err Buffer to write error messages to.
 * @param addr Page address to validate.
 * @param len Length of page address, excluding null-terminator.
 */
static enum lobi_result lobi_page_addr_validate(char* err, const char* addr, const size_t len)
{
	if (len > LOBI_PAGE_ADDR_LEN) {
		if (err) sprintf(err, "Page address exceeds maximum length (max %zu)", (size_t)LOBI_PAGE_ADDR_LEN);
		return FAILURE_INPUT_E;
	}

	enum lobi_result result = FAILURE_GENERAL_E;
	regex_t re_page_location;
	char* addr_buffer = NULL;

	// Build regex to validate page address format
	if (regcomp(&re_page_location, "^[a-z0-9]{1,3260}-w[1-4]-s[1-5]-v((0[1-9])|([1-2][0-9])|(3[0-2])):0*(([1-3]?[0-9]{1,2})|(40[0-9])|(410))$", REG_EXTENDED | REG_NOSUB) != 0) {
		if (err) sprintf(err, "Failed to build regex");
		goto exit;
	}

	// Copy page address to buffer - regex.h has no method with parameter for input string
	addr_buffer = calloc(STR_CHARS(len), sizeof(char));
	if (!addr_buffer) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto exit;
	}
	memcpy(addr_buffer, addr, len);

	// Validate page address format using regex
	if (regexec(&re_page_location, addr_buffer, 0, NULL, 0) != 0) {
		if (err) sprintf(err, "Page address is invalid");
		result = FAILURE_INPUT_E;
		goto exit;
	}

	result = SUCCESS_E;

	exit:
	if (addr_buffer) free(addr_buffer);
	regfree(&re_page_location);
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
		if (err) sprintf(err, "Page content below minimum length (min 1)");
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
 * Search Library of Babel for page with given content.
 *
 * @param err Buffer to write error messages to.
 * @param addr Struct to output page address to, including null-terminator.
 * @param text Page content to search for.
 * @param len Length of page content, excluding null-terminator.
 * @param result_title HTML to locate type of search result to return.
 * @returns SUCCESS_E if page with exact given content was located, otherwise error value.
 */
static enum lobi_result lobi_page_search(char* err, struct dynarr* addr, const char* text, const size_t len, const char* result_title)
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

	#define PAGE_ADDR_PARTS 5

	static const char page_addr_part_prefix[PAGE_ADDR_PARTS][3] = {"", "-w", "-s", "-v", ":"};

	if (!addr) {
		if (err) sprintf(err, "No dynamic array to output page address given");
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
	if (!curl_init_lobi(err, &search_curl, SEARCH_URL_PATH, SEARCH_URL_PATH_LEN / sizeof(char), &search_response_body))
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
	for (size_t addr_part_ind = 0; addr_part_ind < PAGE_ADDR_PARTS; addr_part_ind++) {
		// Advance HTML pointer to start of parameter
		html_ptr = &html_ptr[SEARCH_HTML_POSTFORM_PARAM_PREFIX_LEN];

		// Get length of parameter
		size_t html_postform_param_len = strcspn(html_ptr, SEARCH_HTML_POSTFORM_PARAM_SUFFIX);

		// Get prefix for address part
		const char* addr_part_prefix = page_addr_part_prefix[addr_part_ind];
		size_t addr_part_prefix_len = strlen(addr_part_prefix);

		// Append prefix for address part to result
		if (addr_part_prefix_len > 0) {
			if (!dynarr_set(addr, addr->len, addr_part_prefix, addr_part_prefix_len, sizeof(char))) {
				if (err) sprintf(err, "Failed to update dynamic array");
				goto exit;
			}
		}

		// Use parameter value as address part and append to result
		if (!dynarr_set(addr, addr->len, html_ptr, html_postform_param_len, sizeof(char))) {
			if (err) sprintf(err, "Failed to update dynamic array");
			goto exit;
		}

		// Advance pointer to end of parameter
		html_ptr = &html_ptr[html_postform_param_len + SEARCH_HTML_POSTFORM_PARAM_SUFFIX_LEN];
	}

	// Ensure page address is null-terminated string
	if (*(char*)dynarr_get(*addr, addr->len - 1) != '\0' && !dynarr_push(addr, &"\0", sizeof(char))) {
		if (err) sprintf(err, "Failed to update dynamic array");
		goto exit;
	}

	// Validate built page address
	if (lobi_page_addr_validate(err, (char*)addr->vals, addr->len - 1) != SUCCESS_E)
		goto exit;

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

enum lobi_result lobi_page_search_exact(char* err, struct dynarr* addr, const char* text, const size_t len)
{
	return lobi_page_search(err, addr, text, len, "<h3>exact match:</h3>");
}

enum lobi_result lobi_page_get(char* err, struct dynarr* text, const char* addr, const size_t len)
{
	#define PAGE_URL_PATH_BASE "/book.cgi?"
	#define PAGE_URL_PATH_BASE_LEN 10

	if (!text) {
		if (err) sprintf(err, "No dynamic array to output page content given");
		return FAILURE_INPUT_E;
	}

	// Validate page address
	enum lobi_result addr_validate_result = lobi_page_addr_validate(err, addr, len);
	if (addr_validate_result != SUCCESS_E)
		return addr_validate_result;

	enum lobi_result result = FAILURE_GENERAL_E;

	// Build page URL path
	char url_path[PAGE_URL_PATH_BASE_LEN + LOBI_PAGE_ADDR_LEN] = {0};
	strcat(url_path, PAGE_URL_PATH_BASE);
	strncat(url_path, addr, len);

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

	// Find start of first opening <PRE> tag - this element contains the page text
	char* pre_text = strstr(page_response_body.vals, "<PRE");
	if (!pre_text) {
		if (err) sprintf(err, "Failed to find expected HTML tag in HTTP response body");
		goto exit;
	}

	// Find end of opening <PRE> tag
	pre_text = strchr(pre_text, '>');
	if (!pre_text) {
		if (err) sprintf(err, "Failed to find expected HTML tag in HTTP response body");
		goto exit;
	}

	// Get page text within opening <PRE> tag
	pre_text = &pre_text[1];

	// Get length of page text (up to closing </PRE> tag)
	size_t page_text_len = strcspn(pre_text, "</");

	// Write page text to dynamic array
	for (size_t pre_text_ind = 0; pre_text_ind < page_text_len; pre_text_ind++) {
		char c = pre_text[pre_text_ind];
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
