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

#ifndef AC_EXPR_H
#define AC_EXPR_H

#include <alca/types.h>

ac_expr *ac_expr_new_binary(ac_expr *left, ac_token *op, ac_expr *right);

ac_expr *ac_expr_new_unary(ac_token *op, ac_expr *right);

ac_expr *ac_expr_new_grouping(ac_expr *expression);

ac_expr *ac_expr_new_literal(ac_token *value);

ac_expr *ac_expr_new_call(ac_expr *callee, ac_token *paren);

ac_expr *ac_expr_new_field(ac_expr *object, ac_token *field_name);

ac_expr *ac_expr_new_index(ac_expr *object, ac_expr *index, ac_token *bracket);

ac_expr *ac_expr_new_range(uint32_t match_type, uint32_t fixed, ac_token *ivar, ac_expr *start, ac_expr *end, ac_expr *cond);

void ac_expr_call_append_argument(ac_expr *call, ac_expr *argument);

ac_statement *ac_expr_new_rule(ac_token *name, ac_token *event, ac_expr *condition, int external, int private);

ac_statement *ac_expr_new_sequence(ac_token *name, uint32_t maxSpan);

ac_statement *ac_expr_new_import(ac_token *name);

void ac_expr_sequence_append_rule(ac_statement *seq, ac_statement *rule);

ac_ast *ac_expr_new_ast(const char *path);

void ac_expr_ast_add_stmt(ac_ast *program, ac_statement *statement);

void ac_expr_free_expression(ac_expr *expr);

void ac_expr_free_rule(ac_statement *rule);

void ac_expr_free_sequence(ac_statement *seq);

void ac_expr_free_import(ac_statement *import);

void ac_expr_free_ast(ac_ast *program);

#endif //AC_EXPR_H
