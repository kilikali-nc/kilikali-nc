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

#include "command.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

typedef enum {
    QUOTE_TYPE_NONE,
    QUOTE_TYPE_SINGLE,
    QUOTE_TYPE_DOUBLE
} QuoteType;

typedef struct {
    char *data;
    size_t size; /* Includes null-terminator */
    size_t max_size; /* Includes null-terminator */
} ArgString;

static ArgString _arg_strings[COMMAND_MAX_ARGS];

static GSList *_commands = NULL; /* Linked list of statically allocated commands. */

static Command *_find_command (const char *name) {
    Command *retcmd = 0;
    gint found = 0;
    for (gint i = 0; i < g_slist_length (_commands); i++) {
        Command *command = (Command *)g_slist_nth_data (_commands, i);
        if (strncmp(command->name, name, strlen (name)) == 0) {
            retcmd = command;
            found++;
        }
    }
    if (found == 1) return retcmd;
    return 0;
}

static int _add_char_to_arg_string (ArgString *arg_string, char c) {
    if (arg_string->size == arg_string->max_size) {
        size_t new_max_size = arg_string->max_size * 2;
        assert (new_max_size >= arg_string->size + 1);
        char *new_str = realloc (arg_string->data, new_max_size);
        if (!new_str) {
            return -1;
        }
        arg_string->data = new_str;
        arg_string->max_size = new_max_size;
    }
    arg_string->data[arg_string->size++] = c;
    return 0;
}

int command_init (void) {
    int num_allocated = 0;
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i) {
        size_t size = 64;
        _arg_strings[i].data = malloc (size);
        if (!_arg_strings[i].data) {
            for (int j = 0; j < num_allocated; ++j) {
                free(_arg_strings[j].data);
                _arg_strings[j].data = 0;
                return -1;
            }
        }
        _arg_strings[i].max_size = size;
        num_allocated++;
    }
    return 0;
}

void command_destroy (void) {
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i) {
        free(_arg_strings[i].data);
    }
    if (_commands != NULL) g_slist_free (_commands); /* data is not allocated dynamically -> no freeing for data */
    _commands = NULL;
    memset(_arg_strings, 0, sizeof(_arg_strings));
}

int command_register (Command *command) {
    GSList *other_command = _commands;
    while (other_command) {
        Command *cmd = (Command *)other_command->data;
        if (strcmp (cmd->name, command->name) == 0) {
            return -1;
        }
        other_command = other_command->next;
    }
    _commands = g_slist_prepend (_commands, command);
    return 0;
}

CommandError command_parse_and_run_string (const char *buf) {
    for (int i = 0; i < COMMAND_MAX_ARGS; ++i) {
        _arg_strings[i].size = 0;
    }
    const char *c = buf;
    QuoteType quote_type = QUOTE_TYPE_NONE;
    int have_non_whitespace = 0;
    int last_is_space = 0;
    int escaped = 0;
    int argc = 0;
    for (;;) {
        if (quote_type == QUOTE_TYPE_NONE) {
            if (escaped == 0 && isspace(*c)) {
                if (!last_is_space) {
                    /* End of a word */
                    if (have_non_whitespace) {
                        _add_char_to_arg_string (&_arg_strings[argc], 0);
                        argc++;
                    }
                }
                last_is_space = 1;
            } else {
                if (last_is_space && argc == COMMAND_MAX_ARGS) {
                    return COMMAND_ERROR_COMMAND_FAILED;
                }
                if (!escaped) {
                    if (*c == '\\') {
                        escaped = 1;
                    } else if (*c == '\'') {
                        quote_type = QUOTE_TYPE_SINGLE;
                    } else if (*c == '\"') {
                        quote_type = QUOTE_TYPE_DOUBLE;
                    } else {
                        if (argc == COMMAND_MAX_ARGS) {
                            return COMMAND_ERROR_COMMAND_FAILED;
                        }
                        _add_char_to_arg_string (&_arg_strings[argc], *c);
                        have_non_whitespace = 1;
                    }
                } else {
                    if (argc == COMMAND_MAX_ARGS) {
                        return COMMAND_ERROR_COMMAND_FAILED;
                    }
                    _add_char_to_arg_string (&_arg_strings[argc], *c);
                    have_non_whitespace = 1;
                    escaped = 0;
                }
                last_is_space = 0;
            }
        } else { /* Being quoted */
            if (!escaped) {
                int stop_quoting = 0;
                if (quote_type == QUOTE_TYPE_SINGLE) {
                    if (*c == '\'') {
                        stop_quoting = 1;
                    }
                } else /* quote_type == QUOTE_TYPE_DOUBLE */ {
                    if (*c == '\"') {
                        stop_quoting = 1;
                    }
                }
                if (stop_quoting) {
                    quote_type = QUOTE_TYPE_NONE;
                } else {
                    if (argc == COMMAND_MAX_ARGS) {
                        return COMMAND_ERROR_COMMAND_FAILED;
                    }
                    _add_char_to_arg_string (&_arg_strings[argc], *c);
                    have_non_whitespace = 1;
                }
            } else {
                escaped = 0;
            }
        }
        if (!(*c++)) {
            argc++;
            break;
        }
    }
    if (!argc) { /* No command */
        return COMMAND_ERROR_NO_SUCH_COMMAND;
    }
    Command *command = _find_command (_arg_strings[0].data);
    if (!command) {
        return COMMAND_ERROR_NO_SUCH_COMMAND;
    }
    char *argv[COMMAND_MAX_ARGS];
    for (int i = 0; i < argc; ++i) {
        argv[i] = _arg_strings[i].data;
    }
    if (command->callback (argc, argv)) {
        return COMMAND_ERROR_COMMAND_FAILED;
    }
    return COMMAND_ERROR_NONE;
}

GSList *command_commands (void)
{
    return _commands;
}
