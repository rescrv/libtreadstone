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

// C
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX
#include <errno.h>

// e
#include <e/guard.h>

// Treadstone
#include <treadstone.h>

int
main(int argc, const char* argv[])
{
    while (true)
    {
        char* line = NULL;
        size_t line_sz = 0;
        ssize_t amt = getline(&line, &line_sz, stdin);

        if (amt < 0)
        {
            if (feof(stdin) != 0)
            {
                break;
            }

            fprintf(stderr, "could not read from stdin: %s\n", strerror(ferror(stdin)));
            return EXIT_FAILURE;
        }

        if (!line)
        {
            continue;
        }

        e::guard line_guard = e::makeguard(free, line);
        (void) line_guard;

        if (amt < 1)
        {
            continue;
        }

        line[amt - 1] = '\0';
        unsigned char* binary1 = NULL;
        size_t binary1_sz = 0;

        if (treadstone_json_to_binary(line, &binary1, &binary1_sz) < 0)
        {
            printf("failure on binary1 conversion\n");
            continue;
        }

        assert(binary1);
        e::guard binary1_guard = e::makeguard(free, binary1);
        char* json1 = NULL;

        if (treadstone_binary_to_json(binary1, binary1_sz, &json1) < 0)
        {
            printf("failure on json1 conversion\n");
            continue;
        }

        assert(json1);
        e::guard json1_guard = e::makeguard(free, json1);
        unsigned char* binary2 = NULL;
        size_t binary2_sz = 0;

        if (treadstone_json_to_binary(json1, &binary2, &binary2_sz) < 0)
        {
            printf("failure on binary2 conversion\n");
            continue;
        }

        assert(binary2);
        e::guard binary2_guard = e::makeguard(free, binary2);
        char* json2 = NULL;

        if (treadstone_binary_to_json(binary2, binary2_sz, &json2) < 0)
        {
            printf("failure on json2 conversion\n");
            continue;
        }

        assert(json2);
        e::guard json2_guard = e::makeguard(free, json2);
        unsigned char* binary3 = NULL;
        size_t binary3_sz = 0;

        if (treadstone_json_to_binary(json1, &binary3, &binary3_sz) < 0)
        {
            printf("failure on binary3 conversion\n");
            continue;
        }

        assert(binary3);
        e::guard binary3_guard = e::makeguard(free, binary3);
        char* json3 = NULL;

        if (treadstone_binary_to_json(binary3, binary3_sz, &json3) < 0)
        {
            printf("failure on json3 conversion\n");
            continue;
        }

        assert(json3);
        e::guard json3_guard = e::makeguard(free, json3);

        bool json_same = strcmp(json1, json2) == 0 &&
                         strcmp(json2, json3) == 0;
        bool binary_same = binary2_sz == binary3_sz &&
                           memcmp(binary2, binary3, binary2_sz) == 0;

        if (!json_same || !binary_same)
        {
            printf("json_same=%s binary_same=%s\n\t%s\n",
                   (json_same ? "yes" : "no"),
                   (binary_same ? "yes" : "no"),
                   line);
        }
    }

    (void) argc;
    (void) argv;
    return EXIT_SUCCESS;
}
