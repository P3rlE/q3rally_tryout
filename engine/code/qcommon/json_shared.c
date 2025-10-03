/*
 * Shared JSON implementation wrapper to ensure a single translation unit
 * provides the json.h helpers for all modules that require them.
 */
#define JSON_IMPLEMENTATION
#include "json.h"
