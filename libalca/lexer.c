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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <alca/errors.h>
#include <alca/utils.h>
#include <alca/lexer.h>

#include <pcre2.h>

#define lex_panic(lexer, ...) { \
    lex_error(lexer, __VA_ARGS__ ); \
    lexer->status = ERROR_UNSUCCESSFUL; \
    return; \
}

ac_lexer *ac_lex_new(const char *source, const char *source_name, size_t source_size)
{
    if (source_size == 0)
        return NULL;
    ac_lexer *lexer = ac_alloc(sizeof(ac_lexer));
    lexer->source = source;
    if (!source_name)
        lexer->source_name = "";
    else
        lexer->source_name = source_name;
    lexer->source_size = source_size;
    lexer->line = 1;
    return lexer;
}

void ac_lex_set_silence_warnings(ac_lexer *lexer, int silence_warnings)
{
    lexer->silence_warnings = silence_warnings;
}

void lex_free_tokens(ac_lexer *lexer)
{
    if (lexer->tokens)
    {
        for (int i = 0; i < lexer->token_count; i++)
        {
            ac_token *token = lexer->tokens[i];
            if (token->type == TOKEN_STRING ||
                token->type == TOKEN_NUMBER ||
                token->type == TOKEN_IDENTIFIER)
            {
                if (token->value)
                    free(token->value);
            }
            free(token);
        }
        free(lexer->tokens);
        lexer->tokens = NULL;
    }
}

void ac_lex_free(ac_lexer *lexer)
{
    lex_free_tokens(lexer);
    free(lexer);
}

void lex_error(ac_lexer *lexer, const char *message, ...)
{
    va_list args;
    va_start(args, message);
    int msg_len = vsnprintf(NULL, 0, message, args);
    char *msg = ac_alloc(msg_len + 1);
    vsnprintf(msg, msg_len + 1, message, args);
    va_end(args);

    int fullmsg_len = snprintf(NULL, 0, "Rule '%s', line %d: error: %s\n", lexer->source_name, lexer->line, msg);
    char *fullmsg = ac_alloc(fullmsg_len + 1);
    snprintf(fullmsg, fullmsg_len + 1, "Rule '%s', line %d: error: %s\n", lexer->source_name, lexer->line, msg);
    ac_free(msg);
    lexer->error_msg = fullmsg;
}

int lex_isEof(ac_lexer *lexer)
{
    return lexer->current >= lexer->source_size;
}

char lex_advance(ac_lexer *lexer)
{
    char c = lexer->source[lexer->current++]; // evaluates before incrementing
    if (c == '\n')
        lexer->line++;
    return c;
}

char lex_previous_char(ac_lexer *lexer)
{
    return lexer->source[lexer->current-2];
}

ac_token *lex_add_token(ac_lexer *lexer, ac_token_type token_type, void *token_value)
{
    lexer->token_count++;
    ac_token *token = ac_alloc(sizeof(ac_token));
    token->type = token_type;
    token->value = token_value;
    token->line = lexer->line;
    if (lexer->tokens == NULL)
        lexer->tokens = ac_alloc(lexer->token_count * sizeof(ac_token *));
    else
        lexer->tokens = ac_realloc(lexer->tokens, lexer->token_count * sizeof(ac_token *));
    lexer->tokens[lexer->token_count - 1] = token;
    return token;
}

char lex_peek(ac_lexer *lexer)
{
    if (lex_isEof(lexer))
    {
        return '\0';
    }
    return lexer->source[lexer->current];
}

ac_token_type lex_last_token_type(ac_lexer *lexer)
{
    if (lexer->tokens)
        return lexer->tokens[lexer->token_count-1]->type;
    return TOKEN_EOF;
}

// includes number chars since identifier starts with non-number before being valiated
int isValidIdentifier(char c)
{
    return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void lex_handleRegex(ac_lexer *lexer, char *regex)
{
    int pcre_flags = 0;
    char supported_modifiers[] = {'i', 0, 's', 0, 'x', 0, 'm', 0, 'n', 0};
    while (!lex_isEof(lexer))
    {
        int found = FALSE;
        char c = lex_peek(lexer);
        for (int i = 0; i < sizeof(supported_modifiers); i+=2)
        {
            if (c == supported_modifiers[i])
            {
                if (supported_modifiers[i+1] && !lexer->silence_warnings)
                    printf("lexer: warning (line %d): duplicate modifier '%c'\n", lexer->line, c);
                else
                {
                    switch (c)
                    {
                        case 'i': pcre_flags |= PCRE2_CASELESS; break;
                        case 's': pcre_flags |= PCRE2_DOTALL; break;
                        case 'm': pcre_flags |= PCRE2_MULTILINE; break;
                        case 'n': pcre_flags |= PCRE2_NO_AUTO_CAPTURE; break;
                        case 'x': pcre_flags |= PCRE2_EXTENDED; break;
                        default: ;
                    }
                    supported_modifiers[i+1] = 1;
                }
                lex_advance(lexer);
                found = TRUE;
                break;
            }
        }
        if (!found)
        {
            ac_token *token = lex_add_token(lexer, TOKEN_REGEX, regex);
            token->flags = pcre_flags;
            return;
        }
    }
}

void lex_handleString(ac_lexer *lexer, char first)
{
    int len = 0;
    char *str = NULL;
    while (!lex_isEof(lexer))
    {
        char c = lex_advance(lexer);
        if (c == '\n')
            lex_panic(lexer, "unterminated string literal");
        if (c == '\\' && first != '/') // we pass escape sequences in regex to the regex engine
        {
            c = lex_advance(lexer);
            switch (c)
            {
                case '\'':
                case '/':
                case '"':
                case '\\': break;
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case 'a': c = '\a'; break;
                case 'f': c = '\f'; break;
                case 'v': c = '\v'; break;
                case 'b': c = '\b'; break;
                default: lex_panic(lexer, "unknown escape sequence '\\%c'", c);
            }
        } else if (c == first)
        {
            // regex identifier
            if (c == '/')
                lex_handleRegex(lexer, str);
            else
                lex_add_token(lexer, TOKEN_STRING, str);
            break;
        }
        str = ac_str_extend(str, c);
        len++;
    }
}

void lex_handleInteger(ac_lexer *lexer, char first)
{
    // support hex numbers
    int base = 10;
    char *str = NULL;
    if (first == '0' && lex_peek(lexer) == 'x')
    {
        base = 16;
        lex_advance(lexer);
    } else
    {
        // count first character as part of the number if base 10 (decimal)
        str = ac_str_extend(str, first);
    }
    while (!lex_isEof(lexer))
    {
        char c = lex_peek(lexer); // only peek in case check fails, meaning it's another token type
        if ((c >= '0' && c <= '9') ||
            (base == 16 && (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        {
            lex_advance(lexer);
            str = ac_str_extend(str, c);
            continue;
        }
        break;
    }
    char *endptr;
    uint32_t num = strtoul(str, &endptr, base);
    if (num == LONG_MAX)
        lex_panic(lexer, "integer cannot exceed sizeof(long)");
    uint32_t *int_token = ac_alloc(sizeof(uint32_t));
    int_token[0] = num;
    lex_add_token(lexer, TOKEN_NUMBER, int_token);
}

void lex_handleIdentifier(ac_lexer *lexer, char first)
{
    // identifiers must start with a letter or underscore.
    if (!isValidIdentifier(first))
    {
        lex_panic(lexer, "invalid identifier");
    }
    char *str = ac_str_extend(NULL, first);
    while (!lex_isEof(lexer))
    {
        char c = lex_peek(lexer);
        if (isValidIdentifier(c))
        {
            lex_advance(lexer);
            str = ac_str_extend(str, c);
        } else
            break;
    }
    int free = TRUE;
    // keyword check (conditional and, or, not, as well as true, false)
    if (strcmp("and", str) == 0)
        lex_add_token(lexer, TOKEN_AND, NULL);
    else if (strcmp("or", str) == 0)
        lex_add_token(lexer, TOKEN_OR, NULL);
    else if (strcmp("not", str) == 0)
        lex_add_token(lexer, TOKEN_NOT, NULL);
    else if (strcmp("true", str) == 0)
        lex_add_token(lexer, TOKEN_TRUE, NULL);
    else if (strcmp("false", str) == 0)
        lex_add_token(lexer, TOKEN_FALSE, NULL);
    else if (strcmp("startswith", str) == 0)
        lex_add_token(lexer, TOKEN_STARTSWITH, NULL);
    else if (strcmp("istartswith", str) == 0)
        lex_add_token(lexer, TOKEN_ISTARTSWITH, NULL);
    else if (strcmp("endswith", str) == 0)
        lex_add_token(lexer, TOKEN_ENDSWITH, NULL);
    else if (strcmp("iendswith", str) == 0)
        lex_add_token(lexer, TOKEN_IENDSWITH, NULL);
    else if (strcmp("contains", str) == 0)
        lex_add_token(lexer, TOKEN_CONTAINS, NULL);
    else if (strcmp("icontains", str) == 0)
        lex_add_token(lexer, TOKEN_ICONTAINS, NULL);
    else if (strcmp("iequals", str) == 0)
        lex_add_token(lexer, TOKEN_IEQUALS, NULL);
    else if (strcmp("matches", str) == 0)
        lex_add_token(lexer, TOKEN_MATCHES, NULL);
    else if (strcmp("for", str) == 0)
        lex_add_token(lexer, TOKEN_FOR, NULL);
    else if (strcmp("any", str) == 0)
        lex_add_token(lexer, TOKEN_ANY, NULL);
    else if (strcmp("all", str) == 0)
        lex_add_token(lexer, TOKEN_ALL, NULL);
    else if (strcmp("in", str) == 0)
        lex_add_token(lexer, TOKEN_IN, NULL);

        // types (add as required)
    else if (strcmp("private", str) == 0)
        lex_add_token(lexer, TOKEN_PRIVATE, NULL);
    else if (strcmp("rule", str) == 0)
        lex_add_token(lexer, TOKEN_RULE, NULL);
    else if (strcmp("sequence", str) == 0)
        lex_add_token(lexer, TOKEN_SEQUENCE, NULL);
    else if (strcmp("event", str) == 0)
        lex_add_token(lexer, TOKEN_IMPORT, NULL);
    else
    {
        free = FALSE;
        lex_add_token(lexer, TOKEN_IDENTIFIER, str);
    }
    if (free) ac_free(str);
}

void lex_handleComment(ac_lexer *lexer, char next)
{
    while (!lex_isEof(lexer))
    {
        char c = lex_advance(lexer);
        // wait for single-line comment to end
        if (next == '/')
        {
            if (c == '\n')
                return;
        } else if (next == '*')
        {
            if (c == '*' && lex_peek(lexer) == '/')
            {
                lex_advance(lexer);
                return;
            }
        }
    }
    lex_panic(lexer, "unterminated multi-line comment");
}

#define isascii(c) (c < 0x80)

void lex_scanToken(ac_lexer *lexer)
{
    char peek;
    char c = lex_advance(lexer);
    if (!isascii(c) && !lexer->silence_warnings)
    {
        printf("lexer: warning (line %d): encountered non-ascii character 0x%.2x\n", lexer->line, c);
    }
    switch (c)
    {
        case '(': lex_add_token(lexer, TOKEN_LPAREN, NULL);
            break;
        case ')': lex_add_token(lexer, TOKEN_RPAREN, NULL);
            break;
        case '[': lex_add_token(lexer, TOKEN_LBRACKET, NULL);
            break;
        case ']': lex_add_token(lexer, TOKEN_RBRACKET, NULL);
            break;
        case '{': lex_add_token(lexer, TOKEN_LBRACE, NULL);
            break;
        case '}': lex_add_token(lexer, TOKEN_RBRACE, NULL);
            break;
        case ',': lex_add_token(lexer, TOKEN_COMMA, NULL);
            break;
        case '+': lex_add_token(lexer, TOKEN_PLUS, NULL);
            break;
        case '-': lex_add_token(lexer, TOKEN_MINUS, NULL);
            break;
        case '*': lex_add_token(lexer, TOKEN_MULT, NULL);
            break;
        case '%': lex_add_token(lexer, TOKEN_MOD, NULL);
            break;
        case '|': lex_add_token(lexer, TOKEN_PIPE, NULL);
            break;
        case '&': lex_add_token(lexer, TOKEN_BITAND, NULL);
            break;
        case '~': lex_add_token(lexer, TOKEN_BITNOT, NULL);
            break;
        case '^': lex_add_token(lexer, TOKEN_BITXOR, NULL);
            break;
        case ':': lex_add_token(lexer, TOKEN_COLON, NULL);
            break;
        case '#': lex_add_token(lexer, TOKEN_HASH, NULL);
            break;
        case '\\':
        // more complex cases
        // comparisons (one or two characters)
        case '.':
            if (lex_peek(lexer) == '.')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_DOT_DOT, NULL);
            } else
                lex_add_token(lexer, TOKEN_DOT, NULL);
            break;
        case '!':
            if (lex_peek(lexer) == '=')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_BANG_EQUAL, NULL);
            } else
                lex_add_token(lexer, TOKEN_BANG, NULL);
            break;
        case '/':
            peek = lex_peek(lexer);
            if (peek == '/' || peek == '*')
            {
                lex_advance(lexer);
                lex_handleComment(lexer, peek);
            } else if (lex_last_token_type(lexer) == TOKEN_MATCHES)
            {
                lex_handleString(lexer, c); // cascades to handle regex
            } else
                lex_add_token(lexer, TOKEN_DIV, NULL);
            break;
        case '=':
            if (lex_peek(lexer) == '=')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_EQUAL_EQUAL, NULL);
            } else
                lex_add_token(lexer, TOKEN_EQUAL, NULL);
            break;
        case '>':
            if (lex_peek(lexer) == '=')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_GREATER_EQUAL, NULL);
            } else if (lex_peek(lexer) == '>')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_SHR, NULL);
            } else
                lex_add_token(lexer, TOKEN_GREATER, NULL);
            break;
        case '<':
            if (lex_peek(lexer) == '=')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_LESS_EQUAL, NULL);
            } else if (lex_peek(lexer) == '<')
            {
                lex_advance(lexer);
                lex_add_token(lexer, TOKEN_SHL, NULL);
            } else
                lex_add_token(lexer, TOKEN_LESS, NULL);
            break;

        // string (leave numbers, identifiers and keywords last)
        case '`':
        case '"':
        case '\'': lex_handleString(lexer, c);
            break;
        default:
            if (c >= '0' && c <= '9')
                lex_handleInteger(lexer, c);
            else if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) // identifier start with these
                lex_handleIdentifier(lexer, c); // treat keywords as identifiers until they match a keyword
            break;
    }
    if (lexer->status != ERROR_SUCCESS)
        return;
    if (lex_isEof(lexer))
        lex_add_token(lexer, TOKEN_EOF, NULL);
}

ac_token **ac_lex_scan(ac_lexer *lexer)
{
    while (!lex_isEof(lexer))
    {
        lex_scanToken(lexer);
        if (lexer->status != ERROR_SUCCESS)
            return NULL;
    }
    return lexer->tokens;
}

int ac_lex_token_count(ac_lexer *lexer)
{
    if (!lexer)
        return -1;
    return lexer->token_count;
}
