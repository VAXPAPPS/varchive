/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 *
 * var-application.h — Application class header
 */

#pragma once

#include <adwaita.h>
#include "var-types.h"

G_BEGIN_DECLS

#define VAR_TYPE_APPLICATION (var_application_get_type ())
G_DECLARE_FINAL_TYPE (VarApplication, var_application, VAR, APPLICATION, AdwApplication)

VarApplication *var_application_new (void);

G_END_DECLS
