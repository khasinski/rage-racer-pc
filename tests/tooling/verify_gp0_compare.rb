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

  # Raw textured commands ignore RGB modulation; stale packet bytes must not
  # create a visual mismatch in the oracle.
  File.write(psx, "gp0-command frame=7 code=65 length=4 " \
                  "words=65010203,00100020,00040005,00080008\n")
  File.write(native, "gp0-packet frame=9 code=65 length=4 " \
                     "words=65a0b0c0,00100020,00040005,00080008\n")
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")

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

  File.write(psx, "gp0-command frame=7 code=2c length=3 " \
                  "words=2c565656,0069007b,001e427e\n")
  File.write(native, <<~LOG)
    gp0-packet frame=9 code=2c length=3 words=2c575757,0069007b,001e427e
    gp0-packet frame=9 code=2c length=3 words=2c565656,0069007b,001e427e
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise "variant stream unexpectedly equal" if status.success?
  raise output unless output.include?("same-payload variants=") &&
                      output.include?("word0=2c565656") &&
                      output.include?("word0=2c575757")

  File.write(psx, <<~LOG)
    gp0-command frame=7 packet=0 code=28 length=1 words=28000001
    gp0-command frame=7 packet=1 code=28 length=1 words=28000002
    gp0-command frame=7 packet=2 code=28 length=1 words=28000003
  LOG
  File.write(native, <<~LOG)
    gp0-packet frame=9 packet=0 code=28 length=1 words=28000001
    gp0-packet frame=9 packet=1 code=28 length=1 words=280000ff
    gp0-packet frame=9 packet=2 code=28 length=1 words=28000002
    gp0-packet frame=9 packet=3 code=28 length=1 words=28000003
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native,
                                   "--resync-window", "4")
  raise "extra packet stream unexpectedly equal" if status.success?
  raise output unless output.include?("resync: kind=native-extra") &&
                      output.include?("psx_skip=0 native_skip=1")

  File.write(psx, <<~LOG)
    gp0-command frame=7 code=e3 length=1 words=e3000000
    gp0-command frame=7 code=28 length=5 words=28000000,00100010,00200020,00300030,00400040
  LOG
  File.write(native, <<~LOG)
    gp0-packet frame=9 code=e3 length=1 words=e303c000
    gp0-packet frame=9 code=28 length=5 words=28000000,00100010,00200020,00300030,00400040
  LOG
  output, status = Open3.capture2e(RbConfig.ruby, tool, "--psx", psx,
                                   "--native", native)
  raise output unless status.success? && output.include?("gp0 streams equal")
end

puts "GP0 comparison reports changed words and locally resynchronizes packets"
