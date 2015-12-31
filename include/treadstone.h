/* Copyright (c) 2014, Robert Escriva
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of libtreadstone nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef treadstone_h_
#define treadstone_h_

/* C */
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

int treadstone_json_to_binary(const char* json,
                              unsigned char** binary, size_t* binary_sz);
int treadstone_json_sz_to_binary(const char* json, size_t json_sz,
                                 unsigned char** binary, size_t* binary_sz);
int treadstone_binary_to_json(const unsigned char* binary, size_t binary_sz,
                              char** json);
int treadstone_binary_validate(const unsigned char* binary, size_t binary_sz);

int treadstone_string_to_binary(const char* string, size_t string_sz,
                                unsigned char** binary, size_t* binary_sz);
int treadstone_integer_to_binary(int64_t number,
                                 unsigned char** binary, size_t* binary_sz);
int treadstone_double_to_binary(double number,
                                unsigned char** binary, size_t* binary_sz);

int treadstone_binary_is_string(const unsigned char* binary, size_t binary_sz);
size_t treadstone_binary_string_bytes(const unsigned char* binary, size_t binary_sz);
void treadstone_binary_to_string(const unsigned char* binary, size_t binary_sz,
                                 char* string);
/* second two assert the first */

int treadstone_binary_is_integer(const unsigned char* binary, size_t binary_sz);
int64_t treadstone_binary_to_integer(const unsigned char* binary, size_t binary_sz);
/* latter asserts the former */

int treadstone_binary_is_double(const unsigned char* binary, size_t binary_sz);
double treadstone_binary_to_double(const unsigned char* binary, size_t binary_sz);
/* latter asserts the former */

int treadstone_validate_path(const char* path);

struct treadstone_transformer;

struct treadstone_transformer* treadstone_transformer_create(const unsigned char* binary, size_t binary_sz);
void treadstone_transformer_destroy(struct treadstone_transformer*);

int treadstone_transformer_output(struct treadstone_transformer*,
                                  unsigned char** binary, size_t* binary_sz);

int treadstone_transformer_unset_value(struct treadstone_transformer*,
                                       const char* path);
int treadstone_transformer_set_value(struct treadstone_transformer*,
                                     const char* path,
                                     const unsigned char* value, size_t value_sz);
int treadstone_transformer_extract_value(struct treadstone_transformer*,
                                         const char* path,
                                         unsigned char** value, size_t* value_sz);
int treadstone_transformer_array_prepend_value(struct treadstone_transformer*,
                                               const char* path,
                                               const unsigned char* value, size_t value_sz);
int treadstone_transformer_array_append_value(struct treadstone_transformer*,
                                              const char* path,
                                              const unsigned char* value, size_t value_sz);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* treadstone_h_ */
