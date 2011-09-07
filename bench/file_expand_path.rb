require "benchmark"
require "fenix"

# build a huge path for testing (319 characters long)
names = ["abc", "def", "mno", "xyz"]
tmp = []
80.times { tmp << names.sample }
long_path = tmp.join(File::SEPARATOR)
full_long_path = File.join("C:", long_path)

TESTS = 10_000

puts "File.expand_path: #{TESTS} times."
Benchmark.bmbm do |results|
  results.report("Ruby ''") { TESTS.times {  File.expand_path('') } }
  results.report("Fenix ''") { TESTS.times { Fenix::File.expand_path('') } }

  results.report("Ruby '.'") { TESTS.times {  File.expand_path('.') } }
  results.report("Fenix '.'") { TESTS.times { Fenix::File.expand_path('.') } }

  results.report("Ruby 'foo', 'bar'") { TESTS.times {  File.expand_path('foo', 'bar') } }
  results.report("Fenix 'foo', 'bar'") { TESTS.times { Fenix::File.expand_path('foo', 'bar') } }

  results.report("Ruby '', 'C:/'") { TESTS.times {  File.expand_path('', 'C:/') } }
  results.report("Fenix '', 'C:/'") { TESTS.times { Fenix::File.expand_path('', 'C:/') } }

  results.report("Ruby 'foo', 'C:/'") { TESTS.times {  File.expand_path('foo', 'C:/') } }
  results.report("Fenix 'foo', 'C:/'") { TESTS.times { Fenix::File.expand_path('foo', 'C:/') } }

  results.report("Ruby '~'") { TESTS.times {  File.expand_path('~') } }
  results.report("Fenix '~'") { TESTS.times { Fenix::File.expand_path('~') } }

  results.report("Ruby '~/foo'") { TESTS.times {  File.expand_path('~/foo') } }
  results.report("Fenix '~/foo'") { TESTS.times { Fenix::File.expand_path('~/foo') } }

  results.report("Ruby '~', 'C:/Foo'") { TESTS.times {  File.expand_path('~', 'C:/Foo') } }
  results.report("Fenix '~', 'C:/Foo'") { TESTS.times { Fenix::File.expand_path('~', 'C:/Foo') } }

  results.report("Ruby long_path") { TESTS.times { File.expand_path(long_path) } }
  results.report("Fenix long_path") { TESTS.times { Fenix::File.expand_path(long_path) } }

  results.report("Ruby long_path, 'rel'") { TESTS.times { File.expand_path(long_path, 'rel') } }
  results.report("Fenix long_path, 'rel'") { TESTS.times { Fenix::File.expand_path(long_path, 'rel') } }

  results.report("Ruby long_path, 'C:/Foo'") { TESTS.times { File.expand_path(long_path, 'C:/Foo') } }
  results.report("Fenix long_path, 'C:/Foo'") { TESTS.times { Fenix::File.expand_path(long_path, 'C:/Foo') } }

  results.report("Ruby full_long_path") { TESTS.times { File.expand_path(full_long_path) } }
  results.report("Fenix full_long_path") { TESTS.times { Fenix::File.expand_path(full_long_path) } }
end
