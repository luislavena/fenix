require "dl/import"

module Support
  module Kernel32
    extend DL::Importer # Ruby 1.9

    dlload "kernel32"

    typealias "DWORD", "unsigned long"

    extern "DWORD GetVersion(void)"

    def self.os_version
      return @os_version if defined?(@os_version)

      ver, major, minor, build = 0

      ver = self.GetVersion

      # Get Windows version (major, minor)
      # dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
      # dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
      major = (ver & 0xffff) & 0xff
      minor = ((ver & 0xffff) >> 8) & 0xff

      if ver < 0x80000000
        # dwBuild = (DWORD)(HIWORD(dwVersion));
        build = (ver >> 16) & 0xffff
      end

      @os_version = [major, minor, build].join(".")
    end
  end

  module WindowsVersion
    def os_version
      Kernel32.os_version
    end

    def windows_xp?
      !os_version.match(/^5\.1/).nil?
    end
  end
end

MiniTest::Spec.send(:include, Support::WindowsVersion)
