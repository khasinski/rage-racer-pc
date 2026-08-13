#!/usr/bin/env ruby
# frozen_string_literal: true

require "csv"
require "fileutils"
require "open3"
require "tmpdir"

executable = File.expand_path(ARGV.fetch(0))
source_dir = File.expand_path(ARGV.fetch(1))

Dir.mktmpdir("rage-mirror-entry-") do |capture_dir|
  environment = {
    "SDL_AUDIODRIVER" => "dummy",
    "RAGE_PORT_SMOKE_FRAMES" => "1800",
    "RAGE_PORT_DISABLE_HOST_INPUT" => "1",
    "RAGE_PORT_INPUT_SCRIPT" =>
      "400:START,500:START,650:CROSS,1015:CROSS,1165:CROSS,1265:CROSS," \
      "1533-10000:CROSS",
    "RAGE_PORT_SMOKE_CAPTURE_DIR" => capture_dir,
    "RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE" => "1",
    "RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN" => "365",
    "RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX" => "371",
    "RAGE_PORT_SMOKE_CAPTURE_SCENE" => "12",
    "RAGE_PORT_SMOKE_STOP_SCENE" => "12",
    "RAGE_PORT_SMOKE_STOP_SCENE_TIMER" => "371"
  }

  output, status = Open3.capture2e(environment, executable, chdir: source_dir)
  abort output unless status.success?

  rows = CSV.read(File.join(capture_dir, "capture-manifest.csv"), headers: true)
  abort "mirror-entry capture did not cover seven consecutive timers" unless
    rows.map { |row| row["timer"].to_i } == (365..371).to_a

  rows.each do |row|
    path = File.join(capture_dir, row.fetch("filename"))
    data = File.binread(path)
    header = data.match(/\AP6\s+(\d+)\s+(\d+)\s+255\s/m) or abort "invalid PPM: #{path}"
    pixels = data.byteslice(header.end(0)..)
    blue_pixels = pixels.bytes.each_slice(3).count do |red, green, blue|
      blue > 40 && blue > red + 20 && blue > green + 20
    end
    abort "mirror entry flooded frame #{row['timer']} blue (#{blue_pixels} pixels)" if
      blue_pixels > 20_000
  end
end

puts "Mirror entry retained the main scene through every zero-height clip frame"
