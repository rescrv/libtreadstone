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

// Treadstone
#include <treadstone.h>
#include "test/th.h"

// C
#include <string.h>

TEST(JsonToBinary, EmptyString)
{
    const char *json = "";
    char unsigned *binary = NULL;
    size_t binary_sz = 0;

    int res = treadstone_json_to_binary(json, &binary, &binary_sz);
    ASSERT_TRUE(binary == NULL);
    ASSERT_EQ(binary_sz, 0);
    ASSERT_NE(res, 0);
}

TEST(JsonToBinary, NullPointer)
{
    const char *json = NULL;
    char unsigned *binary = NULL;
    size_t binary_sz = 0;

    int res = treadstone_json_to_binary(json, &binary, &binary_sz);
    ASSERT_TRUE(binary == NULL);
    ASSERT_EQ(binary_sz, 0);
    ASSERT_NE(res, 0);
}

TEST(JsonToBinary, RandomString)
{
    const char *json = "banana";
    char unsigned *binary = NULL;
    size_t binary_sz = 0;

    int res = treadstone_json_to_binary(json, &binary, &binary_sz);
    ASSERT_TRUE(binary == NULL);
    ASSERT_EQ(binary_sz, 0);
    ASSERT_NE(res, 0);
}

TEST(JsonToBinary, EmptyJson)
{
    const char *json = "{}";
    char unsigned *binary = NULL;
    size_t binary_sz = 0;

    int res = treadstone_json_to_binary(json, &binary, &binary_sz);
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(binary != NULL);
    free(binary);
}

TEST(JsonToBinary, EncodeAndDecode)
{
    const char *json = "{}";
    char unsigned *binary = NULL;
    size_t binary_sz = 0;

    int res = treadstone_json_to_binary(json, &binary, &binary_sz);
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(binary != NULL);

    char *json2 = NULL;
    int res2 = treadstone_binary_to_json(binary, binary_sz, &json2);
    ASSERT_EQ(res2, 0);
    ASSERT_EQ(strcmp(json, json2), 0);

    free(binary);
    free(json2);
}
