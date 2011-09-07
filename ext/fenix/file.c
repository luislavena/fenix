#include "fenix.h"

static inline void
replace_wchar(wchar_t *s, int find, int replace)
{
	while (*s != 0) {
		if (*s == find)
			*s = replace;
		s++;
	}
}

static wchar_t *
home_dir()
{
	wchar_t *buffer = NULL;
	size_t buffer_len = 0, len = 0;
	size_t home_env = 0;

	// determine User's home directory trying:
	// HOME, HOMEDRIVE + HOMEPATH and USERPROFILE environment variables
	// TODO: Special Folders - Profile and Personal

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
		buffer = (wchar_t *)xcalloc(buffer_len, sizeof(wchar_t));

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
		replace_wchar(buffer, L'\\', L'/');

		// wprintf(L"home dir: '%s' using home_env (%i)\n", buffer, home_env);
		return buffer;
	}

	return NULL;
}

// TODO: can we fail allocating memory?
static VALUE
expand_path(int argc, VALUE *argv)
{
	size_t size = 0, wpath_len = 0, wdir_len = 0, whome_len = 0;
	size_t buffer_len = 0;
	char *fullpath = NULL;
	wchar_t *wfullpath = NULL, *wpath = NULL, *wpath_pos = NULL, *wdir = NULL;
	wchar_t *whome = NULL, *buffer = NULL;
	UINT cp;
	VALUE result = Qnil, path = Qnil, dir = Qnil;

	// retrieve path and dir from argv
	rb_scan_args(argc, argv, "11", &path, &dir);

	// convert char * to wchar_t
	// path
	if (!NIL_P(path)) {
		size = MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(path), -1, NULL, 0);
		wpath = wpath_pos = (wchar_t *)xcalloc(size + 1, sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(path), -1, wpath, size);
		wpath_len = wcslen(wpath);
		// wprintf(L"wpath: '%s' with (%i) characters long.\n", wpath, wpath_len);
	}

	// dir
	if (!NIL_P(dir)) {
		size = MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(dir), -1, NULL, 0);
		wdir = (wchar_t *)xcalloc(size + 1, sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, RSTRING_PTR(dir), -1, wdir, size);
		wdir_len = wcslen(wdir);
		// wprintf(L"wdir: '%s' with (%i) characters long.\n", wdir, wdir_len);
	}

	// calculate buffer size required
	if (wpath && wpath_len) {
		// determine if is expanding home directory (~)
		if (*wpath_pos == L'~') {
			if (whome = home_dir()) {
				whome_len = wcslen(whome);
				buffer_len += (whome_len + wpath_len);

				// exclude character from the result
				wpath_pos += 1;
				wpath_len -= 1;

				// wprintf(L"whome: '%s' with (%i) characters long.\n", whome, whome_len);
			} else {
				// TODO: Handle failure?
			}
		} else {
			buffer_len += wpath_len;
		}
	} else {
		// "."
		buffer_len += 1;
	}

	// make room in buffer for dir + 1 (slash)
	if (wdir && wdir_len) {
		buffer_len += wdir_len + 1;
	}

	// wprintf(L"buffer_len: %i\n", buffer_len + 1);
	buffer = (wchar_t *)xcalloc(buffer_len + 1, sizeof(wchar_t));

	// push home directory first
	if (whome_len) {
		// wprintf(L"Adding home '%s' to buffer\n", whome);
		wcsncat(buffer, whome, whome_len);
	} else if (wdir_len) {
		// push dir into buffer if present (and ensure separator)
		// wprintf(L"Adding dir '%s' to buffer\n", wdir);
		wcsncat(buffer, wdir, wdir_len);
	}

	// push path into buffer or default to "." (current)
	if (wpath_len) {
		// Only add separator if required
		if (whome_len || wdir_len) {
			// wprintf(L"Add separator slash to buffer\n");
			wcsncat(buffer, L"/", 1);
		}

		// wprintf(L"Adding '%s' to buffer\n", wpath_pos);
		wcsncat(buffer, wpath_pos, wpath_len);
	} else {
		wcsncat(buffer, L".", 1);
	}

	// wprintf(L"buffer: '%s'\n", buffer);

	// FIXME: Make this more robust
	// Determine require buffer size
	size = GetFullPathNameW(buffer, 0, NULL, NULL);
	if (size) {
		// allocate enough memory to contain the response
		wfullpath = (wchar_t *)xcalloc(size, sizeof(wchar_t));
		GetFullPathNameW(buffer, size, wfullpath, NULL);

		// sanitize backslashes with forwardslashes
		replace_wchar(wfullpath, L'\\', L'/');
		// wprintf(L"wfullpath: '%s'\n", wfullpath);

		// What CodePage should we use?
		cp = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

		// convert to char *
		size = WideCharToMultiByte(cp, 0, wfullpath, -1, NULL, 0, NULL, NULL);
		fullpath = (char *)xcalloc(size, sizeof(char));
		WideCharToMultiByte(cp, 0, wfullpath, -1, fullpath, size, NULL, NULL);

		// convert to VALUE and set the filesystem encoding
		result = rb_filesystem_str_new_cstr(fullpath);
	}

	// TODO: better cleanup
	if (buffer)
		xfree(buffer);

	if (wpath)
		xfree(wpath);

	if (wdir)
		xfree(wdir);

	if (whome)
		xfree(whome);

	if (wfullpath)
		xfree(wfullpath);

	if (fullpath)
		xfree(fullpath);

	return result;
}

VALUE cFenixFile;

void Init_fenix_file()
{
	cFenixFile = rb_define_class_under(mFenix, "File", rb_cObject);

	rb_define_singleton_method(cFenixFile, "expand_path", expand_path, -1);
}
