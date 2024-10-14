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

#ifndef _KK_SEARCH_H_
#define _KK_SEARCH_H_

#include <glib.h>
#include <regex.h>

typedef enum {
    SEARCH_CASE_SENSITIVY_YES = 0,
    SEARCH_CASE_SENSITIVY_NO,
    SEARCH_CASE_SENSITIVY_SMART
} SearchCaseSensitivy;

typedef struct {
    gchar *current; /* search string to match */
    regex_t regex; /* regcomp result from current */
    SearchCaseSensitivy case_sensitivy;
/* TODO: ??? 
    gboolean highlight;
    gboolean noh;
    */
} SearchType;

#define MAX_REGEX_RESULTS 100
#define MAX_REGEX_SUB_RESULTS 1
    
typedef struct {
    off_t start; /* matching search start byte */
    off_t end;   /* matching search end byte */
} SearchResultType;

typedef struct {
    size_t num_matches;
    SearchResultType results[MAX_REGEX_RESULTS];
} SearchMatchType;

/*
 *  Initial values for search struct
 *
 *  in: s - search struct
 *  return: zero at success
 */
gint search_init (SearchType *s);

/*
 * Free any allocated SearchType 
 *
 *  in: s - search struct
 *  return: -
 */
void search_free (SearchType *s);

/*
 * Set search string and compiles new restrict.
 *
 * in: s - search struct 
 *     str - new search string for regex, NULL frees' current
 * return: zero at success
 */
gint search_set (SearchType *s, const gchar *str);

/*
 * Search match
 *
 * in: s - search struct 
 *     line - nline to match
 *     sm - search match struct
 * Return: match, zero when matching. Only when match search match struct is filled
 */
gint search_match (SearchType *s, const gchar *line, SearchMatchType *sm);

#endif
