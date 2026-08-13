#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "optparse"

options = { budgets: [] }
OptionParser.new do |cli|
  cli.banner = "usage: rage_visual_budget.rb --summary PATH --budget PROFILE.METRIC=VALUE [...]"
  cli.on("--summary PATH") { |value| options[:summary] = value }
  cli.on("--budget SPEC", "metrics: clear, black, clear_component, black_component, needle, rmse, matched_min") do |value|
    options[:budgets] << value
  end
end.parse!
abort "--summary and at least one --budget are required" unless
  options[:summary] && !options[:budgets].empty?

metric_fields = {
  "clear" => ["maximum_clear", :maximum],
  "black" => ["maximum_black", :maximum],
  "clear_component" => ["maximum_clear_component", :maximum],
  "black_component" => ["maximum_black_component", :maximum],
  "needle" => ["maximum_needle_mismatch", :maximum],
  "rmse" => ["worst_region_rmse", :maximum],
  "matched_min" => ["matched_frames", :minimum]
}.freeze
frame_fields = {
  "clear" => "maximum_clear_frame", "black" => "maximum_black_frame",
  "clear_component" => "maximum_clear_component_frame",
  "black_component" => "maximum_black_component_frame",
  "needle" => "maximum_needle_frame"
}.freeze

summary = JSON.parse(File.read(options[:summary]))
checks = options[:budgets].map do |spec|
  target, limit_text = spec.split("=", 2)
  profile, metric = target.to_s.split(".", 2)
  abort "invalid visual budget: #{spec}" unless profile && metric && limit_text
  field, direction = metric_fields.fetch(metric) do
    abort "unknown visual budget metric #{metric.inspect}"
  end
  result = summary.fetch(profile) { abort "summary has no profile #{profile.inspect}" }
  actual = result.fetch(field)
  limit = metric == "rmse" ? Float(limit_text) : Integer(limit_text)
  passed = direction == :maximum ? actual <= limit : actual >= limit
  { "budget" => spec, "profile" => profile, "metric" => metric,
    "actual" => actual, "limit" => limit, "passed" => passed,
    "frame" => frame_fields[metric] && result[frame_fields.fetch(metric)] }
end

summary["validation"] = {
  "passed" => checks.all? { |check| check.fetch("passed") }, "checks" => checks
}
File.write(options[:summary], JSON.pretty_generate(summary) + "\n")
failures = checks.reject { |check| check.fetch("passed") }
if failures.empty?
  puts "visual budgets passed (#{checks.length} checks)"
else
  failures.each do |check|
    warn "visual budget failed: #{check['budget']} actual=#{check['actual']} " \
         "frame=#{check.dig('frame', 'frame') || '-'}"
  end
  exit 1
end
