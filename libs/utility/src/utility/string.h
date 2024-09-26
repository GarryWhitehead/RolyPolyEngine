/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __UTILITY_STRING_H__
#define __UTILITY_STRING_H__

#include <stdbool.h>
#include <stdint.h>

// Forward declarations.
typedef struct Arena arena_t;

typedef struct String
{
    char* data;
    uint32_t len;
} string_t;

/**
 Create a new NULL terminated string instance.
 @param str The character string the new instance will contain.
 @param arena An arena allocator.
 @return The new string instance.
 */
string_t string_init(const char* str, arena_t* arena);

string_t string_copy(string_t* other, arena_t* arena);

/**
 Compare two strings.
 @param a The lhs string.
 @param b The rhs string.
 @return If the two strings match, returns true.
 */
bool string_cmp(string_t* a, string_t* b);

/**
 Create a sub-string from the original.
 @param s The original string.
 @param start The index within @sa s the sub-string will start from.
 @param end The index within @sa s the sub-string will end at.
 @param arena An arena allocator.
 @return The sub-string.
 */
string_t string_substring(string_t* s, uint32_t start, uint32_t end, arena_t* arena);

/**
 Check if a string contains a sub-string within it.
 @param s The original string.
 @param sub The sub-string to check for.
 @return If the sub-string is found, returns a pointer to the start of the sub-string.
 */
char* string_contains(string_t* s, const char* sub);

/**
 Split a string based on the specified character literal.
 @note If the split character is not found, then the original string is returned.
 @param s The string to split.
 @param literal The character to split the string with.
 @param [out] count The number of splits which occurred. If the split character is not found in the
 string, the count will be one.
 @param arena AN arena allocator.
 @return A list of char pointers to each split string.
 */
string_t** string_split(string_t* s, char literal, uint32_t* count, arena_t* arena);

/**
 Append two strings.
 If @sa a is NULL, then the output string will contain string @sa b.
 @param a The lhs string.
 @param b The rhs string.
 @param arena An arena allocator.
 @return The appended string a + b.
 */
string_t string_append(string_t* a, const char* b, arena_t* arena);

/**
 Append three strings.
 If @sa a is NULL, then the output string will contain strings @sb + @sa c.
 @param a The lhs string.
 @param b The mid string.
 @param c The rhs string.
 @param arena An arena allocator.
 @return The appended string a + b + c.
 */
string_t string_append3(string_t* a, const char* b, const char* c, arena_t* arena);

/**
 Find the first instance of the specified character in a string.
 @param str A string instance.
 @param c The character to find the first instance of.
 @return An index within @sa str of the first occurrence of the character. If the character is not
 present, returns UINT_MAX.
 */
uint32_t string_find_first_of(string_t* str, char c);

/**
 Find the last instance of the specified character in a string.
 @param str A string instance.
 @param c The character to find the last instance of.
 @return An index within @sa str of the last occurrence of the character. If the character is not
 present, returns UINT_MAX.
 */
uint32_t string_find_last_of(string_t* str, char c);

/**
 Count the number of times a sub-string occurs within a string.
 @param str A string instance.
 @param cs The sub-string to count the number of instances of.
 @return The sub-string count.
 */
uint32_t string_count(string_t* str, const char* cs);

/**
 Remove a section of a string based upon the specified indices.
 @param str A string instance.
 @param start_idx The start index within @sa str to remove.
 @param end_idx The end index within @sa str to remove.
 @param arena An arena allocator.
 @return A string instance with the removed text block.
 */
string_t string_remove(string_t* str, uint32_t start_idx, uint32_t end_idx, arena_t* arena);

/**
 Trim all instances of the specified character from a string.
 @param str A string instance.
 @param c The character to trim from string @sa str.
 @param arena An arena allocator.
 @return The trimmed string. If the character @sa c is not present in the string, then the original
 is returned.
 */
string_t string_trim(string_t* str, char c, arena_t* arena);

/**
 Replace all instances of a character in a string with another.
 @param str A string instance.
 @param orig The character to replace.
 @param rep The character the @sa orig will be replaced with.
 @param arena An arena allocator.
 @return A string instamce with the replaced characters. If character @sa orig is not present in the
 string, the original is returned.
 */
string_t string_replace(string_t* str, const char* orig, const char* rep, arena_t* arena);

#endif