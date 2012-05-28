require "helper"

class TestFenixFile < MiniTest::Unit::TestCase
  def setup
    @base    = Dir.pwd
    @tmpdir  = "C:/Temporary"
    @rootdir = "C:/"
    @drive   = @base[%r'\A(?:[a-z]:|//[^/]+/[^/]+)'i]
  end

  def test_resolve_empty_string_current_directory
    assert_equal @base, Fenix::File.expand_path("")
  end

  def test_resolve_dot_current_directory
    assert_equal @base, Fenix::File.expand_path(".")
  end

  def test_resolve_file_name_relative_current_directory
    assert_equal File.join(@base, "foo"), Fenix::File.expand_path("foo")
  end

  def test_ignore_nil_dir_string
    assert_equal File.join(@base, "foo"), Fenix::File.expand_path("foo", nil)
  end

  def test_resolve_file_name_and_dir_string_relative
    assert_equal File.join(@base, "bar", "foo"),
      Fenix::File.expand_path("foo", "bar")
  end

  # TODO: converts a pathname into absolute pathname [ruby-talk:18512]
  def test_cleanup_dots_file_name
    bug = "[ruby-talk:18512]"

    assert_equal File.join(@base, "a"), Fenix::File.expand_path("a."), bug
    assert_equal File.join(@base, ".a"), Fenix::File.expand_path(".a"), bug

    # FIXME
    assert_equal File.join(@base, "..a"), Fenix::File.expand_path("..a"), bug
  end

  def test_cleanup_dots_end_file_name
    bug = "[ruby-talk:18512]"

    assert_equal File.join(@base, "a"), Fenix::File.expand_path("a.."), bug
  end
end
