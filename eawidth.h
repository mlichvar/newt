#include <stddef.h>

typedef enum {
   ambiguous, neutral, narrow, half_width, wide, full_width
} east_asia_type;

/**
 *  get_east_asia_type  returns the character classification of a given
 *                      Unicode character according to Unicode Technical
 *                      Report 11.
 *
 *  @param unicode      the Unicode/ISO-10646 character to get the type of
 *  @return             the classification of the character, according to
 *                      UAX#11 data.
 */
east_asia_type get_east_asia_type(wchar_t unicode);

/**
 *  get_east_asia_width measures the amount of space a multibyte string
 *                      takes an a traditional fixed width terminal such
 *                      as xterm in character cells.
 *                      
 *  @param locale_name  used to determine the if character width should
 *                      be interpreted as CJK or Asian in the case of
 *                      Greek and Cyrillic characters. If NULL, it will
 *                      use setlocale() to determine the LC_CTYPE setting.
 *  @param s            the multibyte string to measure the width of.
 *  @param x            the current cursor position, used to handle the
 *                      calculation of tab widths. 0 is the first column.
 *  @return             the number of fixed width character cells the string
 *                      takes up. If the string is an illegal multibyte
 *                      string, or a memory error or other occurs, INT_MAX
 *                      is returned and errno is set.
 */
int get_east_asia_str_width(const char *locale_name, const char *s, int x);
int get_east_asia_str_n_width (const char *locale_name, const char *s, size_t n, int x);
int east_asia_mblen (const char *locale_name, const char *s, size_t n, int x);
