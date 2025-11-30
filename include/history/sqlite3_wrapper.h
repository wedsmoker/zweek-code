#pragma once

// Include SQLite3 header outside any namespaces and wrapped in extern "C"
// This prevents namespace pollution when included from within zweek namespace

#ifndef ZWEEK_SQLITE3_WRAPPER_H
#define ZWEEK_SQLITE3_WRAPPER_H

// Close any open namespaces before including sqlite3
#ifdef __cplusplus
extern "C" {
#endif

#include <sqlite3.h>

#ifdef __cplusplus
}
#endif

#endif // ZWEEK_SQLITE3_WRAPPER_H
