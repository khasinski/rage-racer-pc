#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "tmpdir"
require "rbconfig"

tool = ARGV.fetch(0, File.expand_path("../tools/rage_geometry_trace_compare.rb", __dir__))
line = "terrain-decision timer=1 index=0 mirror=0 clut=7c00 tpage=0015 " \
       "clip=12,-4 sxy=1,2/3,4/5,6/7,8 bounds=0,320,0,240 " \
       "raw=1024 depth=32 result=submit\n"

Dir.mktmpdir("rage-geometry-trace-") do |root|
  psx = File.join(root, "psx.log")
  native = File.join(root, "native.log")
  File.write(psx, line)
  File.write(native, line.sub("clip=12,-4", "clip=13,-5"))
  _out, _err, status = Open3.capture3(RbConfig.ruby, tool,
                                      "--clip-tolerance", "1", psx, native)
  abort "trace comparator rejected an allowed numeric delta" unless status.success?

  File.write(native, line.sub("result=submit", "result=reject reason=backface"))
  _out, err, status = Open3.capture3(RbConfig.ruby, tool, psx, native)
  abort "trace comparator accepted a submit/reject divergence" if status.success?
  abort "trace comparator did not identify the first record" unless
    err.include?("first geometry divergence at record 0")
end

puts "geometry trace comparator distinguishes numeric drift from semantic divergence"
