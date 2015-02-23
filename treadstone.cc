// Copyright (c) 2014-2015, Robert Escriva
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of libtreadstone nor the names of its contributors may
//       be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX
#include <errno.h>

// STL
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// e
#include <e/endian.h>
#include <e/guard.h>
#include <e/slice.h>
#include <e/varint.h>

// Treadstone
#include <treadstone.h>
#include "namespace.h"
#include "visibility.h"
#include "treadstone-types.h"

BEGIN_TREADSTONE_NAMESPACE

void
j2b_skip_whitespace(const char** ptr, const char* limit)
{
    while (*ptr < limit)
    {
        if (!isspace(**ptr))
        {
            break;
        }

        ++*ptr;
    }
}

bool
j2b_make_room_for(size_t room,
                  unsigned char** binary,
                  size_t* binary_sz,
                  size_t* binary_cap)
{
    if (*binary_sz + room <= *binary_cap)
    {
        return true;
    }

    size_t new_cap = *binary_cap + ((*binary_cap) >> 2) + room;
    unsigned char* tmp = reinterpret_cast<unsigned char*>(realloc(*binary, new_cap));

    if (!tmp)
    {
        return false;
    }

    *binary = tmp;
    *binary_cap = new_cap;
    return true;
}

bool
j2b_transform(const char** ptr, const char* limit,
              unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_value(const char** ptr, const char* limit,
          unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_object(const char** ptr, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_array(const char** ptr, const char* limit,
          unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_string(const char** ptr, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_number(const char** ptr, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_true(const char** ptr, const char* limit,
         unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_false(const char** ptr, const char* limit,
          unsigned char** binary, size_t* binary_sz, size_t* binary_cap);
bool
j2b_null(const char** ptr, const char* limit,
         unsigned char** binary, size_t* binary_sz, size_t* binary_cap);

// TODO use constexpr to calculate during compile time
const unsigned char empty_object[2] = { BINARY_OBJECT, 0 };

bool
j2b_transform(const char** ptr, const char* limit,
              unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(*limit == '\0');
    j2b_skip_whitespace(ptr, limit);

    if (!j2b_value(ptr, limit, binary, binary_sz, binary_cap))
    {
        return false;
    }

    j2b_skip_whitespace(ptr, limit);
    return *ptr == limit;
}

bool
j2b_value(const char** ptr, const char* limit,
            unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(*ptr < limit);
    j2b_skip_whitespace(ptr, limit);

    if (*ptr >= limit)
    {
        return false;
    }

    switch (**ptr)
    {
        case '{':
            return j2b_object(ptr, limit, binary, binary_sz, binary_cap);
        case '[':
            return j2b_array(ptr, limit, binary, binary_sz, binary_cap);
        case '"':
            return j2b_string(ptr, limit, binary, binary_sz, binary_cap);
        case '+':
        case '-':
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'e':
        case 'E':
            return j2b_number(ptr, limit, binary, binary_sz, binary_cap);
        case 't':
            return j2b_true(ptr, limit, binary, binary_sz, binary_cap);
        case 'f':
            return j2b_false(ptr, limit, binary, binary_sz, binary_cap);
        case 'n':
            return j2b_null(ptr, limit, binary, binary_sz, binary_cap);
        default:
            return false;
    }
}

bool
j2b_prepend_header(unsigned char type, size_t starting_sz,
                   unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(starting_sz <= *binary_sz);
    uint64_t bytes = *binary_sz - starting_sz;
    size_t diff = 1 + e::varint_length(bytes);

    if (!j2b_make_room_for(diff, binary, binary_sz, binary_cap))
    {
        return false;
    }

    char* tmp = reinterpret_cast<char*>(*binary) + starting_sz;
    memmove(tmp + 1 + e::varint_length(bytes), tmp, bytes);
    e::pack8be(type, tmp);
    e::packvarint64(bytes, reinterpret_cast<char*>(tmp + 1));
    *binary_sz += diff;
    return true;
}

bool
j2b_object(const char** ptr, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(*ptr < limit);
    assert(**ptr == '{');
    ++*ptr;
    bool first = true;
    size_t start = *binary_sz;

    while (*ptr < limit)
    {
        j2b_skip_whitespace(ptr, limit);

        if (*ptr < limit && **ptr == '}')
        {
            ++*ptr;
            return j2b_prepend_header(BINARY_OBJECT, start, binary, binary_sz, binary_cap);
        }

        if (!first)
        {
            if (*ptr >= limit || **ptr != ',')
            {
                return false;
            }

            ++*ptr;
        }

        first = false;
        j2b_skip_whitespace(ptr, limit);

        if (*ptr >= limit || **ptr != '"' ||
            !j2b_string(ptr, limit, binary, binary_sz, binary_cap))
        {
            return false;
        }

        j2b_skip_whitespace(ptr, limit);

        if (*ptr >= limit || **ptr != ':')
        {
            return false;
        }

        ++*ptr;
        j2b_skip_whitespace(ptr, limit);

        if (!j2b_value(ptr, limit, binary, binary_sz, binary_cap))
        {
            return false;
        }
    }

    return false;
}

bool
j2b_array(const char** ptr, const char* limit,
          unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(*ptr < limit);
    assert(**ptr == '[');
    ++*ptr;
    bool first = true;
    size_t start = *binary_sz;

    while (*ptr < limit)
    {
        j2b_skip_whitespace(ptr, limit);

        if (*ptr < limit && **ptr == ']')
        {
            ++*ptr;
            return j2b_prepend_header(BINARY_ARRAY, start, binary, binary_sz, binary_cap);
        }

        if (!first)
        {
            if (*ptr >= limit || **ptr != ',')
            {
                return false;
            }

            ++*ptr;
        }

        first = false;
        j2b_skip_whitespace(ptr, limit);

        if (!j2b_value(ptr, limit, binary, binary_sz, binary_cap))
        {
            return false;
        }
    }

    return false;
}

bool
j2b_string(const char** ptr, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    assert(*ptr < limit);
    assert(**ptr == '"');
    ++*ptr;
    const char* start = *ptr;

    while (*ptr < limit)
    {
        if (**ptr == '\\')
        {
            if (*ptr + 1 >= limit)
            {
                return false;
            }

            if ((*ptr)[1] == 'u')
            {
                if (*ptr + 6 >= limit)
                {
                    return false;
                }

                *ptr += 6;
            }
            else
            {
                *ptr += 2;
            }
        }
        else if (**ptr == '"')
        {
            break;
        }
        else
        {
            ++*ptr;
        }
    }

    if (*ptr >= limit)
    {
        return false;
    }

    uint64_t bytes = *ptr - start;
    uint64_t diff = 1 + e::varint_length(bytes) + bytes;

    if (!j2b_make_room_for(diff, binary, binary_sz, binary_cap))
    {
        return false;
    }

    unsigned char* tmp = *binary + *binary_sz;
    tmp = e::pack8be(BINARY_STRING, tmp);
    tmp = e::packvarint64(bytes, tmp);
    memmove(tmp, start, bytes);
    *binary_sz += diff;
    ++*ptr;
    return true;
}

bool
j2b_number(const char** start, const char* limit,
           unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    const char* tmp = *start;
    const char* end = tmp;
    enum { INTEGER, DOUBLE } type = INTEGER;

    while (end < limit)
    {
        if (isdigit(*end) ||
            *end == '+' ||
            *end == '-' ||
            *end == '.' ||
            *end == 'e' ||
            *end == 'E')
        {
            if (*end == '.' || *end == 'e' || *end == 'E')
            {
                type = DOUBLE;
            }

            ++end;
        }
        else
        {
            break;
        }
    }

    assert(tmp < end);
    assert(type == INTEGER || type == DOUBLE);

    if (!j2b_make_room_for(10, binary, binary_sz, binary_cap))
    {
        return false;
    }

    if (type == INTEGER)
    {
        char* e = NULL;
        long long int x = strtoll(tmp, &e, 10);

        if (e != end)
        {
            return false;
        }

        unsigned char* ptr = *binary + *binary_sz;
        ptr = e::pack8be(BINARY_INTEGER, ptr);
        ptr = e::packvarint64(x, ptr);
        *binary_sz += ptr - (*binary + *binary_sz);
    }

    if (type == DOUBLE)
    {
        char* e = NULL;
        double x = strtod(tmp, &e);

        if (e != end)
        {
            return false;
        }

        unsigned char* ptr = *binary + *binary_sz;
        ptr = e::pack8be(BINARY_DOUBLE, ptr);
        ptr = e::packdoublebe(x, ptr);
        *binary_sz += 9;
    }

    *start = end;
    return true;
}

bool
j2b_constant(const char** ptr, const char* limit,
             const char* constant, size_t constant_sz, unsigned char c,
             unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    if (*ptr + constant_sz > limit)
    {
        return false;
    }

    if (memcmp(*ptr, constant, constant_sz) != 0)
    {
        return false;
    }

    if (!j2b_make_room_for(1, binary, binary_sz, binary_cap))
    {
        return false;
    }

    unsigned char* x = *binary + *binary_sz;
    *x = c;
    *ptr += constant_sz;
    ++*binary_sz;
    assert(*ptr <= limit);
    assert(*binary_sz <= *binary_cap);
    return true;
}

bool
j2b_true(const char** ptr, const char* limit,
         unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    return j2b_constant(ptr, limit, "true", 4, BINARY_TRUE, binary, binary_sz, binary_cap);
}

bool
j2b_false(const char** ptr, const char* limit,
          unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    return j2b_constant(ptr, limit, "false", 5, BINARY_FALSE, binary, binary_sz, binary_cap);
}

bool
j2b_null(const char** ptr, const char* limit,
         unsigned char** binary, size_t* binary_sz, size_t* binary_cap)
{
    return j2b_constant(ptr, limit, "null", 4, BINARY_NULL, binary, binary_sz, binary_cap);
}

bool
b2j_make_room_for(size_t room,
                  char** json,
                  size_t* json_sz,
                  size_t* json_cap)
{
    if (*json_sz + room <= *json_cap)
    {
        return true;
    }

    size_t new_cap = *json_cap + ((*json_cap) >> 2) + room;
    char* tmp = reinterpret_cast<char*>(realloc(*json, new_cap));

    if (!tmp)
    {
        return false;
    }

    *json = tmp;
    *json_cap = new_cap;
    return true;
}

bool
b2j_append_char(char c,
                char** json,
                size_t* json_sz,
                size_t* json_cap)
{
    if (!b2j_make_room_for(1, json, json_sz, json_cap))
    {
        return false;
    }

    (*json)[*json_sz] = c;
    ++*json_sz;
    return true;
}

bool
b2j_transform(const unsigned char** ptr, const unsigned char* limit,
              char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_value(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_object(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_array(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_string(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_double(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_integer(const unsigned char** ptr, const unsigned char* limit,
            char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_true(const unsigned char** ptr, const unsigned char* limit,
         char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_false(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap);
bool
b2j_null(const unsigned char** ptr, const unsigned char* limit,
         char** json, size_t* json_sz, size_t* json_cap);

bool
b2j_transform(const unsigned char** ptr, const unsigned char* limit,
              char** json, size_t* json_sz, size_t* json_cap)
{
    return b2j_value(ptr, limit, json, json_sz, json_cap) &&
           *ptr == limit;
}

bool
b2j_value(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit)
    {
        return false;
    }

    switch (**ptr)
    {
        case BINARY_OBJECT:
            return b2j_object(ptr, limit, json, json_sz, json_cap);
        case BINARY_ARRAY:
            return b2j_array(ptr, limit, json, json_sz, json_cap);
        case BINARY_STRING:
            return b2j_string(ptr, limit, json, json_sz, json_cap);
        case BINARY_DOUBLE:
            return b2j_double(ptr, limit, json, json_sz, json_cap);
        case BINARY_INTEGER:
            return b2j_integer(ptr, limit, json, json_sz, json_cap);
        case BINARY_TRUE:
            return b2j_true(ptr, limit, json, json_sz, json_cap);
        case BINARY_FALSE:
            return b2j_false(ptr, limit, json, json_sz, json_cap);
        case BINARY_NULL:
            return b2j_null(ptr, limit, json, json_sz, json_cap);
        default:
            return false;
    }
}

bool
b2j_object(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit || **ptr != BINARY_OBJECT)
    {
        return false;
    }

    uint64_t sz;
    const unsigned char* end = e::varint64_decode(*ptr + 1, limit, &sz);

    if (end == NULL || end + sz > limit)
    {
        return false;
    }

    *ptr = end;
    end += sz;

    if (!b2j_append_char('{', json, json_sz, json_cap))
    {
        return false;
    }

    bool first = true;

    while (*ptr < end)
    {

        if (!first)
        {
            if (!b2j_append_char(',', json, json_sz, json_cap))
            {
                return false;
            }
        }

        if (!b2j_string(ptr, end, json, json_sz, json_cap))
        {
            return false;
        }

        if (!b2j_append_char(':', json, json_sz, json_cap))
        {
            return false;
        }

        if (!b2j_value(ptr, end, json, json_sz, json_cap))
        {
            return false;
        }

        first = false;
    }

    return *ptr == end && b2j_append_char('}', json, json_sz, json_cap);
}

bool
b2j_array(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit || **ptr != BINARY_ARRAY)
    {
        return false;
    }

    uint64_t sz;
    const unsigned char* end = e::varint64_decode(*ptr + 1, limit, &sz);

    if (end == NULL || end + sz > limit)
    {
        return false;
    }

    *ptr = end;

    if (!b2j_append_char('[', json, json_sz, json_cap))
    {
        return false;
    }

    bool first = true;

    while (*ptr < end + sz)
    {
        if (!first)
        {
            if (!b2j_append_char(',', json, json_sz, json_cap))
            {
                return false;
            }
        }

        if (!b2j_value(ptr, end + sz, json, json_sz, json_cap))
        {
            return false;
        }

        first = false;
    }

    return *ptr == end + sz && b2j_append_char(']', json, json_sz, json_cap);
}

bool
b2j_string(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit || **ptr != BINARY_STRING)
    {
        return false;
    }

    uint64_t sz;
    const unsigned char* end = e::varint64_decode(*ptr + 1, limit, &sz);

    if (end == NULL || end + sz > limit)
    {
        return false;
    }

    if (!b2j_make_room_for(sz + 2, json, json_sz, json_cap))
    {
        return false;
    }

    (*json)[*json_sz] = '"';
    ++*json_sz;
    memmove(*json + *json_sz, end, sz);
    *json_sz += sz;
    (*json)[*json_sz] = '"';
    ++*json_sz;
    *ptr = end + sz;
    return true;
}

bool
b2j_double(const unsigned char** ptr, const unsigned char* limit,
           char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr + sizeof(double) >= limit || **ptr != BINARY_DOUBLE)
    {
        return false;
    }

    double num;
    e::unpackdoublebe(*ptr + 1, &num);
    char buf[40];
    int sz = snprintf(buf, 40, "%g", num);

    if (sz >= 40 || sz <= 0)
    {
        return false;
    }

    if (!b2j_make_room_for(sz, json, json_sz, json_cap))
    {
        return false;
    }

    memmove(*json + *json_sz, buf, sz);
    *ptr += sizeof(unsigned char) + sizeof(double);
    *json_sz += sz;
    return true;
}

bool
b2j_integer(const unsigned char** ptr, const unsigned char* limit,
            char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit || **ptr != BINARY_INTEGER)
    {
        return false;
    }

    uint64_t unum;
    const unsigned char* end = e::varint64_decode(*ptr + 1, limit, &unum);

    if (end == NULL)
    {
        return false;
    }

    int64_t num = unum;
    char buf[40];
    int sz = snprintf(buf, 40, "%lld", (long long)num);

    if (sz >= 40 || sz <= 0)
    {
        return false;
    }

    if (!b2j_make_room_for(sz, json, json_sz, json_cap))
    {
        return false;
    }

    memmove(*json + *json_sz, buf, sz);
    *ptr = end;
    *json_sz += sz;
    return true;
}

bool
b2j_constant(const unsigned char** ptr, const unsigned char* limit,
             const char* constant, size_t constant_sz, unsigned char c,
             char** json, size_t* json_sz, size_t* json_cap)
{
    if (*ptr >= limit || **ptr != c)
    {
        return false;
    }

    if (!b2j_make_room_for(constant_sz, json, json_sz, json_cap))
    {
        return false;
    }

    memmove(*json + *json_sz, constant, constant_sz);
    *json_sz += constant_sz;
    ++*ptr;
    return true;
}

bool
b2j_true(const unsigned char** ptr, const unsigned char* limit,
         char** json, size_t* json_sz, size_t* json_cap)
{
    return b2j_constant(ptr, limit, "true", 4, BINARY_TRUE, json, json_sz, json_cap);
}

bool
b2j_false(const unsigned char** ptr, const unsigned char* limit,
          char** json, size_t* json_sz, size_t* json_cap)
{
    return b2j_constant(ptr, limit, "false", 5, BINARY_FALSE, json, json_sz, json_cap);
}

bool
b2j_null(const unsigned char** ptr, const unsigned char* limit,
         char** json, size_t* json_sz, size_t* json_cap)
{
    return b2j_constant(ptr, limit, "null", 4, BINARY_NULL, json, json_sz, json_cap);
}

void
free_if_allocated_unsigned_char_star(unsigned char** x)
{
    if (*x)
    {
        free(*x);
    }
}

void
free_if_allocated_char_star(char** x)
{
    if (*x)
    {
        free(*x);
    }
}

struct path
{
    enum component_t { FIELD, INDEX };
    struct component
    {
        component();
        component(const std::string& f) : type(FIELD), field(f), index() {}
        component(int i) : type(INDEX), field(), index(i) {}

        component_t type;
        std::string field;
        int index;
    };

    path(const char* p);

    bool is_valid() const { return m_valid; }
    size_t depth() const { return m_components.size(); }
    const component& get(size_t i) const { return m_components[i]; }

    const component& head() const { return m_components[0]; }
    const component& back() const { return m_components[depth()-1]; }

    // Get whole path w/o back
    path front() const;

    // Get whole path w/o head
    path tail() const;

private:
    path() : m_valid(true) {}

    void parse(const char* p);
    friend std::ostream& operator << (std::ostream& lhs, const path& p);

    bool m_valid;
    std::vector<component> m_components;
};

path path::front() const
{
    path p;

    for(uint32_t i = 0; i < depth() - 1; ++i)
    {
        p.m_components.push_back(m_components[i]);
    }

    return p;
}

path path::tail() const
{
    path p;

    for(uint32_t i = 1; i < depth(); ++i)
    {
        p.m_components.push_back(m_components[i]);
    }

    return p;
}

std::ostream&
operator << (std::ostream& lhs, const path& p)
{
    if (!p.m_valid)
    {
        return lhs << "<invalid path>";
    }

    for (size_t i = 0; i < p.m_components.size(); ++i)
    {
        if (i > 0)
        {
            lhs << ", ";
        }

        switch (p.m_components[i].type)
        {
            case path::FIELD:
                lhs << "FIELD:" << p.m_components[i].field;
                break;
            case path::INDEX:
                lhs << "INDEX:" << p.m_components[i].index;
                break;
            default:
                lhs << "<bad component>";
                break;
        }
    }

    return lhs;
}

path :: path(const char* p)
    : m_valid(true)
    , m_components()
{
    parse(p);
}

void
path :: parse(const char* p)
{
    char prev = '\0';

    while (true)
    {
        if (*p == '\0')
        {
            break;
        }

        if (*p == '[')
        {
            if (prev != '\0' && prev != 'I' && prev != 'F')
            {
                m_valid = false;
                return;
            }

            char* end = NULL;
            long int x = strtol(p + 1, &end, 0);

            if (p + 1 >= end || *end != ']')
            {
                m_valid = false;
                return;
            }

            m_components.push_back(component(x));
            assert(*end == ']');
            ++end;
            p = end;
            prev = 'I';
        }
        else if (*p == '.')
        {
            if (prev != 'I' && prev != 'F')
            {
                m_valid = false;
                return;
            }

            ++p;
            prev = '.';
        }
        else
        {
            if (prev != '\0' && prev != '.')
            {
                m_valid = false;
                return;
            }

            const char* end = p;

            while (*end != '[' && *end != ']' &&
                   *end != '.' && *end != '\0')
            {
                ++end;
            }

            if (*end != '[' && *end != '.' && *end != '\0')
            {
                m_valid = false;
                return;
            }

            m_components.push_back(component(std::string(p, end)));
            p = end;
            prev = 'F';
        }
    }
}

END_TREADSTONE_NAMESPACE

TREADSTONE_API int
treadstone_json_to_binary(const char* json,
                          unsigned char** binary, size_t* binary_sz)
{
    // Invalid JSON
    if(json == NULL || strcmp(json, "") == 0)
    {
        *binary = NULL;
        *binary_sz = 0;
        return -1;
    }

    size_t json_sz = strlen(json);

    *binary = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * json_sz));
    *binary_sz = 0;

    if (!*binary)
    {
        *binary = NULL;
        *binary_sz = 0;
        // carry errno from failed malloc
        return -1;
    }

    int saved = errno;
    errno = EINVAL;
    const char* ptr = json;
    const char* limit = json + json_sz;
    bool ret = treadstone::j2b_transform(&ptr, limit, binary, binary_sz, &json_sz);

    if (ret)
    {
        errno = saved;
        return 0;
    }
    else
    {
        free(*binary);
        *binary = NULL;
        *binary_sz = 0;
        // errno set in j2b_transform, or is EINVAL from above
        return -1;
    }
}

TREADSTONE_API int
treadstone_binary_to_json(const unsigned char* binary, size_t binary_sz,
                          char** json)
{
    if(binary == NULL || binary_sz == 0)
    {
        // Allow empty binary data as a valid empty json
        *json = reinterpret_cast<char*>(malloc(strlen("{}")+1));
        strcpy(*json, "{}");
        return 0;
    }

    size_t json_sz = 0;
    size_t json_cap = binary_sz + (binary_sz >> 2);
    *json = reinterpret_cast<char*>(malloc(sizeof(char) * json_cap));

    if (!*json)
    {
        *json = NULL;
        // carry errno from failed malloc
        return -1;
    }

    int saved = errno;
    errno = EINVAL;
    const unsigned char* ptr = binary;
    const unsigned char* limit = binary + binary_sz;
    bool ret = treadstone::b2j_transform(&ptr, limit, json, &json_sz, &json_cap);

    if (ret)
    {
        errno = saved;

        if (!treadstone::b2j_make_room_for(1, json, &json_sz, &json_cap))
        {
            return -1;
        }

        memset(*json + json_sz, 0, json_cap - json_sz);
        return 0;
    }
    else
    {
        free(*json);
        *json = NULL;
        return -1;
    }
}

TREADSTONE_API int
treadstone_string_to_binary(const char* string, size_t string_sz,
                            unsigned char** binary, size_t* binary_sz)
{
    *binary_sz = string_sz + 1 + e::varint_length(string_sz);
    *binary = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * *binary_sz));

    if (!*binary)
    {
        return -1;
    }

    unsigned char* ptr = *binary;
    ptr = e::pack8be(BINARY_STRING, ptr);
    ptr = e::packvarint64(string_sz, ptr);
    memmove(ptr, string, string_sz);
    return 0;
}

TREADSTONE_API int
treadstone_integer_to_binary(int64_t number,
                             unsigned char** binary, size_t* binary_sz)
{
    *binary = reinterpret_cast<unsigned char*>(malloc(11));

    if (!*binary)
    {
        return -1;
    }

    unsigned char* ptr = *binary;
    unsigned char* start = ptr;
    ptr = e::pack8be(BINARY_INTEGER, ptr);
    ptr = e::packvarint64(number, ptr);
    *binary_sz = ptr - start;
    return 0;
}

TREADSTONE_API int
treadstone_double_to_binary(double number,
                            unsigned char** binary, size_t* binary_sz)
{
    *binary = reinterpret_cast<unsigned char*>(malloc(10));

    if (!*binary)
    {
        return -1;
    }

    unsigned char* ptr = *binary;
    unsigned char* start = ptr;
    ptr = e::pack8be(BINARY_DOUBLE, ptr);
    ptr = e::packdoublebe(number, ptr);
    *binary_sz = ptr - start;
    return 0;
}

TREADSTONE_API int
treadstone_binary_is_string(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;

    if (ptr >= limit || *ptr != BINARY_STRING)
    {
        return -1;
    }

    uint64_t sz;
    const unsigned char* end = e::varint64_decode(ptr + 1, limit, &sz);
    return (end != NULL && end + sz == limit) ? 0 : -1;
}

TREADSTONE_API size_t
treadstone_binary_string_bytes(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;
    assert (ptr < limit && *ptr == BINARY_STRING);
    uint64_t sz;
    const unsigned char* end = e::varint64_decode(ptr + 1, limit, &sz);
    assert(end != NULL && end + sz == limit);
    return sz;
}

TREADSTONE_API void
treadstone_binary_to_string(const unsigned char* binary, size_t binary_sz,
                            char* string)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;
    assert (ptr < limit && *ptr == BINARY_STRING);
    uint64_t sz;
    const unsigned char* end = e::varint64_decode(ptr + 1, limit, &sz);
    assert(end != NULL && end + sz == limit);
    memmove(string, end, sz);
}

TREADSTONE_API int
treadstone_binary_is_integer(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;

    if (ptr >= limit || *ptr != BINARY_INTEGER)
    {
        return -1;
    }

    uint64_t unum;
    const unsigned char* end = e::varint64_decode(ptr + 1, limit, &unum);
    return end == limit ? 0 : -1;
}

TREADSTONE_API int64_t
treadstone_binary_to_integer(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;
    assert(ptr < limit && *ptr == BINARY_INTEGER);
    uint64_t unum;
    const unsigned char* end = e::varint64_decode(ptr + 1, limit, &unum);
    assert(end == limit);
    int64_t num = unum;
    return num;
}

TREADSTONE_API int
treadstone_binary_is_double(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;
    return (ptr + 1 + sizeof(double) == limit && *ptr == BINARY_DOUBLE) ? 0 : -1;
}

TREADSTONE_API double
treadstone_binary_to_double(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = ptr + binary_sz;
    assert(ptr + 1 + sizeof(double) == limit && *ptr == BINARY_DOUBLE);
    double num;
    e::unpackdoublebe(ptr + 1, &num);
    return num;
}

TREADSTONE_API int
treadstone_validate_path(const char* path)
{
    treadstone::path p(path);
    return p.is_valid() ? 0 : -1;
}

struct treadstone_transformer
{
    treadstone_transformer(const unsigned char* binary, size_t binary_sz);
    ~treadstone_transformer() throw ();
    int output(unsigned char** binary, size_t* binary_sz);
    int unset_value(const char* path);
    int set_value(const treadstone::path& path,
                  const unsigned char* value, size_t value_sz);
    int extract_value(const char* path,
                      unsigned char** value, size_t* value_sz);
    int array_prepend_value(const char* path,
                            const unsigned char* value, size_t value_sz);
    int array_append_value(const char* path,
                           const unsigned char* value, size_t value_sz);

    private:
        struct stub
        {
            stub(unsigned char t,
                 const unsigned char* ds, const unsigned char* dl,
                 const unsigned char* ss, const unsigned char* sl)
                : type(t)
                , del_start(ds)
                , del_limit(dl)
                , set_start(ss)
                , set_limit(sl)
            {
            }

            unsigned char type;
            const unsigned char* del_start;
            const unsigned char* del_limit;
            const unsigned char* set_start;
            const unsigned char* set_limit;
        };

        treadstone_transformer(const treadstone_transformer&);
        treadstone_transformer& operator = (const treadstone_transformer&);

        int parse(const treadstone::path& path, std::vector<stub>* stubs);
        int parse_value(const treadstone::path& path,
                        std::vector<stub>* stubs,
                        const unsigned char* del_start,
                        const unsigned char* del_limit,
                        const unsigned char* set_start,
                        const unsigned char* set_limit,
                        unsigned depth);
        int parse_object(const treadstone::path& path,
                        std::vector<stub>* stubs,
                        const unsigned char* del_start,
                        const unsigned char* del_limit,
                        const unsigned char* set_start,
                        const unsigned char* set_limit,
                        unsigned depth);
        int parse_array(const treadstone::path& path,
                        std::vector<stub>* stubs,
                        const unsigned char* del_start,
                        const unsigned char* del_limit,
                        const unsigned char* set_start,
                        const unsigned char* set_limit,
                        unsigned depth);
        int replace(const std::vector<stub>& stubs,
                    const unsigned char* cut_start,
                    const unsigned char* cut_limit,
                    const unsigned char* rep_with,
                    size_t rep_with_sz);
        int replace(const std::vector<stub>& stubs,
                    const unsigned char* cut_start,
                    const unsigned char* cut_limit,
                    const unsigned char** rep_withs,
                    size_t* rep_with_szs,
                    size_t reps);

        unsigned char* m_binary;
        size_t m_binary_sz;
        size_t m_binary_cap;
        bool m_error;
};

treadstone_transformer :: treadstone_transformer(const unsigned char* binary, size_t binary_sz)
    : m_binary()
    , m_binary_sz()
    , m_binary_cap()
    , m_error(false)
{
    m_binary = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char*) * binary_sz));
    m_binary_sz = binary_sz;
    m_binary_cap = binary_sz;
    m_error = m_binary == NULL;

    if (m_binary)
    {
        memmove(m_binary, binary, binary_sz);
    }
}

treadstone_transformer :: ~treadstone_transformer() throw ()
{
    if (m_binary)
    {
        free(m_binary);
    }
}

int
treadstone_transformer :: output(unsigned char** binary, size_t* binary_sz)
{
    *binary = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * m_binary_sz));

    if (!*binary)
    {
        return -1;
    }

    *binary_sz = m_binary_sz;
    memmove(*binary, m_binary, m_binary_sz);
    return 0;
}

int
treadstone_transformer :: unset_value(const char* p)
{
    treadstone::path path(p);

    if (!path.is_valid())
    {
        return -1;
    }

    std::vector<stub> stubs;

    if (parse(path, &stubs) < 0)
    {
        return -1;
    }

    // if we found exactly the field we wanted
    if (stubs.size() == path.depth() + 1)
    {
        return replace(stubs, stubs.back().del_start, stubs.back().del_limit, NULL, 0);
    }
    // we fell short
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: set_value(const treadstone::path& path, const unsigned char* value, size_t value_sz)
{
    using namespace treadstone;

    if (!path.is_valid())
    {
        return -1;
    }

    std::vector<stub> stubs;

    if (parse(path, &stubs) < 0)
    {
        return -1;
    }

    // overwrite the whole value
    if (path.depth() == 0)
    {
        return replace(stubs, m_binary, m_binary + m_binary_sz, value, value_sz);
    }
    // the item does not exist yet
    else if (stubs.size() == path.depth())
    {
        const path::component& c(path.get(path.depth() - 1));

        if (stubs.back().type == BINARY_OBJECT && c.type == path::FIELD)
        {
            unsigned char* binary;
            size_t binary_sz;
            e::guard g = e::makeguard(treadstone::free_if_allocated_unsigned_char_star, &binary);

            if (treadstone_string_to_binary(c.field.c_str(), c.field.size(), &binary, &binary_sz) < 0)
            {
                return -1;
            }

            const unsigned char* rep_withs[2];
            size_t rep_with_szs[2];
            rep_withs[0] = binary;
            rep_withs[1] = value;
            rep_with_szs[0] = binary_sz;
            rep_with_szs[1] = value_sz;
            return replace(stubs, stubs.back().del_limit, stubs.back().del_limit, rep_withs, rep_with_szs, 2);
        }
        // we don't support inserting into an array by index --- if it doesn't
        // already exist, use push/pop
        else
        {
            return -1;
        }
    }
    // overwrite existing value
    else if (stubs.size() == path.depth() + 1)
    {
        return replace(stubs, stubs.back().set_start, stubs.back().set_limit, value, value_sz);
    }
    // We first have to create the parent(s) of the field
    else if (stubs.size() < path.depth())
    {
        if(set_value(path.front(), empty_object, sizeof(empty_object)) == -1)
        {
            return -1;
        }
        else
        {
            return set_value(path, value, value_sz);
        }
    }
    // Something went wrong...
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: extract_value(const char* p,
                                        unsigned char** value, size_t* value_sz)
{
    treadstone::path path(p);

    if (!path.is_valid())
    {
        return -1;
    }

    std::vector<stub> stubs;

    if (parse(path, &stubs) < 0)
    {
        return -1;
    }

    // if we found exactly the field we wanted
    if (stubs.size() == path.depth() + 1)
    {
        *value_sz = stubs.back().set_limit - stubs.back().set_start;
        *value = reinterpret_cast<unsigned char*>(malloc(*value_sz));

        if (!*value)
        {
            return -1;
        }

        memmove(*value, stubs.back().set_start, *value_sz);
        return 0;
    }
    // we fell short
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: array_prepend_value(const char* p,
                                              const unsigned char* value, size_t value_sz)
{
    treadstone::path path(p);

    if (!path.is_valid())
    {
        return -1;
    }

    std::vector<stub> stubs;

    if (parse(path, &stubs) < 0)
    {
        return -1;
    }

    // if we found exactly the field we wanted
    if (stubs.size() == path.depth() + 1 && stubs.back().type == BINARY_ARRAY)
    {
        uint64_t arr_sz;
        const unsigned char* end = e::varint64_decode(stubs.back().set_start + 1, stubs.back().set_limit, &arr_sz);

        if (end == NULL || end + arr_sz != stubs.back().set_limit)
        {
            return -1;
        }

        unsigned char header[20];
        header[0] = BINARY_ARRAY;
        uint64_t body_sz = value_sz + arr_sz;
        size_t header_sz = e::packvarint64(body_sz, header + 1) - header;

        const unsigned char* rep_withs[3];
        size_t rep_with_szs[3];
        rep_withs[0] = header;
        rep_withs[1] = value;
        rep_withs[2] = end;
        rep_with_szs[0] = header_sz;
        rep_with_szs[1] = value_sz;
        rep_with_szs[2] = arr_sz;
        return replace(stubs, stubs.back().set_start, stubs.back().set_limit, rep_withs, rep_with_szs, 3);
    }
    // we fell short
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: array_append_value(const char* p,
                                             const unsigned char* value, size_t value_sz)
{
    treadstone::path path(p);

    if (!path.is_valid())
    {
        return -1;
    }

    std::vector<stub> stubs;

    if (parse(path, &stubs) < 0)
    {
        return -1;
    }

    // if we found exactly the field we wanted
    if (stubs.size() == path.depth() + 1 && stubs.back().type == BINARY_ARRAY)
    {
        uint64_t arr_sz;
        const unsigned char* end = e::varint64_decode(stubs.back().set_start + 1, stubs.back().set_limit, &arr_sz);

        if (end == NULL || end + arr_sz != stubs.back().set_limit)
        {
            return -1;
        }

        unsigned char header[20];
        header[0] = BINARY_ARRAY;
        uint64_t body_sz = value_sz + arr_sz;
        size_t header_sz = e::packvarint64(body_sz, header + 1) - header;

        const unsigned char* rep_withs[3];
        size_t rep_with_szs[3];
        rep_withs[0] = header;
        rep_withs[1] = end;
        rep_withs[2] = value;
        rep_with_szs[0] = header_sz;
        rep_with_szs[1] = arr_sz;
        rep_with_szs[2] = value_sz;
        return replace(stubs, stubs.back().set_start, stubs.back().set_limit, rep_withs, rep_with_szs, 3);
    }
    // we fell short
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: parse(const treadstone::path& path, std::vector<stub>* stubs)
{
    stubs->clear();
    unsigned char* start = m_binary;
    unsigned char* limit = m_binary + m_binary_sz;
    return parse_value(path, stubs, start, limit, start, limit, 0);
}

int
treadstone_transformer :: parse_value(const treadstone::path& path,
                                      std::vector<stub>* stubs,
                                      const unsigned char* del_start,
                                      const unsigned char* del_limit,
                                      const unsigned char* set_start,
                                      const unsigned char* set_limit,
                                      unsigned depth)
{
    if (set_start >= set_limit)
    {
        return -1;
    }

    stubs->push_back(stub(*set_start, del_start, del_limit, set_start, set_limit));

    if (path.depth() <= (size_t)depth)
    {
        return depth;
    }

    switch (*set_start)
    {
        case BINARY_OBJECT:
            return parse_object(path, stubs, del_start, del_limit, set_start, set_limit, depth);
        case BINARY_ARRAY:
            return parse_array(path, stubs, del_start, del_limit, set_start, set_limit, depth);
        case BINARY_STRING:
        case BINARY_DOUBLE:
        case BINARY_INTEGER:
        case BINARY_TRUE:
        case BINARY_FALSE:
        case BINARY_NULL:
            return depth;
        default:
            return -1;
    }

    return -1;
}

int
treadstone_transformer :: parse_object(const treadstone::path& path,
                                       std::vector<stub>* stubs,
                                       const unsigned char*,
                                       const unsigned char*,
                                       const unsigned char* set_start,
                                       const unsigned char* set_limit,
                                       unsigned depth)
{
    using namespace treadstone;
    assert(path.depth() > depth);
    const path::component& c = path.get(depth);

    if (c.type != path::FIELD)
    {
        return -1;
    }

    assert(*set_start == BINARY_OBJECT);
    assert(set_start < set_limit);
    uint64_t obj_sz;
    const unsigned char* end = e::varint64_decode(set_start + 1, set_limit, &obj_sz);

    if (end == NULL || end + obj_sz > set_limit)
    {
        return -1;
    }

    const unsigned char* tmp = end;
    end += obj_sz;
    assert(end <= set_limit);

    while (tmp < end)
    {
        if (*tmp != BINARY_STRING)
        {
            return -1;
        }

        uint64_t key_sz = 0;
        const unsigned char* key_sz_end = e::varint64_decode(tmp + 1, end, &key_sz);

        if (key_sz_end == NULL || key_sz_end + key_sz >= end)
        {
            return -1;
        }

        const unsigned char* const key_start = tmp;
        const unsigned char* const key_limit = key_sz_end + key_sz;
        assert(key_limit < end);
        const unsigned char* const val_start = key_limit;
        uint64_t val_sz;
        const unsigned char* val_tmp = NULL;

        switch (*val_start)
        {
            case BINARY_OBJECT:
            case BINARY_ARRAY:
            case BINARY_STRING:
                val_tmp = e::varint64_decode(val_start + 1, end, &val_sz);

                if (val_tmp == NULL || val_tmp + val_sz > end)
                {
                    return -1;
                }

                val_sz = val_tmp + val_sz - val_start;
                break;
            case BINARY_DOUBLE:
                val_sz = 9;
                break;
            case BINARY_INTEGER:
                val_tmp = e::varint64_decode(val_start + 1, end, &val_sz);

                if (val_tmp == NULL)
                {
                    return -1;
                }

                val_sz = val_tmp - val_start;
                break;
            case BINARY_TRUE:
            case BINARY_FALSE:
            case BINARY_NULL:
                val_sz = 1;
                break;
            default:
                return -1;
        }

        const unsigned char* const val_limit = val_start + val_sz;
        tmp = val_limit;

        if (c.field.size() == key_sz &&
            memcmp(c.field.data(), key_sz_end, key_sz) == 0)
        {
            return parse_value(path, stubs, key_start, val_limit, val_start, val_limit, depth + 1);
        }
    }

    return depth;
}

int
treadstone_transformer :: parse_array(const treadstone::path& path,
                                      std::vector<stub>* stubs,
                                      const unsigned char*,
                                      const unsigned char*,
                                      const unsigned char* set_start,
                                      const unsigned char* set_limit,
                                      unsigned depth)
{
    using namespace treadstone;
    assert(path.depth() > depth);
    const path::component& c = path.get(depth);

    if (c.type != path::INDEX)
    {
        return -1;
    }

    assert(*set_start == BINARY_ARRAY);
    assert(set_start < set_limit);
    uint64_t arr_sz;
    const unsigned char* end = e::varint64_decode(set_start + 1, set_limit, &arr_sz);

    if (end == NULL || end + arr_sz > set_limit)
    {
        return -1;
    }

    const unsigned char* tmp = end;
    end += arr_sz;
    assert(end <= set_limit);
    std::vector<stub> elements;

    while (tmp < end)
    {
        const unsigned char* const elem_start = tmp;
        const unsigned char* elem_tmp = NULL;
        size_t elem_sz;

        switch (*elem_start)
        {
            case BINARY_OBJECT:
            case BINARY_ARRAY:
            case BINARY_STRING:
                elem_tmp = e::varint64_decode(elem_start + 1, end, &elem_sz);

                if (elem_tmp == NULL || elem_tmp + elem_sz > end)
                {
                    return -1;
                }

                elem_sz = elem_tmp + elem_sz - elem_start;
                break;
            case BINARY_DOUBLE:
                elem_sz = 9;
                break;
            case BINARY_INTEGER:
                elem_tmp = e::varint64_decode(elem_start + 1, end, &elem_sz);

                if (elem_tmp == NULL)
                {
                    return -1;
                }

                elem_sz = elem_tmp - elem_start;
                break;
            case BINARY_TRUE:
            case BINARY_FALSE:
            case BINARY_NULL:
                elem_sz = 1;
                break;
            default:
                return -1;
        }

        const unsigned char* const elem_limit = elem_start + elem_sz;
        elements.push_back(stub(*elem_start, elem_start, elem_limit, elem_start, elem_limit));
        tmp = elem_limit;
    }

    const stub* e = NULL;

    if (c.index >= 0 && (size_t)c.index < elements.size())
    {
        e = &elements[c.index];
    }
    else if (c.index < 0 && (size_t)(0 - c.index) <= elements.size())
    {
        e = &elements[c.index + elements.size()];
    }

    if (e)
    {
        return parse_value(path, stubs, e->del_start, e->del_limit, e->set_start, e->set_limit, depth + 1);
    }
    else
    {
        return -1;
    }
}

int
treadstone_transformer :: replace(const std::vector<stub>& stubs,
                                  const unsigned char* cut_start,
                                  const unsigned char* cut_limit,
                                  const unsigned char* rep_with,
                                  size_t rep_with_sz)
{
    return replace(stubs, cut_start, cut_limit, &rep_with, &rep_with_sz, 1);
}

int
treadstone_transformer :: replace(const std::vector<stub>& stubs,
                                  const unsigned char* cut_start,
                                  const unsigned char* cut_limit,
                                  const unsigned char** rep_withs,
                                  size_t* rep_with_szs,
                                  size_t reps)
{
    size_t cumul_rep = 0;

    for (size_t i = 0; i < reps; ++i)
    {
        cumul_rep += rep_with_szs[i];
    }

    size_t new_binary_sz = m_binary_sz + cumul_rep + e::varint_length(cumul_rep) * (1 + stubs.size());
    size_t new_binary_cap = new_binary_sz;
    unsigned char* new_binary = NULL;
    e::guard g = e::makeguard(treadstone::free_if_allocated_unsigned_char_star, &new_binary);
    new_binary = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * new_binary_sz));

    if (!new_binary)
    {
        return -1;
    }

    int64_t diff = cumul_rep - (cut_limit - cut_start);

    // work backwards because inner varints may change in size, affecting the
    // outter varints
    unsigned char* out = new_binary + new_binary_sz;
    size_t remnants = m_binary + m_binary_sz - cut_limit;
    out -= remnants;
    memmove(out, cut_limit, remnants);

    for (size_t i = 0; i < reps; ++i)
    {
        size_t idx = reps - i - 1;
        out -= rep_with_szs[idx];
        memmove(out, rep_withs[idx], rep_with_szs[idx]);
    }

    const unsigned char* prev = cut_start;

    for (size_t i = 0; i < stubs.size(); ++i)
    {
        const stub& s(stubs[stubs.size() - i - 1]);
        assert(s.del_start <= s.set_start);
        assert(s.del_limit >= s.set_limit);
        assert(s.set_start < s.set_limit);

        if (s.set_start < prev)
        {
            const unsigned char* varint_end = NULL;
            uint64_t varint;
            varint_end = e::varint64_decode(s.set_start + 1, prev, &varint);

            if (varint_end == NULL || varint_end + varint != s.set_limit)
            {
                return -1;
            }

            out -= prev - varint_end;
            memmove(out, varint_end, prev - varint_end);
            unsigned char buf[12];
            size_t buf_sz = e::packvarint64(varint + diff, buf) - buf;
            out -= buf_sz;
            memmove(out, buf, buf_sz);
            --out;
            *out = *s.set_start;
            diff += buf_sz - (varint_end - (s.set_start + 1));
            prev = s.set_start;
        }
    }

    new_binary_sz = (new_binary + new_binary_cap) - out;
    memmove(new_binary, out, new_binary_sz);
    out = NULL;

    m_binary_sz = new_binary_sz;
    m_binary_cap = new_binary_cap;
    std::swap(m_binary, new_binary);

    if (m_binary_sz == 0)
    {
        if (!treadstone::j2b_make_room_for(2, &m_binary, &m_binary_sz, &m_binary_cap))
        {
            return -1;
        }

        memmove(m_binary, "\x40\x00", 2);
        m_binary_sz = 2;
    }

    return 0;
}

TREADSTONE_API struct treadstone_transformer*
treadstone_transformer_create(const unsigned char* binary, size_t binary_sz)
{
    return new (std::nothrow) treadstone_transformer(binary, binary_sz);
}

TREADSTONE_API void
treadstone_transformer_destroy(struct treadstone_transformer* trans)
{
    if (trans)
    {
        delete trans;
    }
}

TREADSTONE_API int
treadstone_transformer_output(struct treadstone_transformer* trans,
                              unsigned char** binary, size_t* binary_sz)
{
    return trans->output(binary, binary_sz);
}

TREADSTONE_API int
treadstone_transformer_unset_value(struct treadstone_transformer* trans,
                                   const char* path)
{
    return trans->unset_value(path);
}

TREADSTONE_API int
treadstone_transformer_set_value(struct treadstone_transformer* trans,
                                 const char* path,
                                 const unsigned char* value, size_t value_sz)
{
    return trans->set_value(path, value, value_sz);
}

TREADSTONE_API int
treadstone_transformer_extract_value(struct treadstone_transformer* trans,
                                     const char* path,
                                     unsigned char** value, size_t* value_sz)
{
    return trans->extract_value(path, value, value_sz);
}

TREADSTONE_API int
treadstone_transformer_array_prepend_value(struct treadstone_transformer* trans,
                                           const char* path,
                                           const unsigned char* value, size_t value_sz)
{
    return trans->array_prepend_value(path, value, value_sz);
}

TREADSTONE_API int
treadstone_transformer_array_append_value(struct treadstone_transformer* trans,
                                          const char* path,
                                          const unsigned char* value, size_t value_sz)
{
    return trans->array_append_value(path, value, value_sz);
}
