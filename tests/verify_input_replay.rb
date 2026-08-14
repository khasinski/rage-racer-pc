#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "tempfile"

tool = File.expand_path(ARGV.fetch(0))
Tempfile.create(["rage-input", ".csv"]) do |file|
  file.write("frame,timer,pad_held\n")
  file.write("100,7,64\n101,8,8256\n102,9,8256\n103,10,0\n")
  file.flush
  output, status = Open3.capture2e("ruby", tool, file.path,
                                  "--timer-origin", "200")
  abort output unless status.success?
  expected = "207:CROSS,208-209:CROSS+RIGHT\n"
  abort "unexpected replay: #{output.inspect}" unless output == expected
end

puts "Manual capture input converts to a deterministic smoke replay"
