// Copyright (c) 2014, Robert Escriva
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

// STL
#include <string>

// Treadstone
#include <treadstone.h>
#include "test/th.h"

treadstone_transformer*
json_to_transformer(const char* json)
{
    unsigned char* binary;
    size_t binary_sz;

    if (treadstone_json_to_binary(json, &binary, &binary_sz) < 0)
    {
        return NULL;
    }

    treadstone_transformer* trans = NULL;
    trans = treadstone_transformer_create(binary, binary_sz);
    free(binary);
    return trans;
}

std::string
transformer_dump(treadstone_transformer* trans)
{
    unsigned char* binary;
    size_t binary_sz;
    int ret = treadstone_transformer_output(trans, &binary, &binary_sz);
    ASSERT_EQ(ret, 0);

    char* json = NULL;
    ret = treadstone_binary_to_json(binary, binary_sz, &json);
    free(binary);
    ASSERT_EQ(ret, 0);

    std::string tmp(json);
    free(json);
    return tmp;
}

int
treadstone_transformer_set_value(treadstone_transformer* trans,
                                 const char* path,
                                 const char* value)
{
    unsigned char* binary;
    size_t binary_sz;

    if (treadstone_json_to_binary(value, &binary, &binary_sz) < 0)
    {
        return -1;
    }

    int ret = treadstone_transformer_set_value(trans, path, binary, binary_sz);
    free(binary);
    return ret;
}

int
treadstone_transformer_array_prepend_value(treadstone_transformer* trans,
                                           const char* path,
                                           const char* value)
{
    unsigned char* binary;
    size_t binary_sz;

    if (treadstone_json_to_binary(value, &binary, &binary_sz) < 0)
    {
        return -1;
    }

    int ret = treadstone_transformer_array_prepend_value(trans, path, binary, binary_sz);
    free(binary);
    return ret;
}

int
treadstone_transformer_array_append_value(treadstone_transformer* trans,
                                          const char* path,
                                          const char* value)
{
    unsigned char* binary;
    size_t binary_sz;

    if (treadstone_json_to_binary(value, &binary, &binary_sz) < 0)
    {
        return -1;
    }

    int ret = treadstone_transformer_array_append_value(trans, path, binary, binary_sz);
    free(binary);
    return ret;
}

std::string
treadstone_transformer_extract_value(treadstone_transformer* trans,
                                     const char* path)
{
    unsigned char* binary;
    size_t binary_sz;
    int ret = treadstone_transformer_extract_value(trans, path, &binary, &binary_sz);
    ASSERT_EQ(ret, 0);

    char* json = NULL;
    ret = treadstone_binary_to_json(binary, binary_sz, &json);
    free(binary);
    ASSERT_EQ(ret, 0);

    std::string tmp(json);
    free(json);
    return tmp;
}

TEST(Transforms, SetupTeardown)
{
    treadstone_transformer* trans = json_to_transformer("{}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetFields)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": {\"bar\": {\"baz\": 5}}}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz.quux"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{\"bar\":{\"baz\":5}}}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{\"bar\":{}}}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{\"bar\":{}}}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{}}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{}}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), 0);
    ASSERT_EQ(transformer_dump(trans), "{}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), -1);
    ASSERT_EQ(transformer_dump(trans), "{}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetFields2)
{
    treadstone_transformer* trans = json_to_transformer("{\"beforefoo\": 5, \"foo\": {\"bar\": {\"baz1\": \"abcde\", \"baz\": 5}, \"bar2\": true}, \"afterfoo\": 3.14}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz.quux"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"foo\":{\"bar\":{\"baz1\":\"abcde\",\"baz\":5},\"bar2\":true},\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"foo\":{\"bar\":{\"baz1\":\"abcde\"},\"bar2\":true},\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar.baz"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"foo\":{\"bar\":{\"baz1\":\"abcde\"},\"bar2\":true},\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"foo\":{\"bar2\":true},\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo.bar"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"foo\":{\"bar2\":true},\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"afterfoo\":3.14}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"beforefoo\":5,\"afterfoo\":3.14}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetTopLevelObject)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": {\"bar\": {\"baz\": 5}}, \"quux\": null}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"quux\":null}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"quux\":null}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetEmptyPath)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": {\"bar\": {\"baz\": 5}}, \"quux\": null}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(treadstone_transformer_unset_value(trans, ""), 0);
    ASSERT_EQ(transformer_dump(trans), "{}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, ""), 0);
    ASSERT_EQ(transformer_dump(trans), "{}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetIndices)
{
    treadstone_transformer* trans = json_to_transformer("[1, 2, [\"A\", \"B\", \"C\"], 4, 5]");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "[1,2,[\"A\",\"B\",\"C\"],4,5]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[2][1]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[1,2,[\"A\",\"C\"],4,5]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[2][-1]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[1,2,[\"A\"],4,5]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[0]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[2,[\"A\"],4,5]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[-1]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[2,[\"A\"],4]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[1][0]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[2,[],4]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[1][0]"), -1);
    ASSERT_EQ(transformer_dump(trans), "[2,[],4]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[1]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[2,4]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[-1]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[2]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[1]"), -1);
    ASSERT_EQ(transformer_dump(trans), "[2]");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "[0]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[]");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, UnsetFieldIndices)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": [1, 2, {\"bar\": 8}]}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,{\"bar\":8}]}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo[2].bar"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,{}]}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo[2].bar"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,{}]}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo[2]"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2]}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo[2]"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2]}");
    ASSERT_EQ(treadstone_transformer_unset_value(trans, "foo"), 0);
    ASSERT_EQ(transformer_dump(trans), "{}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, Set)
{
    treadstone_transformer* trans = json_to_transformer("{}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "", "[]"), 0);
    ASSERT_EQ(transformer_dump(trans), "[]");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "", "{}"), 0);
    ASSERT_EQ(transformer_dump(trans), "{}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo.bar", "{}"), -1);
    ASSERT_EQ(transformer_dump(trans), "{}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo", "{}"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{}}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo.bar", "{}"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{\"bar\":{}}}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo.bar.baz", "true"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{\"bar\":{\"baz\":true}}}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo", "null"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":null}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo", "[14]"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[14]}");
    ASSERT_EQ(treadstone_transformer_set_value(trans, "foo[0]", "3.14"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[3.14]}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, ListStarpend)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": []}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[]}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "5"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[5]}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "4"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[4,5]}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "3"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[3,4,5]}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "2"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[2,3,4,5]}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "1"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,3,4,5]}");
    ASSERT_EQ(treadstone_transformer_array_append_value(trans, "foo", "6"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,3,4,5,6]}");
    ASSERT_EQ(treadstone_transformer_array_append_value(trans, "foo", "7"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,3,4,5,6,7]}");
    ASSERT_EQ(treadstone_transformer_array_append_value(trans, "foo", "8"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,3,4,5,6,7,8]}");
    ASSERT_EQ(treadstone_transformer_array_append_value(trans, "foo", "9"), 0);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":[1,2,3,4,5,6,7,8,9]}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, ListStarpendArrayOnly)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": {}}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{}}");
    ASSERT_EQ(treadstone_transformer_array_prepend_value(trans, "foo", "5"), -1);
    ASSERT_EQ(treadstone_transformer_array_append_value(trans, "foo", "5"), -1);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":{}}");
    treadstone_transformer_destroy(trans);
}

TEST(Transforms, Extract)
{
    treadstone_transformer* trans = json_to_transformer("{\"foo\": 5}");
    ASSERT_TRUE(trans);
    ASSERT_EQ(transformer_dump(trans), "{\"foo\":5}");
    ASSERT_EQ(treadstone_transformer_extract_value(trans, ""), "{\"foo\":5}");
    ASSERT_EQ(treadstone_transformer_extract_value(trans, "foo"), "5");
}
