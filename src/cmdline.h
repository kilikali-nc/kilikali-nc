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

#ifndef _KK_CMDLINE_H_
#define _KK_CMDLINE_H_

#include <glib.h>
#include "ncurses-event.h"
#include "cmdline-mode.h"
#include "command.h"

typedef enum {
    CMDLINE_MENU_MODE_NONE = 1,         /* cmdline */
    CMDLINE_MENU_MODE_LIST = (1<<1),    /* list in bar */
    CMDLINE_MENU_MODE_FULL = (1<<2),    /* window */
} CmdlineMenuMode;

typedef enum {
    CMDLINE_ITEM_TYPE_PLACEHOLDER = 0,
    CMDLINE_ITEM_TYPE_DIR = 1,
    CMDLINE_ITEM_TYPE_FILE = 2,
    CMDLINE_ITEM_TYPE_COMMAND = 3
} CmdlineItemType;

typedef struct {
    CmdlineItemType type;
    gchar *str;
    Command *cmd;
} CmdlineItem;


gboolean cmdline_init (GSList *commands);
void cmdline_free (void);

void cmdline_clear (void);

void cmdline_mode_set (CmdlineMode mode);
void cmdline_menu_mode_set (CmdlineMenuMode mode);
CmdlineMenuMode cmdline_menu_mode (void);

const gchar *cmdline_input (NCursesEvent *e);
glong cmdline_cursor_pos (void);
gboolean cmdline_cursor_pos_set (glong pos);

GSList *cmdline_found_commands (void);

/*
 * List of CmdlineFoundPath items
 */
GSList *cmdline_found_paths (void);
gchar *cmdline_search_dir (gint index);
gchar *cmdline_search_part (gint index);

gint cmdline_selected_index (void);
glong cmdline_select_command_by_index (gint index);
glong cmdline_select_path_by_index (gint index);

#endif
