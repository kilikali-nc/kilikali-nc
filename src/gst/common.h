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

#ifndef _KK_GST_COMMON_
#define _KK_GST_COMMON_

#include <gst/gst.h>

#include "song.h"

typedef enum {
    GST_PLAYBIN_FLAGS_VIDEO = (1<<0),
    GST_PLAYBIN_FLAGS_AUDIO = (1<<1),
    GST_PLAYBIN_FLAGS_SUBS = (1<<2),
    GST_PLAYBIN_FLAGS_VISUAL = (1<<3),
    GST_PLAYBIN_FLAGS_SOFT_VOLUME = (1<<4),
    GST_PLAYBIN_FLAGS_NATIVE_AUDIO = (1<<5),
    GST_PLAYBIN_FLAGS_NATIVE_VIDEO = (1<<6),
    GST_PLAYBIN_FLAGS_NATIVE_DOWNLOAD = (1<<7),
    GST_PLAYBIN_FLAGS_NATIVE_BUFFERING = (1<<8),
    GST_PLAYBIN_FLAGS_NATIVE_DEINTERLACE = (1<<9),
    GST_PLAYBIN_FLAGS_NATIVE_SOFT_COLORBALANCE = (1<<10),
    GST_PLAYBIN_FLAGS_NATIVE_FORCE_FILTERS = (1<<11),
    GST_PLAYBIN_FLAGS_NATIVE_FORCE_SW_DECODERS = (1<<12)
} GstPlaybinFlags;

void gst_common_parse_tags (GstMessage *msg, Song *o);

#endif
