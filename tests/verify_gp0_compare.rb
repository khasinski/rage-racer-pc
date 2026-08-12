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
    gp0-command frame=7 chain=1 node=0 packet=0 address=001000 code=e3 length=1 words=e3000000
    gp0-command frame=7 chain=1 node=0 packet=1 address=001004 code=02 length=3 words=02000000,00000000,00f00140
  LOG
  File.write(native, <<~LOG)
    gp0-packet frame=9 chain=0 packet=0 code=e3 length=4 words=e3000000,02000000,00000000,00f00140
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")
  output, status = Open3.capture2e(
    RbConfig.ruby, tool, "--psx", psx, "--native", native,
    "--psx-frame", "7", "--native-frame", "9"
  )
  raise output unless status.success? && output.include?("gp0 streams equal")

  File.write(psx, "gp0-command frame=7 code=38 length=8 " \
                  "words=38010203,00000000,aa040506,00000000," \
                  "bb070809,00000000,cc0a0b0c,00000000\n")
  File.write(native, "gp0-packet frame=9 code=38 length=8 " \
                     "words=38010203,00000000,11040506,00000000," \
                     "22070809,00000000,330a0b0c,00000000\n")
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")
  _, raw_status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                  "--native", native, "--raw")
  raise "raw comparison ignored G4 padding" if raw_status.success?

  # PSY-Q structs retain ignored high bytes beside UV2/UV3.  They differ with
  # compiler/register history but never reach a PS1 GPU field.
  File.write(psx, "gp0-command frame=7 code=2c length=9 " \
                  "words=2c808080,0,0,0,0,0,abcd7f80,0,12347fc0\n")
  File.write(native, "gp0-packet frame=9 code=2c length=9 " \
                     "words=2c808080,0,0,0,0,0,56787f80,0,9abc7fc0\n")
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")

  File.write(psx, <<~LOG)
    gp0-command frame=7 code=e3 length=1 words=e3000000
    gp0-command frame=7 code=02 length=3 words=02000000,00000000,00f00140
  LOG
  File.write(native, <<~LOG)
    gp0-packet frame=9 chain=0 packet=0 code=e3 length=4 words=e3000000,02000000,00000001,00f00140
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise "different streams passed" if status.success?
  raise output unless output.include?("word=2") && output.include?("00000001")
end

puts "GP0 comparison ignores packet grouping and reports the first changed word"
