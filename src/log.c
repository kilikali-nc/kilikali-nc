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

#include "log.h"
#include "config.h"
#include "paths.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

FILE *log_file = 0;
int log_debugging_on = 0;
#if defined(REDIRECT_STDERR_TO_FILE)
static int stderr_fd = -1;
static int save_err = -1;
#endif

int log_init (const char *file_path) {
    int retval = 0;
#if defined(REDIRECT_STDERR_TO_FILE)
    gchar *stderr_path;
#endif
    assert (!log_file);
    if (!file_path) {
        return 0;
    }
    FILE *file = fopen (file_path, "a+");
    if (!file) {
        fprintf(stderr, "Failed to open log file %s.\n", file_path);
        return -1;
    }
    log_file = file;
#if defined(REDIRECT_STDERR_TO_FILE)
    /* stderr to file. some gst plugings for example might have problems and
     * prints to stderr */
    stderr_path = paths_saved_data_stderr_log ();
    if (stderr_path == NULL) { 
        retval = -2;
        goto error;
    }
    stderr_fd = open (stderr_path,  O_RDWR|O_CREAT|O_TRUNC, 0644); 
    if (stderr_fd < 0) {
        retval = -3;
        goto error;
    }
    save_err = dup (fileno(stderr));
    if (dup2 (stderr_fd, fileno(stderr)) == -1) {
        retval = -4;
        goto error;
    }

error:
    g_free (stderr_path);
    if (retval != 0) log_destroy ();
#endif
    return retval;
}

void log_destroy (void) {
    if (log_file) {
        fclose (log_file);
        log_file = 0;
    }
#if defined(REDIRECT_STDERR_TO_FILE)
    fflush (stderr);
    if (stderr_fd > 0) close (stderr_fd);
    if (save_err > 0) {
        dup2 (save_err, fileno (stderr));
        close (save_err);
    }
#endif
}
