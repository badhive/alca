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

#ifndef AC_ERRORS_H
#define AC_ERRORS_H

typedef int ac_error;

// ac_error codes
#define AC_ERROR_SUCCESS 0
#define AC_ERROR_UNSUCCESSFUL 1
#define AC_ERROR_INVALID_SYNTAX 2
#define AC_ERROR_UNEXPECTED_TOKEN 3
#define AC_ERROR_UNTERMINATED_EXPRESSION 4

#define AC_ERROR_FIELD_ACCESS 5
#define AC_ERROR_BAD_LITERAL 6
#define AC_ERROR_NOT_SUBSCRIPTABLE 7
#define AC_ERROR_BAD_CALL 8
#define AC_ERROR_UNKNOWN_FIELD 9
#define AC_ERROR_UNKNOWN_IDENTIFIER 10
#define AC_ERROR_RECURSION 11
#define AC_ERROR_BAD_OPERATION 12
#define AC_ERROR_UNEXPECTED_TYPE 13
#define AC_ERROR_MODULE 14
#define AC_ERROR_REDEFINED 15

#define AC_ERROR_COMPILER_FILE 16
#define AC_ERROR_COMPILER_LOCKED 17
#define AC_ERROR_COMPILER_EXPORT 18
#define AC_ERROR_COMPILER_DONE 19
#define AC_ERROR_COMPILER_NO_SOURCE 20
#define AC_ERROR_NOT_PARSED 21

// runtime errors
#define AC_ERROR_BAD_DATA 22
#define AC_ERROR_MODULE_VERSION 23
#define AC_ERROR_STACK_OVERFLOW 24
#define AC_ERROR_BAD_OPERAND 25
#define AC_ERROR_INDEX 26
#define AC_ERROR_OPERATION 27
#define AC_ERROR_MAX_CALLS 28

#endif //AC_ERRORS_H
