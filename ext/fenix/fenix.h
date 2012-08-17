#ifndef FENIX_H
#define FENIX_H

#include <ruby.h>
#include <ruby/encoding.h>
#include <wchar.h>
#include <shlwapi.h>
#include "file.h"

#define IS_DIR_SEPARATOR_P(c) (c == L'\\' || c == L'/')
#define IS_DIR_UNC_P(c) (IS_DIR_SEPARATOR_P(c[0]) && IS_DIR_SEPARATOR_P(c[1]))

/* MultiByteToWideChar() doesn't work with code page 51932 */
#define INVALID_CODE_PAGE 51932
#define PATH_BUFFER_SIZE MAX_PATH * 2

#define insecure_obj_p(obj, level) ((level) >= 4 || ((level) > 0 && OBJ_TAINTED(obj)))

extern VALUE mFenix;

#endif
