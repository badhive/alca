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

#ifndef AC_CHECKER_H
#define AC_CHECKER_H

#include <alca/context.h>

typedef struct ac_checker ac_checker;

ac_checker *ac_checker_new(ac_ast *ast, ac_context *ctx);

void ac_checker_free(ac_checker *checker);

int ac_checker_check(ac_checker *checker);

int ac_checker_iter_errors(ac_checker *checker, int *line, ac_error *code, char **errmsg);

#endif //AC_CHECKER_H
