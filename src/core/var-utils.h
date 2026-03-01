/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-utils.h — Utility functions header
 */

#pragma once

#include <gtk/gtk.h>
#include "var-types.h"

G_BEGIN_DECLS

/* Format a byte count into human-readable string (e.g. "1.5 MB") */
char *var_utils_format_size (gint64 bytes);

/* Format a unix timestamp */
char *var_utils_format_date (gint64 timestamp);

/* Get a themed icon name for a MIME type */
const char *var_utils_get_file_icon_name (const char *filename);

/* Detect archive format from filename extension */
VarFormatType var_utils_detect_format (const char *filename);

/* Get human-readable format name */
const char *var_utils_format_name (VarFormatType format);

/* Get default extension for a format */
const char *var_utils_format_extension (VarFormatType format);

/* Check if a format is read-only */
gboolean var_utils_format_is_read_only (VarFormatType format);

/* Check if a format is a single-file compressor */
gboolean var_utils_format_is_single_file (VarFormatType format);

/* Get compression ratio as string (e.g. "42%") */
char *var_utils_format_ratio (gint64 original, gint64 compressed);

G_END_DECLS
