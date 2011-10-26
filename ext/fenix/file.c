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
fenix_file_expand_path(int argc, VALUE *argv)
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
}
