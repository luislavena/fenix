require "spec_helper"

describe Fenix::File do
  subject { Fenix::File }
  let(:base) { Dir.pwd }
  let(:tmpdir) { "C:/Temporary" }
  let(:rootdir) { "C:/" }

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

    describe "~" do
      let(:home) { "C:/UserHome" }

      before :each do
        @old_home = ENV["HOME"]
        ENV["HOME"] = home
      end

      after :each do
        ENV["HOME"] = @old_home if @old_home
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
  end
end
