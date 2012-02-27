require "spec_helper"

describe Fenix::File do
  subject { Fenix::File }
  let(:base) { Dir.pwd }
  let(:tmpdir) { "C:/Temporary" }
  let(:rootdir) { "C:/" }
  let(:drive) { Dir.pwd[%r'\A(?:[a-z]:|//[^/]+/[^/]+)'i] }

  describe "expand_path" do
    it "converts an empty pathname into absolute current pathname" do
      subject.expand_path("").must_equal base
    end

    it "converts '.' into absolute pathname using current directory" do
      subject.expand_path(".").must_equal base
    end

    it "converts 'foo' into absolute pathname using current directory" do
      subject.expand_path("foo").must_equal File.join(base, "foo")
    end

    it "converts 'foo' into absolute pathname ignoring nil dir" do
      subject.expand_path("foo", nil).must_equal File.join(base, "foo")
    end

    it "converts 'foo' and 'bar' into absolute pathname" do
      subject.expand_path("foo", "bar").must_equal File.join(base, "bar", "foo")
    end

    it "converts a pathname into absolute pathname [ruby-talk:18512]" do
      subject.expand_path("a.").must_equal File.join(base, "a")
      subject.expand_path('.a').must_equal File.join(base, '.a')
      subject.expand_path('..a').must_equal File.join(base, '..a')

      # this spec is not valid, a.. is not a valid file or directory name so
      # b can't be inside of it
      skip "XP doesn't pass the following. Result is '#{base}/a./b'" if windows_xp?
      subject.expand_path('a../b').must_equal File.join(base, 'a../b')
    end

    it "converts a pathname and make it valid [ruby-talk:18512]" do
      # this differs from original RubySpec because you can't create
      # a directory or file with multiple consecutive dots as part of it
      subject.expand_path('a..').must_equal File.join(base, 'a')
    end

    it "converts a pathname to an absolute pathname, using a complete path" do
      subject.expand_path("", "#{tmpdir}").must_equal tmpdir
      subject.expand_path("a", "#{tmpdir}").must_equal File.join(tmpdir, "a")
      subject.expand_path("../a", "#{tmpdir}/xxx").must_equal File.join(tmpdir, "a")
      subject.expand_path(".", "#{rootdir}").must_equal rootdir
    end

    it "ignores supplied dir if path contains a drive letter" do
      subject.expand_path(rootdir, "D:/").must_equal rootdir
    end

    it "removes trailing slashes from absolute path" do
      subject.expand_path("#{rootdir}/foo/").must_equal File.join(rootdir, "foo")
      subject.expand_path("#{rootdir}/foo.rb/").must_equal File.join(rootdir, "foo.rb")
    end

    it "removes trailing spaces from absolute path" do
      subject.expand_path("#{rootdir}/a ").must_equal File.join(rootdir, "a")
    end

    it "removes trailing dots from absolute path" do
      subject.expand_path("#{rootdir}/a.").must_equal File.join(rootdir, "a")
    end

    it "removes trailing invalid ':$DATA' from absolute path" do
      subject.expand_path("#{rootdir}/aaa::$DATA").must_equal File.join(rootdir, "aaa")
      subject.expand_path("#{rootdir}/aa:a:$DATA").must_equal File.join(rootdir, "aa:a")
      subject.expand_path("#{rootdir}/aaa:$DATA").must_equal File.join(rootdir, "aaa:$DATA")
    end

    it "converts a pathname with a drive letter but no slash [ruby-core:31591]" do
      subject.expand_path('c:').must_match /\Ac:\//i
    end

    it "converts a pathname with a drive letter ignoring different drive dir [ruby-core:42177]" do
      subject.expand_path('c:foo', 'd:/bar').must_match /\Ac:\//i
    end

    it "converts a pathname with a drive letter using same drive dir [ruby-core:42177]" do
      subject.expand_path('c:foo', 'c:/bar').must_match %r'\Ac:/bar/foo\z'i
    end

    it "converts a pathname which starts with a slash using dir's drive" do
      subject.expand_path('/foo', "z:/bar").must_match %r"\Az:/foo\z"i
    end

    it "converts a dot with UNC dir" do
      subject.expand_path('.', "//").must_equal "//"
    end

    it "preserves UNC path root" do
      subject.expand_path("//").must_equal "//"
      subject.expand_path("//.").must_equal "//"
      subject.expand_path("//..").must_equal "//"
    end

    it "converts a pathname which starts with a slash using '//host/share'" do
      subject.expand_path('/foo', "//host/share/bar").must_match %r"\A//host/share/foo\z"i
    end

    it "converts a pathname which starts with a slash using a current drive" do
      subject.expand_path('/foo').must_match %r"\A#{drive}/foo\z"i
    end

    describe "~/" do
      let(:home) { "C:/UserHome" }
      let(:home_drive) { nil }
      let(:home_path) { nil }
      let(:user_profile) { nil }

      before :each do
        @old_home = ENV["HOME"]
        @old_home_drive = ENV["HOMEDRIVE"]
        @old_home_path = ENV["HOMEPATH"]
        @old_user_profile = ENV["USERPROFILE"]
        ENV["HOME"] = home
        ENV["HOMEDRIVE"] = home_drive
        ENV["HOMEPATH"] = home_path
        ENV["USERPROFILE"] = user_profile
      end

      after :each do
        ENV["HOME"] = @old_home if @old_home
        ENV["HOMEDRIVE"] = @old_home_drive if @old_home_drive
        ENV["HOMEPATH"] = @old_home_path if @old_home_path
        ENV["USERPROFILE"] = @old_user_profile if @old_user_profile
      end

      it "converts a pathname to an absolute pathname, using ~ (home) as base" do
        subject.expand_path("~").must_equal home
        subject.expand_path("~", "C:/FooBar").must_equal home
        subject.expand_path("~/a", "C:/FooBar").must_equal File.join(home, "a")
      end

      it "does not modify a HOME string argument" do
        str = "~/a"
        subject.expand_path(str).must_equal "#{home}/a"
        str.must_equal "~/a"
      end

      describe "(nil)" do
        let(:home) { nil }
        let(:home_drive) { nil }
        let(:home_path) { nil }
        let(:user_profile) { nil }

        it "raises ArgumentError when home is nothing" do
          proc { subject.expand_path("~") }.must_raise ArgumentError
        end
      end

      describe "(non-absolute)" do
        let(:home) { "." }

        it "raises ArgumentError when having non-absolute home directories" do
          proc { subject.expand_path("~") }.must_raise ArgumentError
        end

        it "raises ArgumentError when having non-absolute home of a specified user" do
          proc { subject.expand_path("~anything") }.must_raise ArgumentError
        end
      end
    end

    describe "~username" do
      it "raises ArgumentError for any supplied username [ruby-core:39597]" do
        proc { subject.expand_path("~anything") }.must_raise ArgumentError
      end
    end

    it "raises a TypeError if not passed a String type" do
      proc { subject.expand_path(1)    }.must_raise TypeError
      proc { subject.expand_path(nil)  }.must_raise TypeError
      proc { subject.expand_path(true) }.must_raise TypeError
    end

    it "expands C:/./dir to C:/dir" do
      subject.expand_path("C:/./dir").must_equal "C:/dir"
    end

    it "does not modify the string argument" do
      str = "./a/b/../c"
      subject.expand_path(str, base).must_equal "#{base}/a/c"
      str.must_equal "./a/b/../c"
    end

    it "returns a String when passed a String subclass" do
      sub = Class.new(String)
      str = sub.new "./a/b/../c"
      path = subject.expand_path(str, base)
      path.must_equal "#{base}/a/c"
      path.must_be_instance_of(String)
    end

    it "accepts objects that have a #to_path method" do
      klass = Class.new { def to_path; "a/b/c"; end }
      obj = klass.new
      subject.expand_path(obj).must_equal "#{base}/a/b/c"
    end

    describe "(shortname)" do
      use_temporary_directory
      let(:tmpdir) { Dir.pwd }
      let(:long_name) { "This is a directory with a long name" }

      before :each do
        Dir.mkdir long_name
        output = `cmd.exe /C DIR /X`
        @shortname = output.split.find { |o| o =~ /^THISIS/ }
      end

      it "do not expand non-existing shortnames" do
        subject.expand_path("SHORT~1").must_include "SHORT~1"
      end

      it "expands a shortname directory into the full version [ruby-core:39504]" do
        if subject == Fenix::File
          subject.expand_path(@shortname, nil, 1).must_include long_name
        else
          subject.expand_path(@shortname).must_include long_name
        end
      end
    end

    describe "encoding" do
      it "expands using path encoding not file system encoding" do
        if Encoding.find("filesystem") == Encoding::CP1251
          a = "#{drive}/\u3042\u3044\u3046\u3048\u304a".encode("cp932")
        else
          a = "#{drive}/\u043f\u0440\u0438\u0432\u0435\u0442".encode("cp1251")
        end
        subject.expand_path(a).must_equal a
      end

      it "removes trailing backslashes unless it's not in multibyte characters" do
        a = "#{drive}/\225\\\\"
        if File::ALT_SEPARATOR == '\\'
          [%W"cp437 #{drive}/\225", %W"cp932 #{drive}/\225\\"]
        else
          [["cp437", a], ["cp932", a]]
        end.each do |cp, expected|
          subject.expand_path(a.dup.force_encoding(cp)).must_equal expected.force_encoding(cp), cp
        end
      end
    end
  end

  describe "realpath" do
    use_temporary_directory
    let(:tmpdir) { Dir.pwd }

    before :each do
      # all this gets removed by 'use_temporary_directory', so don't worry.

      File.write("foo.rb", "# noop")
      Dir.mkdir("bar")
      File.write("bar/baz", "# stuff")
    end

    it "returns the existing and absolute pathname relative to current" do
      skip "implement me"
      subject.realpath("foo.rb").must_equal File.join(tmpdir, "foo.rb")
    end
  end
end
