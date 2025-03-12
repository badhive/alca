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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alca/lexer.h>
#include <alca/types.h>
#include <alca/utils.h>

#include "test.h"

void validate(ac_test *t,
    ac_token **tokens,
    int token_count,
    int expected_count,
    ac_token_type *expected_types,
    void **expected_values)
{
    char *msg = ac_alloc(256);
    if (token_count != expected_count)
    {
        snprintf(msg, 256, "Expected %d tokens, got %d", expected_count, token_count);
        ac_test_error(t, msg);
    }
    for (int i = 0; i < token_count; i++)
    {
        ac_token *token = tokens[i];
        ac_test_assert_int32_eq(t, token->type, expected_types[i]);
        if (token->type == TOKEN_NUMBER)
        {
            ac_test_assert_int32_eq(t, *(uint32_t*)(token->value), *(uint32_t*)(expected_values[i]));
        }
    }
    ac_free(msg);
}

void test_lexer_scan_basic(ac_test *t)
{
    const char *source = "0xdeadbeef & 0 == 0 and (5 + 5 > 5 or 5 * (5 + 5) != 5)";
    // create struct values
    unsigned long a = 0xdeadbeef;
    long b = 0;
    long c = 5;
    ac_token_type expected_types[] = {
        TOKEN_NUMBER, TOKEN_BITAND, TOKEN_NUMBER,
        TOKEN_EQUAL_EQUAL, TOKEN_NUMBER, TOKEN_AND,
        TOKEN_LPAREN, TOKEN_NUMBER, TOKEN_PLUS,
        TOKEN_NUMBER, TOKEN_GREATER, TOKEN_NUMBER,
        TOKEN_OR, TOKEN_NUMBER, TOKEN_MULT,
        TOKEN_LPAREN, TOKEN_NUMBER, TOKEN_PLUS,
        TOKEN_NUMBER, TOKEN_RPAREN, TOKEN_BANG_EQUAL,
        TOKEN_NUMBER, TOKEN_RPAREN, TOKEN_EOF
    };
    void *expected_values[] = {
        &a, 0, &b, 0, &b, 0, 0, &c, 0, &c, 0, &c, 0, &c, 0, 0, &c, 0, &c, 0, 0, &c, 0, 0
    };
    ac_lexer *lexer = ac_lex_new(source, "rule.raw", strlen(source));
    ac_token **tokens = ac_lex_scan(lexer);

    int token_count = ac_lex_token_count(lexer);

    validate(t, tokens, token_count, lengthof(expected_types), expected_types, expected_values);
    ac_lex_free(lexer);
}

void test_lexer_scan_rule(ac_test *t)
{
    const char *source = "rule myrule { |process| where process.X64 and process.command_line[0] == \"test\" }";
    ac_token_type expected_types[] = {
        TOKEN_RULE, TOKEN_IDENTIFIER, TOKEN_LBRACE, TOKEN_PIPE, TOKEN_IDENTIFIER,
        TOKEN_PIPE, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_DOT, TOKEN_IDENTIFIER,
        TOKEN_AND, TOKEN_IDENTIFIER, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_LBRACKET,
        TOKEN_NUMBER, TOKEN_RBRACKET, TOKEN_EQUAL_EQUAL, TOKEN_STRING,
        TOKEN_RBRACE, TOKEN_EOF
    };
    int n = 0;
    void *expected_values[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &n, 0, 0, "test", 0, 0
    };
    ac_lexer *lexer = ac_lex_new(source, "rule.raw", strlen(source));
    ac_token **tokens = ac_lex_scan(lexer);
    int token_count = ac_lex_token_count(lexer);
    validate(t, tokens, token_count, lengthof(expected_types), expected_types, expected_values);
    ac_lex_free(lexer);
}

void test_lexer_scan_sequence(ac_test *t)
{
    const char *source = "sequence myseq [ {|process| where process.X64}, myrule ]";
    ac_token_type expected_types[] = {
        TOKEN_SEQUENCE, TOKEN_IDENTIFIER, TOKEN_LBRACKET, TOKEN_LBRACE,
        TOKEN_PIPE, TOKEN_IDENTIFIER, TOKEN_PIPE, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER,
        TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_RBRACE, TOKEN_COMMA, TOKEN_IDENTIFIER,
        TOKEN_RBRACKET, TOKEN_EOF
    };
    void *expected_values[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    ac_lexer *lexer = ac_lex_new(source, "rule.raw", strlen(source));
    ac_token **tokens = ac_lex_scan(lexer);

    int token_count = ac_lex_token_count(lexer);

    validate(t, tokens, token_count, lengthof(expected_types), expected_types, expected_values);
    ac_lex_free(lexer);
}

int main()
{
    ac_tester *tester = ac_test_init();
    ac_test_add_test(tester, "lexer_scan_basic", test_lexer_scan_basic);
    ac_test_add_test(tester, "lexer_scan_rule", test_lexer_scan_rule);
    ac_test_add_test(tester, "lexer_scan_sequence", test_lexer_scan_sequence);
    ac_test_runall(tester);
    ac_test_destroy(tester);
}
