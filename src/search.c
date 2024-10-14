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

#include "search.h"
#include "util.h"
#include "config.h"

gint search_init (SearchType *s)
{
    if (s == NULL) return -1;
    s->current = NULL;
    return 0;
}

void search_free (SearchType *s)
{
    if (s == NULL) return;
    g_free (s->current);
    regfree (&s->regex);
    s->current = NULL;
}

gint search_set (SearchType *s, const gchar *str)
{
    if (s == NULL) return -1;

    if (str == NULL) {
        search_free (s);
        return 0; /* no error */
    }

    s->current = g_strdup (str);
    if (s->current == NULL) return -3;

    int flags = REG_EXTENDED;

    if (strncmp(config.search_case_sensivity, "no", 2) == 0) {
        s->case_sensitivy = SEARCH_CASE_SENSITIVY_NO;
    } if (strncmp(config.search_case_sensivity, "yes", 3) == 0) {
        s->case_sensitivy = SEARCH_CASE_SENSITIVY_YES;
    } else {
        s->case_sensitivy = SEARCH_CASE_SENSITIVY_SMART; 
    }

    switch (s->case_sensitivy) {
        case SEARCH_CASE_SENSITIVY_SMART: {
            if (util_has_upper (str) == FALSE) {
                flags |= REG_ICASE;
	    }
            break;
        }
        case SEARCH_CASE_SENSITIVY_NO: {
            flags |= REG_ICASE;
            break;
        }
        default:
            break;
    }

    int ret = regcomp (&s->regex, str, flags);
    if (ret != 0) {
        search_free (s);
	return -3;
    }
    return 0;
}

gint search_match (SearchType *s, const gchar *line, SearchMatchType *sm)
{
    int ret = 0;
    if (s == NULL) return -1;
    else if (s->current == NULL || s->current[0] == '\0') return -2;
    else if (line == NULL) return -3;
    else if (sm == NULL) return -3;

    sm->num_matches = 0;
    const gchar *tst = line;
    regmatch_t results[MAX_REGEX_SUB_RESULTS];
    do {
        ret = regexec (&s->regex, tst, MAX_REGEX_SUB_RESULTS, results, 0);
        if (ret != 0) {
           if (sm->num_matches == 0) return -4;
           else break;
        }

        for (size_t i = 0; i < MAX_REGEX_SUB_RESULTS; i++) {
            if (results[i].rm_so < 0) break;
            off_t add = tst - line;
            sm->results[sm->num_matches].start = add + results[i].rm_so;
            sm->results[sm->num_matches].end = add + results[i].rm_eo;
            sm->num_matches++;
        }
	tst += results[0].rm_eo;
    } while (sm->num_matches < MAX_REGEX_RESULTS);
    return 0;
}

