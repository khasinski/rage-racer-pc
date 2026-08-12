#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-visual-budget-") do |root|
  summary = File.join(root, "summary.json")
  File.write(summary, JSON.generate(
    "road" => { "matched_frames" => 2, "maximum_clear" => 0,
                "maximum_black" => 10, "maximum_needle_mismatch" => 0,
                "worst_region_rmse" => 0.05,
                "maximum_black_frame" => { "frame" => "timer-1", "state_delta" => { "x" => 3 } } }
  ))
  passing = [RbConfig.ruby, tool, "--summary", summary,
             "--budget", "road.clear=0", "--budget", "road.black=10",
             "--budget", "road.rmse=0.05", "--budget", "road.matched_min=2"]
  stdout, stderr, status = Open3.capture3(*passing)
  abort stdout + stderr unless status.success?
  validation = JSON.parse(File.read(summary)).fetch("validation")
  abort "passing visual budgets were not recorded" unless validation.fetch("passed")

  failing = [RbConfig.ruby, tool, "--summary", summary, "--budget", "road.black=9"]
  stdout, stderr, status = Open3.capture3(*failing)
  abort "visual budget did not fail above its threshold" if status.success?
  abort "failing frame was not reported" unless stderr.include?("timer-1")
  validation = JSON.parse(File.read(summary)).fetch("validation")
  abort "failing visual budget was not recorded" if validation.fetch("passed")
end

puts "visual budgets enforce per-profile regression thresholds"
