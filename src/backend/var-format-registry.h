/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-format-registry.h — Format ↔ backend mapping header
 */

#pragma once

#include "var-backend.h"
#include "var-libarchive-backend.h"
#include "var-command-backend.h"

G_BEGIN_DECLS

#define VAR_TYPE_FORMAT_REGISTRY (var_format_registry_get_type ())
G_DECLARE_FINAL_TYPE (VarFormatRegistry, var_format_registry,
                       VAR, FORMAT_REGISTRY, GObject)

VarFormatRegistry *var_format_registry_get_default (void);

/* Get the appropriate backend for a file */
VarBackend    *var_format_registry_get_backend  (VarFormatRegistry *self,
                                                  const char       *filename);

/* Get the appropriate backend for a format type */
VarBackend    *var_format_registry_get_backend_for_format (VarFormatRegistry *self,
                                                            VarFormatType      format);

/* Check what capabilities are available for a format */
VarCapabilityFlags var_format_registry_get_caps (VarFormatRegistry *self,
                                                  VarFormatType      format);

G_END_DECLS
