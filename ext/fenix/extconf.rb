require "mkmf"

# define mininum version of Windows (XP SP1)
$CPPFLAGS << " -D_WIN32_WINNT=0x0501"

have_library("kernel32")

["GetFullPathNameW"].each do |f|
  abort "'#{f}' is required for this extension to work." unless have_func(f)
end

create_makefile "fenix"
