#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-gpu-digest-") do |root|
  alignment = File.join(root, "alignment.json")
  psx = File.join(root, "psx.log")
  native = File.join(root, "native.log")
  output = File.join(root, "result.json")
  File.write(alignment, JSON.generate({ frames: [{
    frame: "pair", psx_frame: "timer-00100-f00012-s12.ppm",
    native_frame: "timer-00100-f00999-s12.ppm"
  }] }))
  File.write(psx, "gpu-digest frame=12 count=3 hash=0123456789abcdef codes=24:2,2c:1\n")
  File.write(native, "gpu-digest frame=999 count=4 hash=fedcba9876543210 codes=24:1,2c:3\n")
  stdout, stderr, status = Open3.capture3(RbConfig.ruby, tool,
    "--alignment", alignment, "--psx-log", psx, "--native-log", native,
    "--output", output)
  abort stdout + stderr unless status.success?
  result = JSON.parse(File.read(output))
  row = result.fetch("frames").first
  abort "digest comparator did not use manifest-relative frame numbers" unless
    result.fetch("comparable_frames") == 1 && row.fetch("count_delta") == 1
  abort "digest comparator lost primitive-code deltas" unless
    row.fetch("code_count_delta") == { "24" => -1, "2c" => 2 }
end
