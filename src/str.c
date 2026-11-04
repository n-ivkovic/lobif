#include "str.h"

#include <stdbool.h>
#include <string.h>

size_t str_copy(char* dst, const char* src, const size_t len)
{
	if (!dst || !src)
		return 0;

	size_t ind = 0;

	for (; ind < len && src[ind] != '\0'; ind++) {
		dst[ind] = src[ind];
	}

	dst[ind] = '\0';
	return ind;
}

size_t str_trim(char* dst, const char* src, const size_t len, int (*is)(const int))
{
	if (!dst || !src)
		return 0;

	size_t src_ind = 0, dst_ind = 0;

	for (; src_ind < len && src[src_ind] != '\0'; src_ind++) {
		if (src_ind == dst_ind && (is ? is((unsigned char)src[dst_ind]) : false))
			dst_ind++;
	}

	size_t dst_len = src_ind;

	while (dst_len > dst_ind && (is ? is((unsigned char)src[dst_len - 1]) : false)) {
		dst_len--;
	}

	return str_copy(dst, &src[dst_ind], dst_len - dst_ind);
}

int str_comp(const char* s1, const char* s2, const size_t len, int (*to)(const int))
{
	// If either input string is null, return comparison between null-ness of input strings
	if (!s1 || !s2)
		return !!s1 - !!s2;

	bool s1_ended = false, s2_ended = false;

	for (size_t ind = 0; ind < len && (!s1_ended || !s2_ended); ind++) {
		int c1 = s1_ended ? '\0' : s1[ind];
		int c2 = s2_ended ? '\0' : s2[ind];

		if (c1 == '\0')
			s1_ended = true;

		if (c2 == '\0')
			s2_ended = true;

		if (s1_ended && s2_ended)
			break;

		if (to) {
			c1 = to((unsigned char)c1);
			c2 = to((unsigned char)c2);
		}

		if (c1 != c2)
			return c1 - c2;
	}

	return 0;
}
