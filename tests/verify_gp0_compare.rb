#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-gp0-compare-") do |directory|
  psx = File.join(directory, "psx.log")
  native = File.join(directory, "native.log")
  File.write(psx, <<~LOG)
    noise
    gp0-command chain=1 node=0 packet=0 address=001000 code=e3 length=1 words=e3000000
    gp0-command chain=1 node=0 packet=1 address=001004 code=02 length=3 words=02000000,00000000,00f00140
  LOG
  File.write(native, <<~LOG)
    gp0-packet chain=0 packet=0 code=e3 length=4 words=e3000000,02000000,00000000,00f00140
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")

  File.write(native, <<~LOG)
    gp0-packet chain=0 packet=0 code=e3 length=4 words=e3000000,02000000,00000001,00f00140
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise "different streams passed" if status.success?
  raise output unless output.include?("word=2") && output.include?("00000001")
end

puts "GP0 comparison ignores packet grouping and reports the first changed word"
