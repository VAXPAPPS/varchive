/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-command-backend.h — CLI tool fallback backend header
 */

#pragma once

#include "var-backend.h"

G_BEGIN_DECLS

#define VAR_TYPE_COMMAND_BACKEND (var_command_backend_get_type ())
G_DECLARE_FINAL_TYPE (VarCommandBackend, var_command_backend,
                       VAR, COMMAND_BACKEND, GObject)

VarCommandBackend *var_command_backend_new (void);

/* Check if a CLI tool is available on the system */
gboolean var_command_backend_tool_available (const char *tool_name);

G_END_DECLS
