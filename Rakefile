begin
  require "isolate/now"
rescue LoadError
  abort "This project requires Isolate to load it's dependencies. Please `gem install isolate` first."
end

require "rake/testtask"
require "rake/extensiontask"

Rake::TestTask.new(:spec) do |t|
  t.libs << "spec"
  t.pattern = "spec/**/*_spec.rb"
end

Rake::ExtensionTask.new("fenix") do |ext|
  ext.config_options << "--enable-debug"
end

# compile before running specs
task :spec => [:compile]

desc "Execute benchmarks"
task :bench => [:spec] do
  benchs = FileList["bench/*.rb"]
  benchs.each do |b|
    ruby "-Ilib #{b}"
  end
end

namespace :bench do
  desc "Execute benchmarks and include slow performing ones"
  task :slow => [:spec] do
    ENV["SLOW"] = "1"
    Rake::Task["bench"].invoke
  end
end
