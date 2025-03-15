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

#ifndef AC_VM_H
#define AC_VM_H

typedef struct ac_vm ac_vm;

#include <time.h>

#include <alca/compiler.h>

// trigger codes
#define AC_VM_RULE 0
#define AC_VM_SEQUENCE 1

typedef void (*ac_trigger_callback)(int type, char *name, time_t at, void *exec_context);

AC_API ac_vm *ac_vm_new(ac_compiler *compiler);

AC_API int ac_vm_add_trigger_callback(ac_vm *vm, ac_trigger_callback cb);

AC_API ac_error ac_vm_exec(ac_vm *vm, unsigned char *event, uint32_t esize, void *exec_context);

AC_API int ac_vm_get_trigger_count(ac_vm *vm);

#endif //AC_VM_H
