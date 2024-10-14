/*
 * Copyright (C) 2024 kilikali-nc team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 */

#ifndef _KK_LOG_
#define _KK_LOG_

#include <stdio.h>

#define LOG(str, ...) \
    (void)(log_file ? (fprintf(log_file ? log_file : stdout, "[INFO] %s: " str "\n", \
        __func__, ##__VA_ARGS__), fflush(log_file)) : 0)

#define LOG_ERROR(str, ...) \
    (void)(log_file ? (fprintf(log_file ? log_file : stdout, "[ERROR] %s: " str "\n", \
        __func__, ##__VA_ARGS__), fflush(log_file)) : 0)

#define LOG_DEBUG(str, ...) \
    (void)((log_debugging_on && log_file) ? (fprintf(log_file ? log_file : stdout, "[DEBUG] %s: " str "\n", \
        __func__, ##__VA_ARGS__), fflush(log_file)) : 0)

extern FILE *log_file;
extern int log_debugging_on;

/* File path can be null. */
int log_init (const char *file_path);

void log_destroy (void);

#endif
