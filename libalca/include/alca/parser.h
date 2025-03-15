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

#ifndef AC_PARSER_H
#define AC_PARSER_H

#include <alca/types.h>

char *ac_psr_get_last_error_message(ac_parser *parser);

ac_error ac_psr_get_last_error(ac_parser *parser);

/** Create a new parser. The lexer maintains ownership of the tokens,
 * which will not be freed until the lexer is freed.
 *
 * @param lexer lexer that scanned the source file
 * @return parser object
 */
ac_parser *ac_psr_new(ac_lexer *lexer);

/** Parses the program and constructs the AST.
 *
 * @param parser parser object
 * @return program AST
 */
ac_ast *ac_psr_parse(ac_parser *parser);

void ac_psr_free(ac_parser *parser);

#endif //AC_PARSER_H
