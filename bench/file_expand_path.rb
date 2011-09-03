require "benchmark"
require "fenix"

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
end
