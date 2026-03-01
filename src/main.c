/*
 * VArchive - Professional Archive Manager for VAXP-OS
 * Copyright (C) 2026 VAXP Organization
 */

#include "core/var-application.h"

int
main (int argc, char *argv[])
{
    g_autoptr(VarApplication) app = var_application_new ();
    return g_application_run (G_APPLICATION (app), argc, argv);
}
