require "mkmf"

NEEDED_FUNCTIONS = %w(GetFullPathNameW GetEnvironmentVariableW)

# enable debugging
if enable_config("debug", false)
  $CFLAGS << " -g -gstabs+"
end

# define mininum version of Windows (XP SP1)
$CFLAGS << " -D_WIN32_WINNT=0x0501"

have_library("shlwapi")
abort "'PathIsRelativeW' is required." unless have_func("PathIsRelativeW", "shlwapi.h")

have_library("kernel32")

NEEDED_FUNCTIONS.each do |f|
  abort "'#{f}' is required for this extension to work." unless have_func(f)
end

create_makefile "fenix"
