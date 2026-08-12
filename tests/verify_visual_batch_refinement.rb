#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)

def write_ppm(path, red)
  File.binwrite(path, "P6\n1 1\n255\n" + [red, 0, 0].pack("C3"))
end

header = %w[
  filename frame scene timer x z speed progress lap body_yaw body_pitch
  body_roll track_lateral model_yaw mirror_y view_x view_y view_z
  view_angle_x view_angle_y view_angle_z environment_mode4
  scratch_env_mode4 random_seed anim_timer rival0_raw
].join(",")

Dir.mktmpdir("rage-visual-refinement-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  write_ppm(File.join(psx, "timer-00100-s12.ppm"), 16)
  write_ppm(File.join(native, "timer-00099-s12.ppm"), 16)
  write_ppm(File.join(native, "timer-00100-s12.ppm"), 240)

  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             header + "\n" + (["timer-00100-s12.ppm"] + state + [100, "deadbeef"]).join(",") + "\n")
  previous_state = state.dup
  previous_state[2] = 99
  native_rows = [
    (["timer-00099-s12.ppm"] + previous_state + [99, "deadbeef"]).join(","),
    (["timer-00100-s12.ppm"] + state + [100, "deadbeef"]).join(",")
  ]
  File.write(File.join(native, "capture-manifest.csv"),
             header + "\n" + native_rows.join("\n") + "\n")

  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position", "--visual-refine", "1"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  frame = JSON.parse(File.read(File.join(output, "summary.json"))).fetch("frames").first
  abort "refinement did not select the displayed previous buffer" unless
    frame.fetch("native_frame") == "timer-00099-s12.ppm"
  delta = frame.fetch("state_delta")
  abort "state alignment moved with the selected display buffer" unless
    delta.fetch("timer") == 0 && delta.fetch("anim_timer") == 0
  abort "display timer delta was not recorded" unless delta.fetch("display_timer") == -1
end

Dir.mktmpdir("rage-visual-seed-gate-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  write_ppm(File.join(psx, "timer-00100-s12.ppm"), 16)
  write_ppm(File.join(native, "timer-00100-s12.ppm"), 16)
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             header + "\n" + (["timer-00100-s12.ppm"] + state + [100, "deadbeef"]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             header + "\n" + (["timer-00100-s12.ppm"] + state + [101, "deadbeef"]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--require-random-seed"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "seed gate accepted unequal RNG states" if status.success?
  abort stdout + stderr unless (stdout + stderr).include?(
    "no state-aligned frame pairs within the position limit"
  )
end

puts "visual refinement separates sampled state from the displayed buffer; RNG gate rejects divergent states"
