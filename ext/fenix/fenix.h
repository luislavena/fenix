#ifndef FENIX_H
#define FENIX_H

#include <ruby.h>
#include <ruby/encoding.h>
#include <wchar.h>
#include <shlwapi.h>
#include "file.h"

#define IS_DIR_SEPARATOR_P(c) (c == L'\\' || c == L'/')
#define IS_DIR_UNC_P(c) (IS_DIR_SEPARATOR_P(c[0]) && IS_DIR_SEPARATOR_P(c[1]))

extern VALUE mFenix;

#endif
