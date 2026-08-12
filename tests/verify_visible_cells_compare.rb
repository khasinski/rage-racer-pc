#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-visible-") do |dir|
  File.write("#{dir}/psx.log", "visible-frame frame=1 scene=12 timer=9\nvisible-cell 0=1,2,3,4\nmirror-mask 0=00000001\n")
  File.write("#{dir}/native.log", "visible-frame frame=2 scene=12 timer=9\nvisible-cell 0=1,2,9,4\nmirror-mask 0=00000001\n")
  output, status = Open3.capture2e("ruby", tool, "#{dir}/psx.log", "#{dir}/native.log", "9")
  raise "expected difference" if status.success?
  raise output unless output.include?("records=2 differences=1")
  raise output unless output.include?("membership=0 active_coordinates=1 masks=0")
  raise output unless output.include?("visible-cell 0 psx=1,2,3,4 native=1,2,9,4")
end
puts "visible-cell comparison reports the first exact record mismatch"
