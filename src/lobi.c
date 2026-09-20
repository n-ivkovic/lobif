#include "lobi.h"

#include "str.h"

#include <curl/curl.h>

#include <regex.h>
#include <stdlib.h>
#include <string.h>

static size_t curl_write_dynarr(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	struct dynarr* da = userdata;
	size_t len_prev = da->len;

	if (!dynarr_set(da, da->len, ptr, nmemb, size))
		return 0;

	return da->len - len_prev;
}

static CURL* curl_init_lobi(char* err, CURL** curl, const char* path, const size_t len, struct dynarr* da)
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

	char* url = calloc(URL_BASE_LEN + len, sizeof(char));
	if (!url) {
		if (err) sprintf(err, "Failed to allocate memory");
		goto error;
	}

	strcat(url, URL_BASE);
	strncat(url, path, len);

	curl_easy_setopt(*curl, CURLOPT_URL, url);
	curl_easy_setopt(*curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, curl_write_dynarr);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, da);

	free(url);
	return *curl;

	error:
	if (url) free(url);
	if (*curl) curl_easy_cleanup(*curl);
	return NULL;
}

static enum lobi_result lobi_page_addr_validate(char* err, const char addr[LOBI_PAGE_ADDR_LEN])
{
	regex_t re_page_location;

	if (regcomp(&re_page_location, "^[a-z0-9]{1,3260}-w[1-4]-s[1-5]-v((0[1-9])|([1-2][0-9])|(3[0-2])):0*(([1-3]?[0-9]{1,2})|(40[0-9])|(410))$", REG_EXTENDED | REG_NOSUB) != 0) {
		if (err) sprintf(err, "Failed to build regex");
		return FAILURE_GENERAL_E;
	}

	if (regexec(&re_page_location, addr, 0, NULL, 0) != 0) {
		if (err) sprintf(err, "Page address is invalid");
		regfree(&re_page_location);
		return FAILURE_INPUT_E;
	}

	regfree(&re_page_location);
	return SUCCESS_E;
}

enum lobi_result lobi_page_get(char* err, struct dynarr* text, const char addr[LOBI_PAGE_ADDR_LEN])
{
	#define PAGE_URL_PATH_BASE "/book.cgi?"
	#define PAGE_URL_PATH_BASE_LEN 10

	// Validate page address
	enum lobi_result addr_validate_result = lobi_page_addr_validate(err, addr);
	if (addr_validate_result != SUCCESS_E)
		return addr_validate_result;

	enum lobi_result result = FAILURE_GENERAL_E;

	// Build page URL path
	char url_path[PAGE_URL_PATH_BASE_LEN + LOBI_PAGE_ADDR_LEN] = {0};
	strcat(url_path, PAGE_URL_PATH_BASE);
	strncat(url_path, addr, LOBI_PAGE_ADDR_LEN);

	// Init page cURL request
	CURL* page_curl = NULL;
	struct dynarr page_da = {0};
	if (!curl_init_lobi(err, &page_curl, url_path, sizeof(url_path) / sizeof(url_path[0]), &page_da))
		goto exit;

	// Perform page cURL request
	if (curl_easy_perform(page_curl) != CURLE_OK) {
		if (err) sprintf(err, "Failed HTTP request");
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	// HTTP response other than 200 OK is failure
	long page_curl_response_code;
	curl_easy_getinfo(page_curl, CURLINFO_RESPONSE_CODE, &page_curl_response_code);
	if (page_curl_response_code != 200) {
		if (err) sprintf(err, "HTTP request returned invalid response code: %ld", page_curl_response_code);
		result = FAILURE_NETWORK_E;
		goto exit;
	}

	curl_easy_cleanup(page_curl);
	curl_global_cleanup();
	page_curl = NULL;

	// Find start of first opening <PRE> tag - this element contains the page text
	char* pre_text = strstr(page_da.vals, "<PRE");
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

	// No struct to output page content to - treat as success
	if (!text)
		goto success;

	for (size_t pre_text_ind = 0; pre_text_ind < page_text_len; pre_text_ind++) {
		char c = pre_text[pre_text_ind];
		if (c == '\n')
			continue;

		if (!dynarr_push(text, &c, sizeof(c))) {
			if (err) sprintf(err, "Failed push to dynamic array");
			goto exit;
		}
	}

	success:
	result = SUCCESS_E;

	exit:
	if (page_curl) {
		curl_easy_cleanup(page_curl);
		curl_global_cleanup();
	}
	dynarr_empty(&page_da);
	return result;
}
