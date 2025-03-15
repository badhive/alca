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

#ifndef AC_LEXER_H
#define AC_LEXER_H

#include <alca/types.h>

ac_lexer *ac_lex_new(const char *source, const char *source_name, size_t source_size);

void ac_lex_set_silence_warnings(ac_lexer *lexer, int silence_warnings);

void ac_lex_free(ac_lexer *lexer);

ac_token **ac_lex_scan(ac_lexer *lexer);

int ac_lex_token_count(ac_lexer *lexer);

#endif //AC_LEXER_H
