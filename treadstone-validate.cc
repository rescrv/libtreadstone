// Copyright (c) 2015, Robert Escriva
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
#include "treadstone-types.h"
#include "visibility.h"

BEGIN_TREADSTONE_NAMESPACE

//! Same as bson2json but without writing the json string
bool
validate_transform(const unsigned char** ptr, const unsigned char* limit);
bool
validate_value(const unsigned char** ptr, const unsigned char* limit);
bool
validate_object(const unsigned char** ptr, const unsigned char* limit);
bool
validate_array(const unsigned char** ptr, const unsigned char* limit);
bool
validate_string(const unsigned char** ptr, const unsigned char* limit);
bool
validate_double(const unsigned char** ptr, const unsigned char* limit);
bool
validate_integer(const unsigned char** ptr, const unsigned char* limit);
bool
validate_true(const unsigned char** ptr, const unsigned char* limit);
bool
validate_false(const unsigned char** ptr, const unsigned char* limit);
bool
validate_null(const unsigned char** ptr, const unsigned char* limit);

bool
binary_validate(const unsigned char** ptr, const unsigned char* limit)
{
    return validate_value(ptr, limit) &&
           *ptr == limit;
}

bool
validate_value(const unsigned char** ptr, const unsigned char* limit)
{
    if (*ptr >= limit)
    {
        return false;
    }

    switch (**ptr)
    {
        case BINARY_OBJECT:
            return validate_object(ptr, limit);
        case BINARY_ARRAY:
            return validate_array(ptr, limit);
        case BINARY_STRING:
            return validate_string(ptr, limit);
        case BINARY_DOUBLE:
            return validate_double(ptr, limit);
        case BINARY_INTEGER:
            return validate_integer(ptr, limit);
        case BINARY_TRUE:
            return validate_true(ptr, limit);
        case BINARY_FALSE:
            return validate_false(ptr, limit);
        case BINARY_NULL:
            return validate_null(ptr, limit);
        default:
            return false;
    }
}

bool
validate_object(const unsigned char** ptr, const unsigned char* limit)
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


    while (*ptr < end)
    {
        if (!validate_string(ptr, end))
        {
            return false;
        }

        if (!validate_value(ptr, end))
        {
            return false;
        }
    }

    return *ptr == end;
}

bool
validate_array(const unsigned char** ptr, const unsigned char* limit)
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

    while (*ptr < end + sz)
    {
        if (!validate_value(ptr, end + sz))
        {
            return false;
        }
    }

    return *ptr == end + sz;
}

bool
validate_string(const unsigned char** ptr, const unsigned char* limit)
{
    if (*ptr >= limit || **ptr != BINARY_STRING)
    {
        return false;
    }

    uint64_t sz;
    const unsigned char* end = e::varint64_decode(*ptr + 1, limit, &sz);


    if(end == NULL || end + sz > limit)
    {
        return false;
    }

    *ptr = end + sz;
    return true;
}

bool
validate_double(const unsigned char** ptr, const unsigned char* limit)
{
    if (*ptr + sizeof(double) >= limit || **ptr != BINARY_DOUBLE)
    {
        return false;
    }

    double num;
    e::unpackdoublebe(*ptr + 1, &num);
    char buf[40];
    int sz = snprintf(buf, 40, "%g", num);

    if(sz >= 40 && sz <= 0)
    {
        return false;
    }

    *ptr += sizeof(unsigned char) + sizeof(double);
    return true;
}

bool
validate_integer(const unsigned char** ptr, const unsigned char* limit)
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

    if(sz >= 40 && sz <= 0)
    {
        return false;
    }

    *ptr = end;
    return true;
}

bool
validate_constant(const unsigned char** ptr, const unsigned char* limit, unsigned char c)
{
    if (*ptr >= limit || **ptr != c)
    {
        return false;
    }

    return true;
}

bool
validate_true(const unsigned char** ptr, const unsigned char* limit)
{
    return validate_constant(ptr, limit, BINARY_TRUE);
}

bool
validate_false(const unsigned char** ptr, const unsigned char* limit)
{
    return validate_constant(ptr, limit, BINARY_FALSE);
}

bool
validate_null(const unsigned char** ptr, const unsigned char* limit)
{
    return validate_constant(ptr, limit, BINARY_NULL);
}

END_TREADSTONE_NAMESPACE

TREADSTONE_API int
treadstone_binary_validate(const unsigned char* binary, size_t binary_sz)
{
    const unsigned char* ptr = binary;
    const unsigned char* limit = binary + binary_sz;
    bool ret = treadstone::binary_validate(&ptr, limit);

    if (ret)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
