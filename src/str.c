#include "str.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

char* str_copy(char* dst, const char* src, const size_t len)
{
	if (!dst || !src)
		return NULL;

	*dst = '\0';
	return strncat(dst, src, len);
}

char* str_trim(char* dst, const char* src, const size_t len)
{
	if (!dst || !src)
		return NULL;

	size_t src_ind = 0, dst_ind = 0;

	for (; src_ind < len && src[src_ind] != '\0'; src_ind++) {
		if (src_ind == dst_ind && isspace(src[dst_ind]))
			dst_ind++;
	}

	size_t dst_len = src_ind;

	while (dst_len > dst_ind && isspace(src[dst_len - 1])) {
		dst_len--;
	}

	char* result = str_copy(dst, &src[dst_ind], dst_len - dst_ind);
	return result;
}

int str_comp(const char* s1, const char* s2, const size_t len, int (*to)(const int))
{
	bool s1_ended = false, s2_ended = false;

	for (size_t ind = 0; ind < len && !s1_ended && !s2_ended; ind++) {
		char c1 = s1_ended ? '\0' : s1[ind];
		char c2 = s2_ended ? '\0' : s2[ind];

		// Compare chars - return if inequal
		int result = to(c1) - to(c2);
		if (result != 0)
			return result;

		// End of s1 reached
		if (c1 == '\0')
			s1_ended = true;

		// End of s2 reached
		if (c2 == '\0')
			s2_ended = true;
	}

	// Strings equal
	return 0;
}
