/*
Copyright (c) 2025, pygrum. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef AC_ERRORS_H
#define AC_ERRORS_H

typedef int ac_error;

// ac_error codes
#define ERROR_SUCCESS 0
#define ERROR_UNSUCCESSFUL 1
#define ERROR_INVALID_SYNTAX 2
#define ERROR_UNEXPECTED_TOKEN 3
#define ERROR_UNTERMINATED_EXPRESSION 4

#define ERROR_FIELD_ACCESS 5
#define ERROR_BAD_LITERAL 6
#define ERROR_NOT_SUBSCRIPTABLE 7
#define ERROR_BAD_CALL 8
#define ERROR_UNKNOWN_FIELD 9
#define ERROR_UNKNOWN_IDENTIFIER 10
#define ERROR_RECURSION 11
#define ERROR_BAD_OPERATION 12
#define ERROR_UNEXPECTED_TYPE 13
#define ERROR_MODULE 14
#define ERROR_REDEFINED 15

#define ERROR_COMPILER_FILE 16
#define ERROR_COMPILER_LOCKED 17
#define ERROR_COMPILER_EXPORT 18
#define ERROR_COMPILER_DONE 19
#define ERROR_COMPILER_NO_SOURCE 20
#define ERROR_NOT_PARSED 21

// runtime errors
#define ERROR_BAD_DATA 22
#define ERROR_MODULE_VERSION 23
#define ERROR_STACK_OVERFLOW 24
#define ERROR_BAD_OPERAND 25
#define ERROR_INDEX 26
#define ERROR_OPERATION 27
#define ERROR_MAX_CALLS 28

#endif //AC_ERRORS_H
