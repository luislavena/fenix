require "mkmf"

# enable debugging
if enable_config("debug", false)
  $defs.push("-g -gstabs+") unless $defs.include?("-g -gstabs+")
end

# define mininum version of Windows (XP SP1)
$defs.push("-D_WIN32_WINNT=0x0501") unless $defs.include?("-D_WIN32_WINNT=0x0501")

have_library("kernel32")

["GetFullPathNameW"].each do |f|
  abort "'#{f}' is required for this extension to work." unless have_func(f)
end

create_makefile "fenix"
