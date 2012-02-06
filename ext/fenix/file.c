#include "fenix.h"

static inline void
fenix_replace_wchar(wchar_t *s, int find, int replace)
{
	while (*s != 0) {
		if (*s == find)
			*s = replace;
		s++;
	}
}

/*
  Return user's home directory using environment variables combinations.
  Memory allocated by this function should be manually freeded afterwards.
*/
static wchar_t *
fenix_home_dir()
{
	wchar_t *buffer = NULL;
	size_t buffer_len = 0, len = 0;
	size_t home_env = 0;

	// determine User's home directory trying:
	// HOME, HOMEDRIVE + HOMEPATH and USERPROFILE environment variables
	// TODO: Special Folders - Profile and Personal

	/*
	  GetEnvironmentVariableW when used with NULL will return the required
	  buffer size and its terminating character.
	  http://msdn.microsoft.com/en-us/library/windows/desktop/ms683188(v=vs.85).aspx
	*/

	if (len = GetEnvironmentVariableW(L"HOME", NULL, 0)) {
		buffer_len = len;
		home_env = 1;
	} else if (len = GetEnvironmentVariableW(L"HOMEDRIVE", NULL, 0)) {
		buffer_len = len;
		if (len = GetEnvironmentVariableW(L"HOMEPATH", NULL, 0)) {
			buffer_len += len;
			home_env = 2;
		} else {
			buffer_len = 0;
		}
	} else if (len = GetEnvironmentVariableW(L"USERPROFILE", NULL, 0)) {
		buffer_len = len;
		home_env = 3;
	}

	// allocate buffer
	if (home_env)
		buffer = (wchar_t *)malloc(buffer_len * sizeof(wchar_t));

	switch (home_env) {
		case 1: // HOME
			GetEnvironmentVariableW(L"HOME", buffer, buffer_len);
			break;
		case 2: // HOMEDRIVE + HOMEPATH
			len = GetEnvironmentVariableW(L"HOMEDRIVE", buffer, buffer_len);
			GetEnvironmentVariableW(L"HOMEPATH", buffer + len, buffer_len - len);
			break;
		case 3: // USERPROFILE
			GetEnvironmentVariableW(L"USERPROFILE", buffer, buffer_len);
			break;
		default:
			// wprintf(L"Failed to determine user home directory.\n");
			break;
	}

	if (home_env) {
		// sanitize backslashes with forwardslashes
		fenix_replace_wchar(buffer, L'\\', L'/');

		// wprintf(L"home dir: '%s' using home_env (%i)\n", buffer, home_env);
		return buffer;
	}

	return NULL;
}

static VALUE
fenix_coerce_to_path(VALUE obj)
{
	VALUE tmp;
	ID to_path;

	CONST_ID(to_path, "to_path");
	tmp = rb_check_funcall(obj, to_path, 0, 0);
	if (tmp == Qundef)
		tmp = obj;

	return StringValue(tmp);
}

// TODO: can we fail allocating memory?
static VALUE
fenix_file_expand_path_plain(int argc, VALUE *argv)
{
	size_t size = 0, wpath_len = 0, wdir_len = 0, whome_len = 0;
	size_t buffer_len = 0;
	char *fullpath = NULL;
	wchar_t *wfullpath = NULL, *wpath = NULL, *wpath_pos = NULL, *wdir = NULL;
	wchar_t *whome = NULL, *buffer = NULL, *buffer_pos = NULL;
	UINT cp;
	VALUE result = Qnil, path = Qnil, dir = Qnil;

	// retrieve path and dir from argv
	rb_scan_args(argc, argv, "11", &path, &dir);

	// coerce them to string
	path = fenix_coerce_to_path(path);
	if (!NIL_P(dir))
		dir = fenix_coerce_to_path(dir);

	// convert char * to wchar_t
	// path
	if (!NIL_P(path)) {
		size = MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(path), -1, NULL, 0) + 1;
		wpath = wpath_pos = (wchar_t *)malloc(size * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(path), -1, wpath, size);
		wpath_len = wcslen(wpath);
		// wprintf(L"wpath: '%s' with (%i) characters long.\n", wpath, wpath_len);
	}

	// dir
	if (!NIL_P(dir)) {
		size = MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(dir), -1, NULL, 0) + 1;
		wdir = (wchar_t *)malloc(size * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(dir), -1, wdir, size);
		wdir_len = wcslen(wdir);
		// wprintf(L"wdir: '%s' with (%i) characters long.\n", wdir, wdir_len);
	}

	/* determine if we need the user's home directory */
	if ((wpath_len == 1 && wpath_pos[0] == L'~') ||
		(wpath_len >= 2 && wpath_pos[0] == L'~' && IS_DIR_SEPARATOR_P(wpath_pos[1]))) {
		// wprintf(L"wpath requires expansion.\n");
		whome = fenix_home_dir();
		whome_len = wcslen(whome);

		// wprintf(L"whome: '%s' with (%i) characters long.\n", whome, whome_len);

		/* ignores dir since we are expading home */
		wdir_len = 0;

		/* exclude ~ from the result */
		wpath_pos++;
		wpath_len--;

		/* exclude separator if present */
		if (wpath_len && IS_DIR_SEPARATOR_P(wpath_pos[0])) {
			// wprintf(L"excluding expansion character and separator\n");
			wpath_pos++;
			wpath_len--;
		}
	} else if (wpath_len >= 2 && wpath_pos[1] == L':') {
		/* ignore dir since path contains a drive letter */
		// wprintf(L"Ignore dir since we have drive letter\n");
		wdir_len = 0;
	}

	// wprintf(L"wpath_len: %i\n", wpath_len);
	// wprintf(L"wdir_len: %i\n", wdir_len);
	// wprintf(L"whome_len: %i\n", whome_len);

	buffer_len = wpath_len + 1 + wdir_len + 1 + whome_len + 1;
	// wprintf(L"buffer_len: %i\n", buffer_len + 1);

	buffer = buffer_pos = (wchar_t *)malloc((buffer_len + 1) * sizeof(wchar_t));

	/* add home */
	if (whome_len) {
		// wprintf(L"Copying whome...\n");
		wcsncpy(buffer_pos, whome, whome_len);
		buffer_pos += whome_len;
	}

	/* Add separator if required */
	if (whome_len && wcsrchr(L"\\/:", buffer_pos[-1]) == NULL) {
		// wprintf(L"Adding separator after whome\n");
		buffer_pos[0] = L'\\';
		buffer_pos++;
	}

	if (wdir_len) {
		// wprintf(L"Copying wdir...\n");
		wcsncpy(buffer_pos, wdir, wdir_len);
		buffer_pos += wdir_len;
	}

	/* add separator if required */
	if (wdir_len && wcsrchr(L"\\/:", buffer_pos[-1]) == NULL) {
		// wprintf(L"Adding separator after wdir\n");
		buffer_pos[0] = L'\\';
		buffer_pos++;
	}

	/* now deal with path */
	if (wpath_len) {
		// wprintf(L"Copying wpath...\n");
		wcsncpy(buffer_pos, wpath_pos, wpath_len);
		buffer_pos += wpath_len;
	}

	/* GetFullPathNameW requires at least "." to determine current directory */
	if (wpath_len == 0) {
		// wprintf(L"Adding '.' to buffer\n");
		buffer_pos[0] = L'.';
		buffer_pos++;
	}

	/* Ensure buffer is NULL terminated */
	buffer_pos[0] = L'\0';

	// wprintf(L"buffer: '%s'\n", buffer);

	// FIXME: Make this more robust
	// Determine require buffer size
	size = GetFullPathNameW(buffer, 0, NULL, NULL);
	if (size) {
		// allocate enough memory to contain the response
		wfullpath = (wchar_t *)malloc(size * sizeof(wchar_t));
		GetFullPathNameW(buffer, size, wfullpath, NULL);

		/* Calculate the new size and leave the garbage out */
		size = wcslen(wfullpath);

		/* Remove any trailing slashes */
		if (IS_DIR_SEPARATOR_P(wfullpath[size - 1]) && wfullpath[size - 2] != L':') {
			// wprintf(L"Removing trailing slash\n");
			wfullpath[size - 1] = L'\0';
		}

		// sanitize backslashes with forwardslashes
		fenix_replace_wchar(wfullpath, L'\\', L'/');
		// wprintf(L"wfullpath: '%s'\n", wfullpath);

		// What CodePage should we use?
		cp = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

		// convert to char *
		size = WideCharToMultiByte(cp, 0, wfullpath, -1, NULL, 0, NULL, NULL) + 1;
		fullpath = (char *)malloc(size * sizeof(char));
		WideCharToMultiByte(cp, 0, wfullpath, -1, fullpath, size, NULL, NULL);

		// convert to VALUE and set the filesystem encoding
		result = rb_filesystem_str_new_cstr(fullpath);
	}

	// TODO: better cleanup
	if (buffer)
		free(buffer);

	if (wpath)
		free(wpath);

	if (wdir)
		free(wdir);

	if (whome)
		free(whome);

	if (wfullpath)
		free(wfullpath);

	if (fullpath)
		free(fullpath);

	return result;
}

/* Convert the path from char to wchar with specified code page */
static inline void
fenix_path_to_wchar(VALUE path, wchar_t **wpath, wchar_t **wpath_pos, size_t *wpath_len, UINT code_page)
{
	size_t size;

	if (NIL_P(path))
		return;

	size = MultiByteToWideChar(code_page, 0, RSTRING_PTR(path), -1, NULL, 0) + 1;
	*wpath = (wchar_t *)malloc(size * sizeof(wchar_t));
	if (wpath_pos)
		*wpath_pos = *wpath;

	MultiByteToWideChar(code_page, 0, RSTRING_PTR(path), -1, *wpath, size);
	*wpath_len = size - 2; // wcslen(*wpath);
}

/* Remove trailing invalid ':$DATA' of the path. */
static inline size_t
fenix_remove_invalid_alternative_data(wchar_t *wfullpath, size_t size) {
	static const wchar_t prime[] = L":$DATA";
	enum {prime_len = (sizeof(prime) / sizeof(wchar_t)) -1};

	if (size <= prime_len || _wcsnicmp(wfullpath + size - prime_len, prime, prime_len) != 0)
		return size;

	// wprintf(L"remove trailng ':$DATA': %s, %s\n", wfullpath, &wfullpath[size - prime_len]);
	/* alias of stream */
	/* get rid of a bug of x64 VC++ */
	if (wfullpath[size - (prime_len + 1)] == ':') {
		/* remove trailing '::$DATA' */
		size -= prime_len + 1; /* prime */
		wfullpath[size] = L'\0';
		// wprintf(L"removed trailng '::$DATA': %s\n", wfullpath);
	} else {
		/* remove trailing ':$DATA' of paths like '/aa:a:$DATA' */
		wchar_t *pos = wfullpath + size - (prime_len + 1);
		while (!IS_DIR_SEPARATOR_P(*pos) && pos != wfullpath) {
			if (*pos ==  L':') {
				size -= prime_len; /* alternative */
				wfullpath[size] = L'\0';
				// wprintf(L"removed trailng ':$DATA': %s\n", wfullpath);
				break;
			}
			pos--;
		}
	}
	return size;
}

/*
  Replace the last part of the path to long name.
  We try to avoid to call FindFirstFileW() since it takes long time.
*/
static inline size_t
fenix_replace_to_long_name(wchar_t **wfullpath, size_t size, int heap) {
	WIN32_FIND_DATAW find_data;
	HANDLE find_handle;

	/*
	  Skip long name conversion if the path is already long name.
	  Short name is 8.3 format.
	  http://en.wikipedia.org/wiki/8.3_filename
	  This check can be skipped for directory components that have file
	  extensions longer than 3 characters, or total lengths longer than
	  12 characters.
	  http://msdn.microsoft.com/en-us/library/windows/desktop/aa364980(v=vs.85).aspx
	*/
	size_t const max_short_name_size = 8 + 1 + 3;
	size_t const max_extension_size = 3;
	size_t path_len = 1, extension_len = 0;
	wchar_t *pos = *wfullpath + size - 1;
	while (!IS_DIR_SEPARATOR_P(*pos) && pos != *wfullpath) {
		if (!extension_len && *pos == L'.') {
			extension_len = path_len - 1;
		}
		if (path_len > max_short_name_size || extension_len > max_extension_size) {
			// wprintf(L"skip long name conversion: %s\n", pos);
			return size;
		}
		path_len++;
		pos--;
	}

	find_handle = FindFirstFileW(*wfullpath, &find_data);
	if (find_handle != INVALID_HANDLE_VALUE) {
		size_t trail_pos = wcslen(*wfullpath);
		size_t file_len = wcslen(find_data.cFileName);

		FindClose(find_handle);
		while (trail_pos > 0) {
			if (IS_DIR_SEPARATOR_P((*wfullpath)[trail_pos]))
				break;
			trail_pos--;
		}
		size = trail_pos + 1 + file_len;
		if ((size + 1) > sizeof(*wfullpath) / sizeof((*wfullpath)[0])) {
			wchar_t *buf = (wchar_t *)malloc((size + 1) * sizeof(wchar_t));
			wcsncpy(buf, *wfullpath, trail_pos + 1);
			if (heap)
				free(*wfullpath);
			*wfullpath = buf;
		}
		wcsncpy(*wfullpath + trail_pos + 1, find_data.cFileName, file_len + 1);
	}
	return size;
}

/* Return system code page. */
static inline UINT system_code_page() {
	return AreFileApisANSI() ? CP_ACP : CP_OEMCP;
}

/* cache 'encoding name' => 'code page' into a hash */
static VALUE rb_code_page;

/*
  Return code page number of the encoding.
  Cache code page into a hash for performance since finding the code page in
  Encoding#names is slow.
*/
static UINT
fenix_code_page(rb_encoding *enc)
{
	VALUE code_page_value, name_key;
	VALUE encoding, names_ary = Qundef, name;
	ID names;
	long i;

	if (!enc)
		return system_code_page();

	name_key = rb_usascii_str_new2(rb_enc_name(enc));
	code_page_value = rb_hash_aref(rb_code_page, name_key);
	if (code_page_value != Qnil) {
		// printf("cached code page: %i\n", FIX2INT(code_page_value));
		if (FIX2INT(code_page_value) == -1) {
			return system_code_page();
		} else {
			return (UINT)FIX2INT(code_page_value);
		}
	}

	encoding = rb_enc_from_encoding(enc);
	if (!NIL_P(encoding)) {
		CONST_ID(names, "names");
		names_ary = rb_funcall(encoding, names, 0);
	}

	if (names_ary != Qundef) {
		for (i = 0; i < RARRAY_LEN(names_ary); i++) {
			name = RARRAY_PTR(names_ary)[i];
			if (strncmp("CP", RSTRING_PTR(name), 2) == 0) {
				int code_page = atoi(RSTRING_PTR(name) + 2);
				rb_hash_aset(rb_code_page, name_key, INT2FIX(code_page));
				return (UINT)code_page;
			}
		}
	}

	rb_hash_aset(rb_code_page, name_key, INT2FIX(-1));
	return system_code_page();
}

#define PATH_BUFFER_SIZE MAX_PATH * 2

// TODO: can we fail allocating memory?
static VALUE
fenix_file_expand_path(int argc, VALUE *argv)
{
	size_t size = 0, wpath_len = 0, wdir_len = 0, whome_len = 0;
	size_t buffer_len = 0;
	char *fullpath = NULL;
	wchar_t *wfullpath = NULL, *wpath = NULL, *wpath_pos = NULL, *wdir = NULL;
	wchar_t *whome = NULL, *buffer = NULL, *buffer_pos = NULL;
	UINT cp;
	VALUE result = Qnil, path = Qnil, dir = Qnil;
	wchar_t wfullpath_buffer[PATH_BUFFER_SIZE];
	wchar_t path_drive = L'\0', dir_drive = L'\0';
	int ignore_dir = 0;
	rb_encoding *path_encoding;

	// 1: convert to long name
	// 0: not convert to long name
	int to_long_name = 0;

	VALUE to_long = Qnil;

	// retrieve path and dir from argv
	rb_scan_args(argc, argv, "12", &path, &dir, &to_long);

	// convert to_long to int
	if (!NIL_P(to_long))
		to_long_name = FIX2INT(to_long);

	// get path encoding
	if (NIL_P(dir)) {
		path_encoding = rb_enc_get(path);
	} else {
		path_encoding = rb_enc_check(path, dir);
	}
	cp = fenix_code_page(path_encoding);
	// printf("code page: %i\n", cp);

	// coerce them to string
	path = fenix_coerce_to_path(path);

	// convert char * to wchar_t
	// path
	fenix_path_to_wchar(path, &wpath, &wpath_pos, &wpath_len, cp);
	// wprintf(L"wpath: '%s' with (%i) characters long.\n", wpath, wpath_len);

	/* determine if we need the user's home directory */
	if ((wpath_len == 1 && wpath_pos[0] == L'~') ||
		(wpath_len >= 2 && wpath_pos[0] == L'~' && IS_DIR_SEPARATOR_P(wpath_pos[1]))) {
		// wprintf(L"wpath requires expansion.\n");
		whome = fenix_home_dir();
		if (whome == NULL) {
			free(wpath);
			rb_raise(rb_eArgError, "couldn't find HOME environment -- expanding `~'");
		}
		if (PathIsRelativeW(whome)) {
			free(wpath);
			rb_raise(rb_eArgError, "non-absolute home");
		}
		whome_len = wcslen(whome);

		if (wpath_len == 1)
			to_long_name = 0;

		// wprintf(L"whome: '%s' with (%i) characters long.\n", whome, whome_len);

		/* ignores dir since we are expading home */
		ignore_dir = 1;

		/* exclude ~ from the result */
		wpath_pos++;
		wpath_len--;

		/* exclude separator if present */
		if (wpath_len && IS_DIR_SEPARATOR_P(wpath_pos[0])) {
			// wprintf(L"excluding expansion character and separator\n");
			wpath_pos++;
			wpath_len--;
		}
	} else if (wpath_len >= 2 && wpath_pos[1] == L':') {
		if (wpath_len >= 3 && IS_DIR_SEPARATOR_P(wpath_pos[2])) {
			/* ignore dir since path contains a drive letter and a root slash */
			// wprintf(L"Ignore dir since we have drive letter and root slash\n");
			ignore_dir = 1;
		} else {
			/* determine if we ignore dir or not later */
			path_drive = wpath_pos[0];
		}
	} else if (wpath_len >= 2 && wpath_pos[0] == L'~') {
		wchar_t *wuser = wpath_pos + 1;
		wchar_t *pos = wuser;
		char *user;

		while (!IS_DIR_SEPARATOR_P(*pos) && *pos != '\0')
			pos++;

		*pos = '\0';
		size = WideCharToMultiByte(cp, 0, wuser, -1, NULL, 0, NULL, NULL);
		user = (char *)malloc(size * sizeof(char));
		WideCharToMultiByte(cp, 0, wuser, -1, user, size, NULL, NULL);

		/* convert to VALUE and set the path encoding */
		result = rb_enc_str_new(user, size - 1, path_encoding);

		free(wpath);
		if (user)
			free(user);

		rb_raise(rb_eArgError, "can't find user %s", StringValuePtr(result));
	}

	/* convert dir */
	if (!ignore_dir && !NIL_P(dir)) {
		// coerce them to string
		dir = fenix_coerce_to_path(dir);

		// convert char * to wchar_t
		// dir
		fenix_path_to_wchar(dir, &wdir, NULL, &wdir_len, cp);
		// wprintf(L"wdir: '%s' with (%i) characters long.\n", wdir, wdir_len);

		if (wdir_len >= 2 && wdir[1] == L':') {
			dir_drive = wdir[0];
			if (wpath_len && IS_DIR_SEPARATOR_P(wpath_pos[0])) {
				wdir_len = 2;
			}
		} else if (wdir_len >= 2 && IS_DIR_UNC_P(wdir)) {
			/* UNC path */
			if (wpath_len && IS_DIR_SEPARATOR_P(wpath_pos[0])) {
				/* cut the UNC path tail to '//host/share' */
				size_t separators = 0;
				size_t pos = 2;
				while (pos < wdir_len && separators < 2) {
					if (IS_DIR_SEPARATOR_P(wdir[pos])) {
						separators++;
					}
					pos++;
				}
				if (separators == 2)
					wdir_len = pos - 1;
				// wprintf(L"UNC wdir: '%s' with (%i) characters.\n", wdir, wdir_len);
			}
		}
	}

	/* determine if we ignore dir or not */
	if (!ignore_dir && path_drive && dir_drive) {
		if (towupper(path_drive) == towupper(dir_drive)) {
			/* exclude path drive letter to use dir */
			// wprintf(L"excluding path drive letter\n");
			wpath_pos += 2;
			wpath_len -= 2;
		} else {
			/* ignore dir since path drive is different from dir drive */
			ignore_dir = 1;
			wdir_len = 0;
		}
	}

	// wprintf(L"wpath_len: %i\n", wpath_len);
	// wprintf(L"wdir_len: %i\n", wdir_len);
	// wprintf(L"whome_len: %i\n", whome_len);

	buffer_len = wpath_len + 1 + wdir_len + 1 + whome_len + 1;
	// wprintf(L"buffer_len: %i\n", buffer_len + 1);

	buffer = buffer_pos = (wchar_t *)malloc((buffer_len + 1) * sizeof(wchar_t));

	/* add home */
	if (whome_len) {
		// wprintf(L"Copying whome...\n");
		wcsncpy(buffer_pos, whome, whome_len);
		buffer_pos += whome_len;
	}

	/* Add separator if required */
	if (whome_len && wcsrchr(L"\\/:", buffer_pos[-1]) == NULL) {
		// wprintf(L"Adding separator after whome\n");
		buffer_pos[0] = L'\\';
		buffer_pos++;
	}

	if (wdir_len) {
		// wprintf(L"Copying wdir...\n");
		wcsncpy(buffer_pos, wdir, wdir_len);
		buffer_pos += wdir_len;
	}

	/* add separator if required */
	if (wdir_len && wcsrchr(L"\\/:", buffer_pos[-1]) == NULL) {
		// wprintf(L"Adding separator after wdir\n");
		buffer_pos[0] = L'\\';
		buffer_pos++;
	}

	/* now deal with path */
	if (wpath_len) {
		// wprintf(L"Copying wpath...\n");
		wcsncpy(buffer_pos, wpath_pos, wpath_len);
		buffer_pos += wpath_len;
	}

	if ((wpath_len == 1 && buffer[0] == L'.') || (wpath_len == 2 && buffer[1] == L':'))
		to_long_name = 0;

	/* GetFullPathNameW requires at least "." to determine current directory */
	if (wpath_len == 0) {
		// wprintf(L"Adding '.' to buffer\n");
		buffer_pos[0] = L'.';
		buffer_pos++;
		to_long_name = 0;
	}

	/* Ensure buffer is NULL terminated */
	buffer_pos[0] = L'\0';

	// wprintf(L"buffer: '%s'\n", buffer);

	// FIXME: Make this more robust
	// Determine require buffer size
	size = GetFullPathNameW(buffer, PATH_BUFFER_SIZE, wfullpath_buffer, NULL);
	if (size) {
		if (size > PATH_BUFFER_SIZE) {
			// allocate enough memory to contain the response
			wfullpath = (wchar_t *)malloc(size * sizeof(wchar_t));
			size = GetFullPathNameW(buffer, size, wfullpath, NULL);
		} else {
			wfullpath = wfullpath_buffer;
		}


		/* Calculate the new size and leave the garbage out */
		// size = wcslen(wfullpath);

		/* Remove any trailing slashes */
		if (IS_DIR_SEPARATOR_P(wfullpath[size - 1]) && wfullpath[size - 2] != L':') {
			// wprintf(L"Removing trailing slash\n");
			size -= 1;
			wfullpath[size] = L'\0';
		}

		/* Remove any trailing dot */
		if (wfullpath[size - 1] == L'.') {
			// wprintf(L"Removing trailing dot\n");
			size -= 1;
			wfullpath[size] = L'\0';
		}

		/* removes trailing invalid ':$DATA' */
		size = fenix_remove_invalid_alternative_data(wfullpath, size);

		/* Replace the trailing path to long name */
		if (to_long_name)
			size = fenix_replace_to_long_name(&wfullpath, size, (wfullpath != wfullpath_buffer));


		// sanitize backslashes with forwardslashes
		fenix_replace_wchar(wfullpath, L'\\', L'/');
		// wprintf(L"wfullpath: '%s'\n", wfullpath);

		// What CodePage should we use?
		// cp = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

		// convert to char *
		size = WideCharToMultiByte(cp, 0, wfullpath, -1, NULL, 0, NULL, NULL);
		fullpath = (char *)malloc(size * sizeof(char));
		WideCharToMultiByte(cp, 0, wfullpath, -1, fullpath, size, NULL, NULL);

		/* convert to VALUE and set the path encoding */
		result = rb_enc_str_new(fullpath, size - 1, path_encoding);
	}

	// TODO: better cleanup
	if (buffer)
		free(buffer);

	if (wpath)
		free(wpath);

	if (wdir)
		free(wdir);

	if (whome)
		free(whome);

	if (wfullpath && wfullpath != wfullpath_buffer)
		free(wfullpath);

	if (fullpath)
		free(fullpath);

	return result;
}

static VALUE
fenix_file_replace()
{
	rb_define_singleton_method(rb_cFile, "expand_path", fenix_file_expand_path, -1);
	return Qtrue;
}

VALUE cFenixFile;

void Init_fenix_file()
{
	cFenixFile = rb_define_class_under(mFenix, "File", rb_cObject);

	rb_define_singleton_method(cFenixFile, "replace!", fenix_file_replace, 0);
	rb_define_singleton_method(cFenixFile, "expand_path", fenix_file_expand_path, -1);
	rb_define_singleton_method(cFenixFile, "expand_path_plain", fenix_file_expand_path_plain, -1);

	rb_code_page = rb_hash_new();

	/* prevent GC removing rb_code_page */
	rb_gc_register_mark_object(rb_code_page);
}
