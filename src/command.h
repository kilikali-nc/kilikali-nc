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

#ifndef _KK_COMMAND_
#define _KK_COMMAND_

#include <glib.h>

#include "cmdline-mode.h"

typedef struct Command Command;

#define COMMAND_MAX_ARGS 32

typedef enum {
    COMMAND_ERROR_NONE = 0,
    COMMAND_ERROR_NO_SUCH_COMMAND = -1,
    COMMAND_ERROR_COMMAND_FAILED = -2
} CommandError;

typedef enum {
    COMMAND_HINT_NONE,
    COMMAND_HINT_RANGE,
    COMMAND_HINT_PATH,
    COMMAND_HINT_DIR
} CommandHint;

struct Command {
    const char *name;
    const char *description;
    const CommandHint hint;
    const CmdlineMode modes;
    int (*callback)(int argc, char **argv);
};

int command_init (void);

void command_destroy (void);

/* Register a command. The allocation of the Command structure must be static
 * and unchanging after registration!
 * Return value is 0 on success. Will fail if a command with the same name
 * already exists.
 *
 * Example:
 *
 * static Command myCommand =
 * {
 *     .name = "my_command",
 *     .description = "do_something",
 *     .callback = my_callback,
 *     .next = 0
 * };
 *
 * if (command_register(&myCommand) != 0) {
 *     ... error ...
 * }
 * */
int command_register (Command *command);

CommandError command_parse_and_run_string (const char *buf);

GSList *command_commands (void);

#endif
