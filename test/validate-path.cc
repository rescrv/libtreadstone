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

// Treadstone
#include <treadstone.h>
#include "test/th.h"

TEST(ValidatePath, SimpleFields)
{
    ASSERT_EQ(treadstone_validate_path(""), 0);
    ASSERT_EQ(treadstone_validate_path("foo"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar.baz"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar.baz.quux"), 0);
}

TEST(ValidatePath, SimpleIndices)
{
    ASSERT_EQ(treadstone_validate_path(""), 0);
    ASSERT_EQ(treadstone_validate_path("[5]"), 0);
    ASSERT_EQ(treadstone_validate_path("foo[5]"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar[5]"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar.baz[5]"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar.baz.quux[5]"), 0);
    ASSERT_EQ(treadstone_validate_path("[5].foo"), 0);
    ASSERT_EQ(treadstone_validate_path("foo[5].bar"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar[5].baz"), 0);
    ASSERT_EQ(treadstone_validate_path("foo.bar.baz[5].quux"), 0);
}

TEST(ValidatePath, MultiDigitIndices)
{
    ASSERT_EQ(treadstone_validate_path("[12345]"), 0);
}

TEST(ValidatePath, MultiDimArray)
{
    ASSERT_EQ(treadstone_validate_path("foo[3][14]"), 0);
}

TEST(ValidatePath, BadPaths)
{
    ASSERT_NE(treadstone_validate_path("foo..bar"), 0);
    ASSERT_NE(treadstone_validate_path("foo.[3]"), 0);
    ASSERT_NE(treadstone_validate_path("foo.[3]bar"), 0);
    ASSERT_NE(treadstone_validate_path("foo.[3].bar"), 0);
    ASSERT_NE(treadstone_validate_path("foo[3]bar"), 0);
}
