require "benchmark"
require "fenix"

# build a huge path for testing (319 characters long)
names = ["abc", "def", "mno", "xyz"]
tmp = []
80.times { tmp << names.sample }
long_path = tmp.join(File::SEPARATOR)
full_long_path = File.join("C:", long_path)

to_path       = Class.new { def to_path; "a/b/c"; end }.new
full_to_path  = Class.new { def to_path; "C:/FooBar"; end }.new

TESTS = 10_000

puts "File.expand_path: #{TESTS} times."
Benchmark.bmbm do |results|
  results.report("Ruby ''") { TESTS.times {  File.expand_path('') } }
  results.report("Fenix ''") { TESTS.times { Fenix::File.expand_path('') } }
  results.report("Plain ''") { TESTS.times { Fenix::File.expand_path_plain('') } }

  results.report("Ruby '.'") { TESTS.times {  File.expand_path('.') } }
  results.report("Fenix '.'") { TESTS.times { Fenix::File.expand_path('.') } }
  results.report("Plain '.'") { TESTS.times { Fenix::File.expand_path_plain('.') } }

  results.report("Ruby 'foo', 'bar'") { TESTS.times {  File.expand_path('foo', 'bar') } }
  results.report("Fenix 'foo', 'bar'") { TESTS.times { Fenix::File.expand_path('foo', 'bar') } }
  results.report("Plain 'foo', 'bar'") { TESTS.times { Fenix::File.expand_path_plain('foo', 'bar') } }

  results.report("Ruby '', 'C:/'") { TESTS.times {  File.expand_path('', 'C:/') } }
  results.report("Fenix '', 'C:/'") { TESTS.times { Fenix::File.expand_path('', 'C:/') } }
  results.report("Plain '', 'C:/'") { TESTS.times { Fenix::File.expand_path_plain('', 'C:/') } }

  results.report("Ruby 'foo', 'C:/'") { TESTS.times {  File.expand_path('foo', 'C:/') } }
  results.report("Fenix 'foo', 'C:/'") { TESTS.times { Fenix::File.expand_path('foo', 'C:/') } }
  results.report("Plain 'foo', 'C:/'") { TESTS.times { Fenix::File.expand_path_plain('foo', 'C:/') } }

  # So slow, exclude them
  if ENV["SLOW"]
    results.report("Ruby 'D:/Bar', 'C:/Foo'") { TESTS.times {  File.expand_path('D:/Bar', 'C:/Foo') } }
    results.report("Fenix 'D:/Bar', 'C:/Foo'") { TESTS.times { Fenix::File.expand_path('D:/Bar', 'C:/Foo') } }
    results.report("Plain 'D:/Bar', 'C:/Foo'") { TESTS.times { Fenix::File.expand_path_plain('D:/Bar', 'C:/Foo') } }
  end

  results.report("Ruby '~'") { TESTS.times {  File.expand_path('~') } }
  results.report("Fenix '~'") { TESTS.times { Fenix::File.expand_path('~') } }
  results.report("Plain '~'") { TESTS.times { Fenix::File.expand_path_plain('~') } }

  results.report("Ruby '~/foo'") { TESTS.times {  File.expand_path('~/foo') } }
  results.report("Fenix '~/foo'") { TESTS.times { Fenix::File.expand_path('~/foo') } }
  results.report("Plain '~/foo'") { TESTS.times { Fenix::File.expand_path_plain('~/foo') } }

  results.report("Ruby 'foo/'") { TESTS.times {  File.expand_path('foo/') } }
  results.report("Fenix 'foo/'") { TESTS.times { Fenix::File.expand_path('foo/') } }
  results.report("Plain 'foo/'") { TESTS.times { Fenix::File.expand_path_plain('foo/') } }

  results.report("Ruby '~', 'C:/Foo'") { TESTS.times {  File.expand_path('~', 'C:/Foo') } }
  results.report("Fenix '~', 'C:/Foo'") { TESTS.times { Fenix::File.expand_path('~', 'C:/Foo') } }
  results.report("Plain '~', 'C:/Foo'") { TESTS.times { Fenix::File.expand_path_plain('~', 'C:/Foo') } }

  results.report("Ruby long_path") { TESTS.times { File.expand_path(long_path) } }
  results.report("Fenix long_path") { TESTS.times { Fenix::File.expand_path(long_path) } }
  results.report("Plain long_path") { TESTS.times { Fenix::File.expand_path_plain(long_path) } }

  results.report("Ruby long_path, 'rel'") { TESTS.times { File.expand_path(long_path, 'rel') } }
  results.report("Fenix long_path, 'rel'") { TESTS.times { Fenix::File.expand_path(long_path, 'rel') } }
  results.report("Plain long_path, 'rel'") { TESTS.times { Fenix::File.expand_path_plain(long_path, 'rel') } }

  results.report("Ruby long_path, 'C:/Foo'") { TESTS.times { File.expand_path(long_path, 'C:/Foo') } }
  results.report("Fenix long_path, 'C:/Foo'") { TESTS.times { Fenix::File.expand_path(long_path, 'C:/Foo') } }
  results.report("Plain long_path, 'C:/Foo'") { TESTS.times { Fenix::File.expand_path_plain(long_path, 'C:/Foo') } }

  results.report("Ruby full_long_path") { TESTS.times { File.expand_path(full_long_path) } }
  results.report("Fenix full_long_path") { TESTS.times { Fenix::File.expand_path(full_long_path) } }
  results.report("Plain full_long_path") { TESTS.times { Fenix::File.expand_path_plain(full_long_path) } }

  results.report("Ruby to_path") { TESTS.times { File.expand_path(to_path) } }
  results.report("Fenix to_path") { TESTS.times { Fenix::File.expand_path(to_path) } }
  results.report("Plain to_path") { TESTS.times { Fenix::File.expand_path_plain(to_path) } }

  results.report("Ruby to_path, 'rel'") { TESTS.times { File.expand_path(to_path, 'rel') } }
  results.report("Fenix to_path, 'rel'") { TESTS.times { Fenix::File.expand_path(to_path, 'rel') } }
  results.report("Plain to_path, 'rel'") { TESTS.times { Fenix::File.expand_path_plain(to_path, 'rel') } }

  results.report("Ruby to_path, 'C:/Foo'") { TESTS.times { File.expand_path(to_path, 'C:/Foo') } }
  results.report("Fenix to_path, 'C:/Foo'") { TESTS.times { Fenix::File.expand_path(to_path, 'C:/Foo') } }
  results.report("Plain to_path, 'C:/Foo'") { TESTS.times { Fenix::File.expand_path_plain(to_path, 'C:/Foo') } }

  results.report("Ruby full_to_path") { TESTS.times { File.expand_path(full_to_path) } }
  results.report("Fenix full_to_path") { TESTS.times { Fenix::File.expand_path(full_to_path) } }
  results.report("Plain full_to_path") { TESTS.times { Fenix::File.expand_path_plain(full_to_path) } }
end
