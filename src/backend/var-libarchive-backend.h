/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-libarchive-backend.h — libarchive backend header
 */

#pragma once

#include "var-backend.h"

G_BEGIN_DECLS

#define VAR_TYPE_LIBARCHIVE_BACKEND (var_libarchive_backend_get_type ())
G_DECLARE_FINAL_TYPE (VarLibarchiveBackend, var_libarchive_backend,
                       VAR, LIBARCHIVE_BACKEND, GObject)

VarLibarchiveBackend *var_libarchive_backend_new (void);

G_END_DECLS
