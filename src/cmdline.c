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

#include "cmdline.h"
#include "config.h"
#include "util.h"

#define CMDLINE_SIZE (5*1024)
#define MAX_HISTORY_SIZE 31 /* 30 + current */

static gsize _line_index;
static gchar _line[CMDLINE_SIZE] = "";
static glong _cursor_position = 0;
static glong _tmp_position = 0;

static CmdlineMode _mode = CMDLINE_MODE_CMD;
static CmdlineMenuMode _menumode = CMDLINE_MENU_MODE_NONE;

static gchar *_hsearch[MAX_HISTORY_SIZE] = {NULL, };
static gchar *_hcmd[MAX_HISTORY_SIZE] = {NULL, };
static gchar *_hfb[MAX_HISTORY_SIZE] = {NULL, };
static gchar *_hfbsearch[MAX_HISTORY_SIZE] = {NULL, };
static gchar **_history;
static gsize _history_index = 0; // 0 = safe pace for new one

static void _add_line_to_history (void);
static void _copy_history_to_line (void);
static void _copy_line_to_history (void);

/* cmd finding */
static gboolean _find_cmd_positions (gint *start_pos, gint *end_pos, glong *utf8_start_pos, glong *utf8_end_pos, gint *d);
static gboolean _find_cmd (gboolean check_is_cursor_ok);

/* path finding */
static gboolean _try_to_find_path (gboolean dir_only);
static gboolean _find_dir_or_file (const char *path, gint size, gboolean dir_only);
static gboolean _find_path (void);
static void _select_parent_dir (void);
static void _select_dir_or_file (void);

static void _free_found_items (void);
static gboolean _has_found_items (void);

static void _handle_tab (gboolean forward);

static glong _find_prev_space (void);
static glong _find_next_space (void);

static void _line_remove_part (glong start_pos, glong end_pos);
static void _line_add_utf8 (glong pos, gchar *str, gint len);
static void _line_remove_utf8 (glong pos);

static gint _sort_str (gconstpointer a, gconstpointer b);

static GSList *_commands;
static GSList *_found_commands = NULL;
static Command *_current_command;

static GSList *_found_paths = NULL; /* list of files and dirs found by path finding */
static gint _selected_index = -1;  /* selected index. -1 unselected */
static gchar *_search_dir = NULL;   /* full dir part of */
static gchar *_search_part = NULL;  /* search part */

gboolean cmdline_init (GSList *commands)
{
    _commands = commands;
    if (_commands == NULL) return FALSE;
    else if (g_slist_length (_commands) < 1) goto error;
    _hcmd[0] = g_strdup("");
    if (_hcmd[0] == NULL) goto error;
    _hfb[0] = g_strdup("");
    if (_hfb[0] == NULL) goto error;
    _hsearch[0] = g_strdup("");
    if (_hsearch[0] == NULL) goto error;
    _hfbsearch[0] = g_strdup("");
    if (_hfbsearch[0] == NULL) goto error;
    _history = _hcmd;
    cmdline_clear ();
    if (strncmp(config.wild, "list", 4) == 0) {
        cmdline_menu_mode_set (CMDLINE_MENU_MODE_LIST);
    } else {
        cmdline_menu_mode_set (CMDLINE_MENU_MODE_NONE);
    }
    return TRUE;
error:
    cmdline_free ();
    return FALSE;
}

void cmdline_free (void)
{
    _free_found_items ();
    for (gsize i = 0; i < MAX_HISTORY_SIZE; i++) {
        g_free (_hcmd[i]);
        _hcmd[i] = NULL;
    }
    for (gsize i = 0; i < MAX_HISTORY_SIZE; i++) {
        g_free (_hsearch[i]);
        _hsearch[i] = NULL;
    }
    for (gsize i = 0; i < MAX_HISTORY_SIZE; i++) {
        g_free (_hfb[i]);
        _hfb[i] = NULL;
    }
    for (gsize i = 0; i < MAX_HISTORY_SIZE; ++i) {
        g_free (_hfbsearch[i]);
        _hfbsearch[i] = NULL;
    }
}

void cmdline_mode_set (CmdlineMode mode)
{
    _mode = mode;
    switch (mode) {
    case CMDLINE_MODE_CMD:
        _history = _hcmd;
        break;
    case CMDLINE_MODE_FILEBROWSER:
        _history = _hfb;
        break;
    case CMDLINE_MODE_SEARCH:
        _history = _hsearch;
        break;
    case CMDLINE_MODE_FILEBROWSER_SEARCH:
        _history = _hfbsearch;
        break;
    }
}

void cmdline_menu_mode_set (CmdlineMenuMode mode)
{
    _menumode = mode;
}

CmdlineMenuMode cmdline_menu_mode (void)
{
    return _menumode;
}

void cmdline_clear (void)
{
    _line_index = 0;
    memset (_line, 0, CMDLINE_SIZE);
    _line[_line_index] = '\0';
    _cursor_position = 0;
    _tmp_position = 0;
}

const gchar *cmdline_input (NCursesEvent *e)
{
    if (e == NULL) goto error;
    else if (_line_index >= (CMDLINE_SIZE - 1)) goto error;

    if (e->type == NCURSES_EVENT_TYPE_UTF8) {
        //if (e->utf8 == NULL) goto error;
        if (e->size < 2) goto error;
        _free_found_items ();
        _line_add_utf8 (_cursor_position, e->utf8, e->size);
        _cursor_position++;
        _tmp_position = _cursor_position;
    } else if (e->type == NCURSES_EVENT_TYPE_CH) {
        if (e->ch < 127 && e->ch > 31) {
            gchar str[2] = {0,};
            str[0] = (gchar)e->ch;
            _free_found_items ();
            _line_add_utf8 (_cursor_position, str, 1);
            _cursor_position++;
            _tmp_position = _cursor_position;
        } else {
            switch (e->ch) {
            case 10: { /* enter */
                _free_found_items ();
                _add_line_to_history ();
                _history_index = 0;
                break;
            }
            case 14: /* ctrl-n */
            case 9: { /* tabulator */
                _handle_tab (TRUE);
                break;
            }
            case 16: /* ctrl-p */
            case 353: { /* shift+tabulator */
                _handle_tab (FALSE);
                break;
            }
            case 260: { /* left */
                if (_menumode == CMDLINE_MENU_MODE_NONE || _has_found_items() == FALSE) {
                    _free_found_items ();
                    if (_cursor_position > 0) _cursor_position--;
                    _tmp_position = _cursor_position;
                } else {
                    _handle_tab (FALSE);
                }
                break;
            }
            case 261: { /* right */
                if (_menumode == CMDLINE_MENU_MODE_NONE || _has_found_items() == FALSE) {
                    if (_cursor_position > 0) _tmp_position = _cursor_position;
                    _free_found_items ();
                    if ( _cursor_position < g_utf8_strlen (_line, strlen (_line)) ) _cursor_position++;
                    _tmp_position = _cursor_position;
                } else {
                    _handle_tab (TRUE);
                }
                break;
            }
            case 259: { /* up */
                if (_menumode == CMDLINE_MENU_MODE_NONE || _has_found_items() == FALSE) {
                    _free_found_items ();
                    if (_history_index + 1 < MAX_HISTORY_SIZE && _history[_history_index + 1] != NULL) {
                       if (_history_index == 0) _copy_line_to_history (); /* copy only current */
                       _history_index++;
                       _copy_history_to_line ();
                    }
                } else {
                    _select_parent_dir ();
                }
                break;
            }
            case 258: { /* down */
                if (_menumode == CMDLINE_MENU_MODE_NONE || _has_found_items() == FALSE) {
                    _free_found_items ();
                    if (_history_index > 0 && _history[_history_index - 1] != NULL) {
                        _history_index--;
                        _copy_history_to_line ();
                    }
                } else {
                    _select_dir_or_file ();
                }
                break;
            }
            case 27: /* esc */
            case 3: { /* ctrl-c */
                if (_menumode == CMDLINE_MENU_MODE_NONE) {
                    cmdline_clear ();
                    _history_index = 0;
                } 
                _free_found_items ();
                break;
            }
            case 263: /* backspace */
            case 127: { /* backspace */
                _free_found_items ();
                if (_cursor_position > 0) {
                    _line_remove_utf8 (_cursor_position);
                    _cursor_position--;
                    _tmp_position = _cursor_position;
                }
                break;
            }
            case 330: { /* delete */
                _free_found_items ();
                if (_cursor_position < g_utf8_strlen (_line, _line_index)) {
                    _line_remove_utf8 (_cursor_position + 1);
                }
                _tmp_position = _cursor_position;
                break;
            }
            case 262: /* end */
            case 1: { /* ctrl-a */
                _cursor_position = 0;
                _free_found_items ();
                _tmp_position = _cursor_position;
                break;
            }
            case 360: /* end */
            case 5: { /* ctrl-e */
                _cursor_position = g_utf8_pointer_to_offset (_line, &_line[_line_index]);
                _free_found_items ();
                _tmp_position = _cursor_position;
                break;
            }
            case 552: { /* ctrl-left */
                _cursor_position = _find_prev_space ();
                _free_found_items ();
                _tmp_position = _cursor_position;
                break;
            }
            case 567: { /* ctrl-right */
                _cursor_position = _find_next_space ();
                _free_found_items ();
                _tmp_position = _cursor_position;
                break;
            }
            default:
                break;
            }
        }
    }

error:
    return (const gchar *)_line;
}

glong cmdline_cursor_pos (void)
{
    return _cursor_position;
}

gboolean cmdline_cursor_pos_set (glong pos)
{
    glong len = g_utf8_strlen (_line, _line_index);
    if (pos > len) return FALSE;
    else if (pos < 0) return FALSE;
    _cursor_position = pos;
    return TRUE;
}

GSList *cmdline_found_commands (void)
{
    return _found_commands;
}

GSList *cmdline_found_paths (void)
{
    return _found_paths;
}

gchar *cmdline_search_dir (gint index)
{
    return _search_dir;
}

gchar *cmdline_search_part (gint index)
{
    return _search_part;
}

gint cmdline_selected_index (void)
{
    return _selected_index;
}

glong cmdline_select_command_by_index (gint index)
{
    Command *cmd;
    gchar *s0;
    gint start_pos = 0;
    gint end_pos = 0;
    gint d = 0;
    glong utf8_start_pos = 0;
    glong utf8_end_pos = 0;

    if (_found_commands == NULL) return -1;
    else if (g_slist_length (_found_commands) <= index) return -1;

    CmdlineItem *ci = (CmdlineItem *)g_slist_nth_data (_found_commands, index);
    if (ci == NULL) return -1;
    cmd = ci->cmd;
    if (cmd == NULL) return -1;

    if (FALSE == _find_cmd_positions (&start_pos, &end_pos, &utf8_start_pos, &utf8_end_pos, &d)) {
        return -1;
    }

    s0 = g_utf8_substring (_line, utf8_end_pos, g_utf8_strlen (_line, _line_index));
    if (s0 == NULL) return -1;

    g_snprintf (&_line[start_pos], CMDLINE_SIZE-1, "%s%s", cmd->name, s0);
    g_free (s0);

    _line_index = strlen (_line);
    _line[_line_index] = '\0';

    end_pos = start_pos;
    while (_line[end_pos] != '\0' && _line[end_pos] != ' ') end_pos = end_pos + 1;
    utf8_end_pos = g_utf8_pointer_to_offset (_line, &_line[end_pos]);
    //_cursor_position = utf8_end_pos;

    _current_command = cmd;

    return utf8_end_pos;
}

glong cmdline_select_path_by_index (gint index)
{
    CmdlineItem *cfp;
    gint end_pos = 0;
    gint s = 0, e = 0;
    glong us = 0, ue = 0;
    gint d = 0;
    gsize pos = 0;
    gint len = -1;

    if (_found_paths == NULL) return -1;

    len = g_slist_length (_found_paths);
    if (len == 1) return -1;
    else if (len <= index) return -1;

    cfp = (CmdlineItem *)g_slist_nth_data (_found_paths, index);
    if (cfp == NULL) return -1;

    if (_find_cmd_positions (&s, &e, &us, &ue, &d) == TRUE) {
        pos = e - s;
        while (_line[pos] == ' ') pos++; 
    }
    GString *add = g_string_new ("");
    if (add == NULL) return -1;
    add = g_string_append (add, _search_dir);
    if (strlen(_search_dir) > 0 && _search_dir[strlen(_search_dir)-1] != G_DIR_SEPARATOR) add = g_string_append (add, G_DIR_SEPARATOR_S);
    add = g_string_append (add, cfp->str);
    if (cfp->type == CMDLINE_ITEM_TYPE_DIR) add = g_string_append (add, G_DIR_SEPARATOR_S);
    util_g_string_replace (add, " ", "\\ ", 0);
    memset (&_line[pos], 0, CMDLINE_SIZE-1-pos);
    g_snprintf (&_line[pos], CMDLINE_SIZE-1-pos, "%s", add->str);
    end_pos = pos + add->len - 1;
    g_string_free (add, TRUE);

    _line_index = strlen (_line);
    _line[_line_index] = '\0';

    ue = g_utf8_pointer_to_offset (_line, &_line[end_pos]);
    //_cursor_position = utf8_end_pos;

    return ue;
}

static gboolean _has_found_items (void)
{
    if (_found_commands != NULL) return TRUE;
    else if (_found_paths != NULL) return TRUE;
    return FALSE;
}

static void _add_line_to_history (void)
{
    g_free (_history[MAX_HISTORY_SIZE - 1]);
    g_free (_history[0]);
    for (gsize i = MAX_HISTORY_SIZE - 1; i >= 2; i--) {
        _history[i] = _history[i - 1];
    }
    _history[1] = g_strdup (_line);
    _history[0] = g_strdup ("");
}

static void _copy_history_to_line (void)
{
    gsize size;

    if (_history[_history_index] == NULL) return;

    size = strlen (_history[_history_index]);
    if (size != 0) memcpy (_line, _history[_history_index], size);
    _line[size] = '\0';
    _line_index = size;
    _cursor_position = size;
}

static void _copy_line_to_history (void)
{
    g_free (_history[_history_index]);
    _history[_history_index] = g_strdup (_line);
}

static gboolean _find_cmd_positions (gint *start_pos, gint *end_pos, glong *utf8_start_pos, glong *utf8_end_pos, gint *d)
{
    if (start_pos == NULL || end_pos == NULL || utf8_start_pos == NULL || utf8_end_pos == NULL || d == NULL) return FALSE;

    /* prefix spaces */
    while (_line[*start_pos] == ' ' && *start_pos <= _cursor_position) *start_pos = *start_pos + 1;

    /* find end */
    *end_pos = *start_pos;
    while (_line[*end_pos] != '\0' && _line[*end_pos] != ' ') *end_pos = *end_pos + 1;

    /* find utf8 positions */
    *utf8_start_pos = g_utf8_pointer_to_offset (_line, &_line[*start_pos]);
    *utf8_end_pos = g_utf8_pointer_to_offset (_line, &_line[*end_pos]);

    *d = *end_pos - *start_pos;

    return TRUE;
}

static gboolean _find_cmd (gboolean check_is_cursor_ok)
{
    gint start_pos = 0;
    gint end_pos = 0;
    gboolean found = FALSE;
    gint d = 0;
    glong utf8_end_pos = 0;
    glong utf8_start_pos = 0;
    Command *cmd;
    gchar *cp;
    CmdlineItem *ci;

    _current_command = NULL;

    if (_found_commands != NULL) {
        for (gint i = 0; i < g_slist_length (_found_commands); i++) {
           ci = (CmdlineItem *)g_slist_nth_data (_found_commands, i);
          /* Do not do this!!! g_free (ci->str); */
           g_free (ci);
        }
        g_slist_free (_found_commands);
    }
    _found_commands = NULL;
    if (_commands == NULL) return FALSE;
    else if (FALSE == _find_cmd_positions (&start_pos, &end_pos, &utf8_start_pos, &utf8_end_pos, &d)) {
        return FALSE;
    }

    if (check_is_cursor_ok == TRUE) {
        cp = g_utf8_offset_to_pointer (_line, _cursor_position);
        d = (gint)(cp - _line);
        if (_cursor_position <= utf8_start_pos) {
            for (gint i = 0; i < g_slist_length (_commands); i++) {
                cmd = (Command *)g_slist_nth_data (_commands, i);
                if ((_mode & cmd->modes) == 0) continue;
                ci = g_malloc0 (sizeof (CmdlineItem));
                ci->type = CMDLINE_ITEM_TYPE_COMMAND;
                ci->str = (gchar *)cmd->name;
                ci->cmd = cmd;
                _found_commands = g_slist_prepend (_found_commands, ci);
            }
            found = TRUE;
        } else if (_cursor_position > utf8_end_pos) return FALSE;
    }

    if (found == FALSE) {
        for (gint i = 0; i < g_slist_length (_commands); i++) {
            cmd = (Command *)g_slist_nth_data (_commands, i);
            if ((_mode & cmd->modes) == 0) continue;
            if (strncmp (cmd->name, &_line[start_pos], d) == 0) {
                ci = g_malloc0 (sizeof (CmdlineItem));
                ci->type = CMDLINE_ITEM_TYPE_COMMAND;
                ci->str = (gchar *)cmd->name;
                ci->cmd = cmd;
                _found_commands = g_slist_prepend (_found_commands, ci);
                found = TRUE;
                _current_command = cmd;
            }
        }
    }

    _found_commands = g_slist_sort (_found_commands, _sort_str);

    if (g_slist_length(_found_commands) > 1) {
        ci = g_malloc0 (sizeof (CmdlineItem));
        if (ci != NULL) {
            ci->str =  " ";
            ci->type = CMDLINE_ITEM_TYPE_PLACEHOLDER;
            _found_commands = g_slist_append (_found_commands, ci);
        }
    }

    if (found == TRUE && g_slist_length (_found_commands) == 1) {
        if (check_is_cursor_ok == TRUE) {
            glong cur = cmdline_select_command_by_index (0);
            if (cur > -1) {
                found = TRUE;
                _cursor_position = cur;
            } else {
                found = FALSE;
            }
        } else {
            found = TRUE;
        }
    }

    return found;
}

void _free_found_items (void)
{
    g_free (_search_dir);
    _search_dir = NULL;
    g_free (_search_part);
    _search_part = NULL;
    if (_found_paths != NULL) {
        for (gint i = 0; i < g_slist_length (_found_paths); i++) {
            CmdlineItem *p = g_slist_nth_data (_found_paths, i);
            if (p != NULL) g_free (p->str);
            g_free (p);
        }
        g_slist_free (_found_paths);
        _found_paths = NULL;
    }
    if (_found_commands != NULL) {
        for (gint i = 0; i < g_slist_length (_found_commands); i++) {
            CmdlineItem *p = (CmdlineItem *)g_slist_nth_data (_found_commands, i);
            /* Do not do this!!! if (p != NULL) g_free (p->str); */
            g_free (p);
        }
        g_slist_free (_found_commands);
        _found_commands = NULL;
    }
    _selected_index = -1;
    _tmp_position = 0;
}

static gboolean _find_dir_or_file (const char *path, gint size, gboolean dir_only)
{
    CmdlineItem *cfp;
    gboolean success = FALSE;
    GDir *dir = NULL;
    gchar *new_path = NULL;
    const gchar *name;
    gchar *find_name = NULL;
    gsize find_name_len = 0;
    GString *p = NULL;

    _free_found_items ();

    if (path == NULL) return FALSE;
    else if (size < 0) return FALSE;

    if (size == 0 || (size == 1 && strcmp (path, " ") == 0)) {
        new_path = g_strdup (".");
        _search_dir = g_strdup ("");
    } else if (size == 1 && strcmp (path, "~") == 0) {
        /* no need to sort */
        cfp = g_malloc0 (sizeof (CmdlineItem));
        if (p != NULL) {
            _search_dir = g_strdup ("~");
            cfp->type = CMDLINE_ITEM_TYPE_DIR;
            cfp->str = g_strdup ("~");
            _found_paths = g_slist_prepend (_found_paths, cfp);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        p = g_string_new_len (path, size);
        util_g_string_replace (p, "\\ ", " ", 0);
        if (p == NULL) goto find_dir_or_file_error;
        if (p->str[p->len-1] == G_DIR_SEPARATOR) {
            find_name = g_strdup ("");
            _search_dir = g_strdup (p->str);
        } else {
            find_name = g_path_get_basename (p->str);
            _search_dir = g_path_get_dirname (p->str);
            if (strcmp (_search_dir, ".") == 0) {
                g_free (_search_dir);
                _search_dir = g_strdup ("");
            }
        }
        find_name_len = strlen (find_name);
        new_path = g_path_get_dirname (p->str);
        if (find_name == NULL) goto find_dir_or_file_error;
    }
    if (new_path == NULL) goto find_dir_or_file_error;

    if (new_path[0] == '~') {
        gchar *tmp = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S, &new_path[1], NULL);
        if (tmp == NULL) goto find_dir_or_file_error;
        g_free (new_path);
        new_path = tmp;
    }
    dir = g_dir_open (new_path, 0, NULL);
    if (dir == NULL) goto find_dir_or_file_error;

    _search_part = g_strdup (find_name);

    while ((name = g_dir_read_name (dir)) != NULL) {
        if (find_name_len == 0 || (strncmp (find_name, name, find_name_len) == 0)) {
            CmdlineItemType type = CMDLINE_ITEM_TYPE_FILE;
            gchar *full_path;

            full_path = g_strconcat (new_path, G_DIR_SEPARATOR_S, name, NULL);
            if (full_path == NULL) {
                goto find_dir_or_file_error;
            }
            if (g_file_test (full_path, G_FILE_TEST_IS_DIR)) {
                type = CMDLINE_ITEM_TYPE_DIR;
            } else if (dir_only == TRUE) {
                g_free (full_path);
                continue;
            }
            g_free (full_path);

            cfp = g_malloc0 (sizeof (CmdlineItem));
            if (cfp != NULL) {
                cfp->str = g_strdup (name);
                cfp->type = type;
                _found_paths = g_slist_prepend (_found_paths, cfp);
                success = TRUE;
            }
        }
    }
    _found_paths = g_slist_sort (_found_paths, _sort_str);

find_dir_or_file_error:
    {
        /* Adds current path to the end of the list */
        gchar *add_name = NULL;
        if (find_name != NULL) {
            while (find_name[find_name_len-1] == G_DIR_SEPARATOR) find_name_len--;
            add_name = g_strndup (find_name, find_name_len);
            g_free (find_name);
        } else {
            add_name = g_strdup("");
        }
        if (add_name != NULL) {
            cfp = g_malloc0 (sizeof (CmdlineItem));
            if (cfp != NULL) {
                cfp->str = add_name;
                cfp->type = CMDLINE_ITEM_TYPE_PLACEHOLDER;
                _found_paths = g_slist_append (_found_paths, cfp);
            }
        }
    }
    if (p != NULL) g_string_free (p, TRUE);
    if (dir != NULL) g_dir_close (dir);
    g_free (new_path);
    return success;
}

/*
 * find backwards begining and end of the path.
 * in:  end_pos, position where usually command starts as byte index
 *      utf8_index, utf8-index (chars from start)
 * out: start_pos, position as byte index where path really starts
 *      cursor_pos, position as byte index where utf8_offset is
 */
static gboolean _find_path_position (gint end_pos, glong utf8_offset, gint *start_pos, gint *cursor_pos)
{
    gchar *cursor_str;
    *start_pos = 0;
    *cursor_pos = 0;
    cursor_str = g_utf8_offset_to_pointer (_line, utf8_offset);
    if (cursor_str == NULL) return FALSE;
    *cursor_pos = (gint)(cursor_str - _line);
    *start_pos = *cursor_pos;
    while ((_line[*start_pos] == ' ' || _line[*start_pos] == '\0') && end_pos <= *start_pos) (*start_pos)--;
    while ((_line[*start_pos] != ' ' || _line[(*start_pos)-1] == '\\')  && end_pos <= *start_pos) (*start_pos)--;
    (*start_pos)++;
    return TRUE;
}

static gboolean _try_to_find_path (gboolean dir_only)
{
    gint start_pos = 0;
    gint cursor_pos = 0;
    gint start_pos2 = 0;
    gint end_pos = 0;
    gint d = 0;
    glong utf8_end_pos = 0;
    glong utf8_start_pos = 0;

    if (FALSE == _find_cmd_positions (&start_pos, &end_pos, &utf8_start_pos, &utf8_end_pos, &d)) {
        return FALSE;
    }
    if (_cursor_position < utf8_end_pos) return FALSE;

    if (FALSE == _find_path_position (end_pos, _cursor_position, &start_pos2, &cursor_pos)) {
        return FALSE;
    }

    if (TRUE == _find_dir_or_file (&_line[start_pos2], cursor_pos-start_pos2, dir_only)) {
        gint len = g_slist_length (_found_paths);
        if (len > 1) return TRUE;
    }
    return FALSE;
}

static gboolean _find_path (void)
{
    if (FALSE == _find_cmd (FALSE)) return FALSE; /* try to find command */
    else if (_current_command == NULL) return FALSE;

    if (_current_command->hint == COMMAND_HINT_PATH) {
        return _try_to_find_path (FALSE);
    } else if (_current_command->hint == COMMAND_HINT_DIR) {
        return _try_to_find_path (TRUE);
    }

    return TRUE;
}

/* go trought backwards from end to start  */
static gboolean _remove_last_dir_path_from_line (gint start_from, gint cursor_pos, gint *cursor_pos_end)
{
    gchar *start = &_line[start_from];
    gchar *end = &_line[cursor_pos];
    if (cursor_pos - start_from > 0) {
        gchar *pos = g_utf8_find_prev_char (_line, &_line[cursor_pos]);
        gchar *pos2 = g_utf8_find_prev_char (_line, pos);
        if (pos == NULL) return FALSE;

        if (*pos == G_DIR_SEPARATOR) {
            pos = g_utf8_find_prev_char (_line, pos);
            pos2 = g_utf8_find_prev_char (_line, pos2);
        }
        while (pos > start) {
            if (*pos == ' ')  {
                if (pos2 >= start && *pos2 == '\\')  {
                    pos = g_utf8_find_prev_char (_line, pos);
                    pos = g_utf8_find_prev_char (_line, pos);
                    pos2 = g_utf8_find_prev_char (_line, pos2);
                    pos2 = g_utf8_find_prev_char (_line, pos2);
                } else break;
            } else if (*pos != G_DIR_SEPARATOR)  {
                pos = g_utf8_find_prev_char (_line, pos);
                pos2 = g_utf8_find_prev_char (_line, pos2);
            } else break;
        }
        if (*pos == G_DIR_SEPARATOR) pos = g_utf8_find_next_char (pos, end);
        size_t to_remove = (size_t)(end - pos);
        memset(pos, '\0', to_remove);
        _cursor_position = g_utf8_pointer_to_offset (_line, pos);
        *cursor_pos_end = (gint)(pos - _line);
        return TRUE;
    }
    return FALSE;
}

static void _select_parent_dir (void)
{
    gint start_pos = 0;
    gint cursor_pos = 0;
    gint cursor_pos2 = 0;
    gint start_pos2 = 0;
    gint end_pos = 0;
    gint d = 0;
    glong utf8_end_pos = 0;
    glong utf8_start_pos = 0;

    if (g_slist_length (_found_paths) == 0) return;

    if (FALSE == _find_cmd_positions (&start_pos, &end_pos, &utf8_start_pos, &utf8_end_pos, &d)) {
        return;
    }
    if (_cursor_position < utf8_end_pos) return;

    /* with line-mode this should be always end and start of path */
    if (FALSE == _find_path_position (end_pos, _cursor_position, &start_pos2, &cursor_pos)) {
        return;
    }

    /* at this point app has att least one dir eg. /bin/ */
    cursor_pos2 = cursor_pos;
    if (TRUE == _remove_last_dir_path_from_line(start_pos2, cursor_pos, &cursor_pos2)) {
        cursor_pos = cursor_pos2;
    }
    _cursor_position = g_utf8_pointer_to_offset (_line, &_line[cursor_pos]);

    /* at least / is there. test it */
    GString *tmp = g_string_new(&_line[start_pos2]);
    util_g_string_replace (tmp, "\\ ", " ", 0);
    char str[PATH_MAX];
    char *rpath = realpath (tmp->str, str);
    g_string_free(tmp, TRUE);
    if (rpath == NULL) {
        str[0] = '\0';
        rpath = str;
    }
    size_t size = strlen (&_line[start_pos2]);
    if (!(strlen(rpath) == 1 && rpath[0] == G_DIR_SEPARATOR) || size == 0) {
        if ((size > 2 && strncmp ( &_line[cursor_pos-3], ".." G_DIR_SEPARATOR_S, 3) != 0) || (size < 3 && size > 0)) { 
            cursor_pos2 = cursor_pos;
            if (TRUE == _remove_last_dir_path_from_line(start_pos2, cursor_pos, &cursor_pos2)) {
                cursor_pos = cursor_pos2;
            }
        } else {
            /* no need to utf8 handling */
            size = strlen (".." G_DIR_SEPARATOR_S);
            memcpy (&_line[cursor_pos], ".." G_DIR_SEPARATOR_S, size);
            _cursor_position += (glong)size;
            cursor_pos += (gint)size;
        }
    }
    _cursor_position = g_utf8_pointer_to_offset (_line, &_line[cursor_pos]);
    _tmp_position = _cursor_position;
    _selected_index = -1;
    _free_found_items ();
    _handle_tab(TRUE);
}

static void _select_dir_or_file (void)
{
    if (g_slist_length (_found_paths) == 0) return;

    _tmp_position = _cursor_position;
    _selected_index = -1;
    _free_found_items ();
    _handle_tab(TRUE);
}

static void _handle_tab (gboolean forward)
{
    gint len = g_slist_length (_found_commands);
    if (len < 1) {
        if (_find_cmd (TRUE) == TRUE) {

        }
        len = g_slist_length (_found_commands);
        if (len == 1) {
            _free_found_items ();
            return;
        }
        else if (len > 1) _cursor_position = _tmp_position;
    }

    if (len > 1) {
        _line_remove_part (_tmp_position, _cursor_position);
        if (forward == TRUE) {
            _selected_index++;
            if (_selected_index > (len-1)) _selected_index = 0;
        } else {
            _selected_index--;
            if (_selected_index < 0) _selected_index = len-1;
        }
        glong cur = cmdline_select_command_by_index (_selected_index);
        if (cur > -1) {
            _cursor_position = cur;
        } else {
            _cursor_position = 0;
        }
    }

    if (len == 0) {
        len = g_slist_length (_found_paths);
        if (len < 1) {
            if (_find_path () == TRUE) {
            }
            len = g_slist_length (_found_paths);
        }
        if (len > 1) {
            _tmp_position = _cursor_position;
            _line_remove_part (_tmp_position, _cursor_position);
            if (forward == TRUE) {
                _selected_index++;
                if (_selected_index > (len-1)) _selected_index = 0;
            } else {
                _selected_index--;
                if (_selected_index < 0) _selected_index = len-1;
            }
            _cursor_position = 1 + cmdline_select_path_by_index (_selected_index);
        }
    }
}

static glong _find_prev_space (void)
{
    gchar *pos = g_utf8_offset_to_pointer (_line, _cursor_position - 1);
    if (_cursor_position < 1) return 0;
    while (pos > _line) {
        if (*pos == ' ')  {
            pos--;
            if (*pos != '\\') break;
        } 
        --pos;
    }
    return g_utf8_pointer_to_offset (_line, pos);
}

static glong _find_next_space (void)
{
    gchar *pos = g_utf8_offset_to_pointer (_line, _cursor_position+1);
    while (pos < &_line[_line_index]) {
        if (*pos == ' ')  {
            gchar *pos2 = pos - 1;
            if (pos2 < _line) continue;
            if (*pos2 != '\\') break;
        } 
        ++pos;
    }
    if (pos > &_line[_line_index]) pos = &_line[_line_index];
    return g_utf8_pointer_to_offset (_line, pos);
}

static void _line_remove_part (glong start_pos, glong end_pos)
{
    if (start_pos > end_pos) return;
    gchar *s0 = g_utf8_substring (_line, 0, start_pos);
    gchar *s1 = g_utf8_substring (_line, end_pos, g_utf8_strlen (_line, _line_index));
    g_snprintf (_line, CMDLINE_SIZE-1, "%s%s", s0, s1);
    g_free (s0);
    g_free (s1);
    _line_index = _line_index - (g_utf8_offset_to_pointer (_line, end_pos) - g_utf8_offset_to_pointer (_line, start_pos));
    _line[_line_index] = '\0';
    memset (&_line[_line_index], 0, CMDLINE_SIZE-_line_index);
}

static void _line_add_utf8 (glong pos, gchar *str, gint len)
{
    gchar *s0 = g_utf8_substring (_line, 0, pos);
    gchar *s1 = g_utf8_substring (_line, pos, g_utf8_strlen (_line, _line_index));
    g_snprintf (_line, CMDLINE_SIZE-1, "%s%s%s", s0, str, s1);
    g_free (s0);
    g_free (s1);
    _line_index = _line_index + len;
    _line[_line_index] = '\0';
}

static void _line_remove_utf8 (glong pos)
{
    gchar *s0 = g_utf8_substring (_line, 0, pos-1);
    gchar *s1 = g_utf8_substring (_line, pos, g_utf8_strlen (_line, _line_index));
    g_snprintf (_line, CMDLINE_SIZE-1, "%s%s", s0, s1);
    g_free (s0);
    g_free (s1);
    _line_index = strlen (_line);
    _line[_line_index] = '\0';
}

static gint _sort_str (gconstpointer a, gconstpointer b)
{
    CmdlineItem *cfpa = (CmdlineItem *)a;
    CmdlineItem *cfpb = (CmdlineItem *)b;
    return strcmp (cfpa->str, cfpb->str);
}
