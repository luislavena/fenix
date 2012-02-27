begin
  require "isolate/now"
rescue LoadError
  abort "This project requires Isolate to load it's dependencies. Please `gem install isolate` first."
end

# prefer the minitest gem over the bundled with Ruby
gem "minitest"
require "minitest/spec"
require "minitest/autorun"

require "support/temporary_directory"
require "support/windows_version"

require "fenix"
