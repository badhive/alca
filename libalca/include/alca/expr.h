/*
 * Copyright (c) 2025 pygrum.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
