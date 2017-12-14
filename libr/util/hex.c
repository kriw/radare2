/* radare - LGPL - Copyright 2007-2016 - pancake */

#include "r_types.h"
#include "r_util.h"
#include <stdio.h>
#include <ctype.h>

/* int c; ret = hex_to_byte(&c, 'c'); */
R_API int r_hex_to_byte(ut8 *val, ut8 c) {
	if (IS_DIGIT(c)) {
		*val = (ut8)(*val) * 16 + (c - '0');
	} else if (c >= 'A' && c <= 'F') {
		*val = (ut8)(*val) * 16 + (c - 'A' + 10);
	} else if (c >= 'a' && c <= 'f') {
		*val = (ut8)(*val) * 16 + (c - 'a' + 10);
	} else {
		return 1;
	}
	return 0;
}

R_API char *r_hex_from_py_str(char *out, const char *code) {
	if (!strncmp (code, "'''", 3)) {
		const char *s = code + 2;
		return r_hex_from_c_str (out, &s);
	} else {
		return r_hex_from_c_str (out, &code);
	}
	return out;
}

const char *skip_comment_py(const char *code) {
	if (*code != '#') {
		return code;
	}
	char *end = strchr (code, '\n');
	if (end) {
		code = end;
	}
	return code + 1;
}

R_API char *r_hex_from_py_array(char *out, const char *code) {
	const char abc[] = "0123456789abcdef";
	if (*code != '[' || strchr (code, ']') == NULL) {
		return NULL;
	}
	code++;
	for (; *code; code++) {
		char *comma = strchr (code, ',');
		if (!comma) {
			comma = strchr (code, ']');
		}
		char *word = r_str_ndup (code, comma - code);
		char * _word = word;
		while (*word == ' ' || *word == '\t' || *word == '\n') {
			word++;
			word = skip_comment_py (word);
		}
		if (IS_DIGIT (*word)) {
			ut8 n = (ut8)r_num_math (NULL, word);
			*out++ = abc[(n >> 4) & 0xf];
			*out++ = abc[n & 0xf];
		}
		free (_word);
		code = comma;
		if (*code == ']') {
			break;
		}
	}
	return out;
}

R_API char* r_hex_from_py(const char *code) {
	if (code == NULL) {
		return NULL;
	}
	char * const ret = malloc (strlen (code) * 3);
	if (!ret) {
		return NULL;
	}
	*ret = '\0';
	char *out = ret;
	const char *tmp_code = strchr (code, '=');
	if (tmp_code != NULL) {
		code = tmp_code;
	}
	for (; *code && *code != '[' && *code != '\''
	  && *code != '"'; code++) {
		code = skip_comment_py (code);
	}
	if (*code == '[') {
		out = r_hex_from_py_array (out, code);
	} else if (*code == '"' || *code == '\'') {
		out = r_hex_from_py_str (out, code);
	}
	if (out == NULL) {
		free (ret);
		return NULL;
	} else {
		*out++ = '\0';
		return ret;
	}
	return ret;
}

R_API char *r_hex_from_c_str(char *out, const char **code) {
	const char abc[] = "0123456789abcdefABCDEF";
	const char *iter = *code;
	if (*iter != '\'' && *iter != '"') {
		return NULL;
	}
	const char end_char = *iter;
	iter++;
	for (; *iter && *iter != end_char; iter++) {
		if (*iter == '\\') {
			iter++;
			switch (iter[0]) {
			case 'e': *out++='1';*out++='b';break;
			case 'r': *out++='0';*out++='d';break;
			case 'n': *out++='0';*out++='a';break;
			case 'x': {
				char c1 = iter[1];
				char c2 = iter[2];
				iter += 2;
				if (c1 == '\0' || c2 == '\0') {
					return NULL;
				} else if (strchr (abc, c1) && strchr (abc, c2)) {
					*out++ = tolower (c1);
					*out++ = tolower (c2);
				} else {
					return NULL;
				}
				break;
			  }
			default:
				if (iter[0] == end_char) {
					*out++ = abc[*iter >> 4];
					*out++ = abc[*iter & 0xf];
				}
				return NULL;
			}
		} else {
			*out++ = abc[*iter >> 4];
			*out++ = abc[*iter & 0xf];
		}
	}
	*code = iter;
	return out;
}

const char *skip_comment_c(const char *code) {
	if (!strncmp (code, "/*", 2)) {
		char *end = strstr (code, "*/");
		if (end) {
			code = end + 2;
		} else {
			eprintf ("Missing closing comment\n");
		}
	} else if (!strncmp (code, "//", 2)) {
		char *end = strchr (code, '\n');
		if (end) {
			code = end + 2;
		}
	}
	return code;
}

R_API char *r_hex_from_c_array(char *out, const char *code) {
	const char abc[] = "0123456789abcdef";
	if (*code != '{' || strchr(code, '}') == NULL) {
		return NULL;
	}
	code++;
	for (; *code; code++) {
		const char *comma = strchr (code, ',');
		if (!comma) {
			comma = strchr (code, '}');
		}
		const char *word = r_str_ndup (code, comma - code);
		char * _word = word;
		word = skip_comment_c (word);
		while (*word == ' ' || *word == '\t' || *word == '\n') {
			word++;
			word = skip_comment_c (word);
		}
		if (IS_DIGIT (*word)) {
			ut8 n = (ut8)r_num_math (NULL, word);
			*out++ = abc[(n >> 4) & 0xf];
			*out++ = abc[n & 0xf];
		}
		free (_word);
		code = comma;
		if (*code == '}') {
			break;
		}
	}
	return out;
}

/* convert:
 *    char *foo = "\x41\x23\x42\x1b";
 * into:
 *    4123421b
 */
R_API char *r_hex_from_c(const char *code) {
	if (code == NULL) {
		return NULL;
	}
	char * const ret = malloc (strlen (code) * 3);
	if (!ret) {
		return NULL;
	}
	*ret = '\0';
	char *out = ret;
	const char *tmp_code = strchr (code, '=');
	if (tmp_code != NULL) {
		code = tmp_code;
	}
	for (; *code != '\0' && *code != '{' && *code != '"'; code++) {
		code = skip_comment_c (code);
	}
	if (*code == '{') {
		out = r_hex_from_c_array (out, code);
	} else if (*code == '"') {
		const char *s1, *s2;
		s1 = code;
		do {
			code = s1;
			out = r_hex_from_c_str (out, &code);
			if (out == NULL) {
				break;
			}
			s1 = strchr (code + 1, '"');
			s2 = strchr (code + 1, ';');
		} while (s1 != NULL && s2 != NULL && (s1 <= s2));
	}
	if (out == NULL) {
		free (ret);
		return NULL;
	} else {
		*out++ = '\0';
		return ret;
	}
}

/* convert
 * "\x41\x23\x42\x1b"
 * "\x41\x23\x42\x1b"
 * into
 * 4123421b4123421b
 */
R_API char *r_hex_no_code(const char *code) {
	if (code == NULL) {
		return NULL;
	}
	char * const ret = malloc (strlen (code) * 3);
	if (!ret) {
		return NULL;
	}
	*ret = '\0';
	char *out = ret;
	out = r_hex_from_c_str (out, &code);
	code = strchr (code + 1, '"');
	if (out == NULL) {
		free (ret);
		return NULL;
	}
	*out = '\0';
	while (out != NULL && code != NULL) {
		*out = '\0';
		out = r_hex_from_c_str (out, &code);
		code = strchr (code + 1, '"');
	}
	return ret;
}

R_API char *r_hex_from_code(const char *code) {
	if (strchr (code, '=') == NULL) {
		return r_hex_no_code (code);
	} else if (strstr (code, "char") || strstr (code, "int")) {
		//C language
		return r_hex_from_c (code);
	} else {
		// Python
		return r_hex_from_py (code);
	}
}

/* int byte = hexpair2bin("A0"); */
// (0A) => 10 || -1 (on error)
R_API int r_hex_pair2bin(const char *arg) {
	ut8 *ptr, c = 0, d = 0;
	ut32 j = 0;

	for (ptr = (ut8*)arg; ;ptr = ptr + 1) {
		if (!*ptr || *ptr==' ' || j==2) {
			break;
		}
		d = c;
		if (*ptr!='.' && r_hex_to_byte (&c, *ptr)) {
			eprintf ("Invalid hexa string at char '%c' (%s).\n",
				*ptr, arg);
			return -1;
		}
		c |= d;
		if (j++ == 0) {
			c <<= 4;
		}
	}
	return (int)c;
}

R_API int r_hex_bin2str(const ut8 *in, int len, char *out) {
	int i, idx;
	char tmp[5];
	if (len < 0)
		return 0;
	for (idx = i = 0; i < len; i++, idx += 2)  {
		snprintf (tmp, sizeof (tmp), "%02x", in[i]);
		memcpy (out+idx, tmp, 2);
	}
	out[idx] = 0;
	return len;
}

R_API char *r_hex_bin2strdup(const ut8 *in, int len) {
	int i, idx;
	char tmp[5], *out;

	if ((len + 1) * 2 < len) return NULL;
	out = malloc ((len + 1) * 2);
	if (!out) return NULL;
	for (i = idx = 0; i < len; i++, idx += 2)  {
		snprintf (tmp, sizeof (tmp), "%02x", in[i]);
		memcpy (out+idx, tmp, 2);
	}
	out[idx] = 0;
	return out;
}

R_API int r_hex_str2bin(const char *in, ut8 *out) {
	long nibbles = 0;

	while (in && *in) {
		ut8 tmp;
		/* skip hex prefix */
		if (*in == '0' && in[1] == 'x') {
			in += 2;
		}
		/* read hex digits */
		while (!r_hex_to_byte (out ? &out[nibbles/2] : &tmp, *in)) {
			nibbles++;
			in++;
		}
		if (*in == '\0') {
			break;
		}
		/* comments */
		if (*in == '#' || (*in == '/' && in[1] == '/')) {
			if ((in = strchr (in, '\n'))) {
				in++;
			}
			continue;
		} else if (*in == '/' && in[1] == '*') {
			if ((in = strstr (in, "*/"))) {
				in += 2;
			}
			continue;
		} else if (!IS_WHITESPACE (*in) && *in != '\n') {
			/* this is not a valid string */
			return 0;
		}
		/* ignore character */
		in++;
	}

	if (nibbles % 2) {
		if (out) r_hex_to_byte (&out[nibbles/2], '0');
		return -(nibbles+1) / 2;
	}

	return nibbles / 2;
}

R_API int r_hex_str2binmask(const char *in, ut8 *out, ut8 *mask) {
	ut8 *ptr;
	int len, ilen = strlen (in)+1;
	int has_nibble = 0;
	memcpy (out, in, ilen);
	for (ptr=out; *ptr; ptr++) if (*ptr=='.') *ptr = '0';
	len = r_hex_str2bin ((char*)out, out);
	if (len<0) { has_nibble = 1; len = -(len+1); }
	if (len != -1) {
		memcpy (mask, in, ilen);
		if (has_nibble)
			memcpy (mask+ilen, "f0", 3);
		for (ptr=mask; *ptr; ptr++) *ptr = (*ptr=='.')?'0':'f';
		len = r_hex_str2bin ((char*)mask, mask);
		if (len<0) len++;
	}
	return len;
}

R_API st64 r_hex_bin_truncate (ut64 in, int n) {
	switch (n) {
	case 1:
		if ((in&UT8_GT0))
			return UT64_8U|in;
		return in&UT8_MAX;
	case 2:
		if ((in&UT16_GT0))
			return UT64_16U|in;
		return in&UT16_MAX;
	case 4:
		if ((in&UT32_GT0))
			return UT64_32U|in;
		return in&UT32_MAX;
	case 8:
		return in&UT64_MAX;
	}
	return in;
}

// Check if str contains only hexademical characters and return length of bytes
R_API int r_hex_str_is_valid(const char* str) {
	int i;
	if (!strncmp (str, "0x", 2)) {
		str += 2;
	}
	for (i = 0; str[i] != '\0' && str[i] != ' '; i++) {
		if (ISHEXCHAR (str[i])) {
			continue;
		}
		return -1; //if we're here, then str isnt valid
	}
	return i;
}
