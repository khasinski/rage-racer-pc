#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "open3"
require "optparse"
require "pathname"
require "rbconfig"

root = Pathname(__dir__).parent.expand_path
options = { profiles: [], top: 1, canonical_component_limit: 2,
            reuse_only: false, dry_run: false }
OptionParser.new do |cli|
  cli.banner = "usage: rage_visual_triage.rb --run DIR [options]"
  cli.on("--run DIR", "rage_visual_run.rb output directory") do |value|
    options[:run] = Pathname(value).expand_path
  end
  cli.on("--profile NAME", %w[road mirror-road hud tacho],
         "profile to triage; repeatable (defaults to all)") do |value|
    options[:profiles] << value
  end
  cli.on("--top N", Integer, "candidates per profile (default 1)") do |value|
    options[:top] = value
  end
  cli.on("--canonical-component-limit N", Integer,
         "largest canonical component still classified as pose/packet state") do |value|
    options[:canonical_component_limit] = value
  end
  cli.on("--reuse-only", "classify existing GP0 bundles without running games") do
    options[:reuse_only] = true
  end
  cli.on("--dry-run", "print selected bundle commands without executing") do
    options[:dry_run] = true
  end
end.parse!

abort "--run is required" unless options[:run]
abort "--top must be positive" unless options[:top].positive?
profiles = options[:profiles].empty? ? %w[road mirror-road hud tacho] : options[:profiles].uniq
canonical_regions = {
  "road" => "road", "mirror-road" => "mirror_road",
  "hud" => "hud", "tacho" => "tachometer_needle"
}.freeze
score = lambda do |profile, frame|
  if profile == "tacho"
    frame.dig("tachometer_needle", "mismatch_pixels").to_i
  else
    [frame.fetch("native_only_black_component", 0).to_i,
     frame.fetch("surface_divergence_component", 0).to_i].max
  end
end

bundle_tool = root / "tools/rage_gp0_bundle.rb"
results = []
profiles.each do |profile|
  summary_path = options[:run] / "compare" / profile / "summary.json"
  abort "missing #{summary_path}" unless summary_path.file?
  frames = JSON.parse(summary_path.read).fetch("frames")
  frames.select { |frame| score.call(profile, frame) > options[:canonical_component_limit] }
        .sort_by { |frame| [-score.call(profile, frame), frame.fetch("frame")] }
        .first(options[:top]).each do |frame|
    bundle = Pathname(frame.fetch("bundle"))
    canonical_path = bundle / "gp0-raster/report.json"
    command = [RbConfig.ruby, bundle_tool.to_s, "--bundle", bundle.to_s]
    if !canonical_path.file? && !options[:reuse_only] && !options[:dry_run]
      stdout, stderr, status = Open3.capture3(*command, chdir: root.to_s)
      File.write(bundle / "triage.log", stdout + stderr)
      abort "GP0 triage failed for #{bundle}; see #{bundle / 'triage.log'}" unless
        status.success? || canonical_path.file?
    end
    canonical = canonical_path.file? ? JSON.parse(canonical_path.read) : nil
    region = canonical&.dig("regions", canonical_regions.fetch(profile))
    visual_component = score.call(profile, frame)
    canonical_component = region&.fetch("largest_component", 0)
    classification = if canonical.nil?
                       options[:dry_run] ? "pending" : "missing_canonical_artifacts"
                     elsif canonical_component > options[:canonical_component_limit]
                       "renderer"
                     elsif visual_component > options[:canonical_component_limit]
                       "pose_or_game_packets"
                     else
                       "within_threshold"
                     end
    results << {
      "profile" => profile, "frame" => frame.fetch("frame"),
      "bundle" => bundle.to_s, "visual_component" => visual_component,
      "canonical_significant_pixels" => region&.fetch("significant_pixels", 0),
      "canonical_component" => canonical_component,
      "classification" => classification,
      "command" => command
    }
  end
end

output = { "run" => options[:run].to_s, "component_limit" => options[:canonical_component_limit],
           "results" => results }
File.write(options[:run] / "triage.json", JSON.pretty_generate(output) + "\n") unless options[:dry_run]
results.each do |result|
  puts "#{result['profile']} #{result['frame']} visual=#{result['visual_component']} " \
       "canonical=#{result['canonical_component'] || '-'} #{result['classification']}"
  puts result.fetch("command").join(" ") if options[:dry_run]
end
