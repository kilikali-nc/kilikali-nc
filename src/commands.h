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

#ifndef _KK_COMMANDS_H_
#define _KK_COMMANDS_H_

#include "command.h"

static Command quit_command = {
    .name = "quit",
    .description = "Quit from application.",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD | CMDLINE_MODE_FILEBROWSER,
    .callback = _quit_callback
};

static Command add_command = {
    .name = "add",
    .description = "Adds files, dirs or URLs.",
    .hint = COMMAND_HINT_PATH,
    .modes = CMDLINE_MODE_CMD,
    .callback = _add_callback
};

static Command remove_command = {
    .name = "remove",
    .description = "Removes playlist items.",
    .hint = COMMAND_HINT_RANGE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _remove_callback
};

static Command write_playlist_command = {
    .name = "write",
    .description = "Writes playlist file.",
    .hint = COMMAND_HINT_PATH,
    .modes = CMDLINE_MODE_CMD,
    .callback = _write_playlist_callback
};

static Command search_command = {
    .name = "search",
    .description = "Searchs from playlist.",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _search_callback
};

static Command open_filebrowser_command = {
    .name = "Ex",
    .description = "Open file browser.",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _open_filebrowser_callback
};

static Command change_directory_command = {
    .name = "cd",
    .description = "Change browsing directory.",
    .hint = COMMAND_HINT_DIR,
    .modes = CMDLINE_MODE_CMD | CMDLINE_MODE_FILEBROWSER,
    .callback = _change_directory_callback
};

static Command clear_search_command = {
    .name = "noh",
    .description = "Clear current search term.",
    .hint = COMMAND_HINT_PATH,
    .modes = CMDLINE_MODE_CMD | CMDLINE_MODE_FILEBROWSER,
    .callback = _clear_search_callback
};

static Command filebrowser_home_command = {
    .name = "home",
    .description = "Change directory to default music directory.",
    .hint = COMMAND_HINT_PATH,
    .modes = CMDLINE_MODE_FILEBROWSER,
    .callback = _filebrowser_home_callback
};

static Command help_command = {
    .name = "help",
    .description = "Show help.",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _help_callback
};

static Command print_working_directory_command = {
    .name = "pwd",
    .description = "Print path of current directory.",
    .modes = CMDLINE_MODE_CMD | CMDLINE_MODE_FILEBROWSER,
    .callback = _print_working_directory_callback
};

static Command metasearch_command = {
    .name = "metasearch",
    .description = "Searchs from playlist with uri, metadata",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _metasearch_callback
};

static Command seek_command = {
    .name = "seek",
    .description = "Seek current track. Formats: hour:min:sec / 60.10%.",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD,
    .callback = _seek_callback
};

static Command volume_command = {
    .name = "volume",
    .description = "Volume. Format: 0-100",
    .hint = COMMAND_HINT_NONE,
    .modes = CMDLINE_MODE_CMD | CMDLINE_MODE_FILEBROWSER,
    .callback = _volume_callback
};

#endif
