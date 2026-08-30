#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-vram-delta-") do |dir|
  before = Array.new(1024 * 512, 0)
  after = before.dup
  after[390 * 1024 + 50] = 0x9084
  File.binwrite("#{dir}/before.vram", before.pack("v*"))
  File.binwrite("#{dir}/after.vram", after.pack("v*"))
  output, status = Open3.capture2e("ruby", tool, "#{dir}/before.vram",
                                  "#{dir}/after.vram", "50,390")
  raise output unless status.success?
  raise output unless output.include?("changed=1 bounds=50,390,50,390")
  raise output unless output.include?("before=0000 after=9084 changed=1")
end
puts "VRAM delta reports exact packet bounds and pixel values"
