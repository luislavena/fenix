require "fileutils"
require "tmpdir"

module Support
  module TemporaryDirectory
    def self.included(example_group)
      example_group.extend self
    end

    def use_temporary_directory(prefix = nil)
      before :each do
        @pwd = Dir.pwd
        @tmp_dir = Dir.mktmpdir(prefix)
        Dir.chdir @tmp_dir
      end

      after :each do
        Dir.chdir @pwd
        FileUtils.rm_rf @tmp_dir
      end
    end
  end
end

MiniTest::Spec.send(:include, Support::TemporaryDirectory)
