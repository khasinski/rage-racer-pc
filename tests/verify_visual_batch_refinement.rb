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

def write_pixels(path, width, height, pixels)
  File.binwrite(path, "P6\n#{width} #{height}\n255\n" + pixels.flatten.pack("C*"))
end

def write_needle_frame(path, needle_x, distractor_red)
  pixels = Array.new(16) { [0, 0, 0] }
  pixels[needle_x] = [255, 0, 0]
  pixels[3] = [distractor_red, distractor_red, distractor_red]
  write_pixels(path, 4, 4, pixels)
end

Dir.mktmpdir("rage-visual-all-phases-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  write_ppm(File.join(psx, "timer-00100-f00010-s12.ppm"), 16)
  write_ppm(File.join(psx, "timer-00100-f00011-s12.ppm"), 80)
  write_ppm(File.join(native, "timer-00100-f00900-s12.ppm"), 240)
  write_ppm(File.join(native, "timer-00100-f00901-s12.ppm"), 16)
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "timer"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  frame = JSON.parse(File.read(File.join(output, "summary.json"))).fetch("frames").first
  abort "all-phase timer pairing did not select the matching display phase" unless
    frame.fetch("psx_frame") == "timer-00100-f00010-s12.ppm" &&
    frame.fetch("native_frame") == "timer-00100-f00901-s12.ppm"
end

Dir.mktmpdir("rage-clear-area-") do |root|
  compare = File.expand_path("../tools/rage_visual_compare.rb", __dir__)
  psx = File.join(root, "psx.ppm")
  native = File.join(root, "native.ppm")
  output = File.join(root, "output")
  width = height = 8
  road = [48, 48, 48]
  clear = [0, 0, 53]
  psx_pixels = Array.new(width * height) { road }
  native_pixels = Array.new(width * height) { road.dup }
  (1..6).each { |value| native_pixels[value * width + value] = clear }
  write_pixels(psx, width, height, psx_pixels)
  write_pixels(native, width, height, native_pixels)
  command = [RbConfig.ruby, compare, "--psx", psx, "--native", native,
             "--output", output, "--clear-region", "0,0,8,8",
             "--artifact-radius", "0"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  clear_report = JSON.parse(File.read(File.join(output, "report.json"))).fetch("native_only_clear")
  abort "one-pixel clear contour was ranked as missing geometry" unless
    clear_report.fetch("raw_count") == 6 && clear_report.fetch("count") == 0

  (2..4).each { |y| (2..4).each { |x| native_pixels[y * width + x] = clear } }
  write_pixels(native, width, height, native_pixels)
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  clear_report = JSON.parse(File.read(File.join(output, "report.json"))).fetch("native_only_clear")
  abort "clear-area filter removed the interior of a real hole" unless
    clear_report.fetch("count") > 0
end

Dir.mktmpdir("rage-visual-diagnostic-preset-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  pixels = Array.new(320 * 240) { [48, 48, 48] }
  write_pixels(File.join(psx, filename), 320, 240, pixels)
  write_pixels(File.join(native, filename), 320, 240, pixels)
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "timer", "--preset", "road"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  abort "road preset did not enable its clear-pixel diagnostic" unless
    stdout.include?("native_only_clear=0")
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
    "no state-aligned frame pairs within configured limits"
  )
end

Dir.mktmpdir("rage-visual-animation-tolerance-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  write_ppm(File.join(psx, filename), 16)
  write_ppm(File.join(native, filename), 16)
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  native_state = state.dup
  native_state[-1] = 10
  File.write(File.join(psx, "capture-manifest.csv"),
             header + "\n" + ([filename] + state + [100, "deadbeef"]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             header + "\n" + ([filename] + native_state + [100, "deadbeef"]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--max-anim-timer-delta", "3"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
end

Dir.mktmpdir("rage-visual-projection-gate-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  write_ppm(File.join(psx, filename), 16)
  write_ppm(File.join(native, filename), 16)
  projection_header = header + ",proj_m00,proj_order"
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             projection_header + "\n" + ([filename] + state + [100, "deadbeef", 4096, 0]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             projection_header + "\n" + ([filename] + state + [100, "deadbeef", 4095, 0]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--max-projection-delta", "0"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "projection gate accepted unequal matrices" if status.success?
  abort stdout + stderr unless (stdout + stderr).include?(
    "no state-aligned frame pairs within configured limits"
  )
  abort "projection rejection did not identify its failed gate" unless
    (stdout + stderr).include?("projection=1>0.0")
end

Dir.mktmpdir("rage-visual-mirror-projection-gate-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  write_ppm(File.join(psx, filename), 16)
  write_ppm(File.join(native, filename), 16)
  mirror_header = header + ",mirror_m00"
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             mirror_header + "\n" + ([filename] + state + [100, "deadbeef", 4096]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             mirror_header + "\n" + ([filename] + state + [100, "deadbeef", 4095]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--max-mirror-projection-delta", "0"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "mirror projection gate accepted unequal matrices" if status.success?
  abort stdout + stderr unless (stdout + stderr).include?(
    "mirror_projection=1>0.0"
  )
end

Dir.mktmpdir("rage-visual-visible-cell-gate-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  write_ppm(File.join(psx, filename), 16)
  write_ppm(File.join(native, filename), 16)
  visible_header = header + ",main_visible_hash,mirror_visible_hash"
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             visible_header + "\n" + ([filename] + state + [100, "deadbeef", 1, 2]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             visible_header + "\n" + ([filename] + state + [100, "deadbeef", 1, 3]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position", "--require-visible-cells"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "visible-cell gate accepted unequal mirror masks" if status.success?
  abort stdout + stderr unless (stdout + stderr).include?("mirror_visible_cells")
end

Dir.mktmpdir("rage-visual-gate-before-ranking-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  psx_name = "timer-00100-s12.ppm"
  rejected_name = "timer-00100-f00001-s12.ppm"
  accepted_name = "timer-00101-f00002-s12.ppm"
  [psx_name, rejected_name, accepted_name].each do |name|
    directory = name == psx_name ? psx : native
    write_ppm(File.join(directory, name), 16)
  end
  projection_header = header + ",proj_m00,proj_order"
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             projection_header + "\n" + ([psx_name] + state + [100, "deadbeef", 4096, 0]).join(",") + "\n")
  rejected = state.dup
  rejected[3] = 200
  accepted = state.dup
  accepted[2] = 101
  native_rows = [
    ([rejected_name] + rejected + [100, "deadbeef", 4096, 0]).join(","),
    ([accepted_name] + accepted + [100, "deadbeef", 4095, 0]).join(",")
  ]
  File.write(File.join(native, "capture-manifest.csv"),
             projection_header + "\n" + native_rows.join("\n") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--max-position-distance", "16", "--max-projection-delta", "1"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  frame = JSON.parse(File.read(File.join(output, "summary.json"))).fetch("frames").first
  abort "ranking selected an ineligible state before applying gates" unless
    frame.fetch("native_frame") == accepted_name
end

Dir.mktmpdir("rage-visual-tacho-rpm-gate-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  filename = "timer-00100-s12.ppm"
  write_ppm(File.join(psx, filename), 16)
  write_ppm(File.join(native, filename), 16)
  rpm_header = header + ",tacho_rpm"
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             rpm_header + "\n" + ([filename] + state + [100, "deadbeef", 5000]).join(",") + "\n")
  File.write(File.join(native, "capture-manifest.csv"),
             rpm_header + "\n" + ([filename] + state + [100, "deadbeef", 5100]).join(",") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position",
             "--max-tacho-rpm-delta", "50"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "tachometer RPM gate accepted unequal needle inputs" if status.success?
  abort stdout + stderr unless (stdout + stderr).include?(
    "no state-aligned frame pairs within configured limits"
  )
end

Dir.mktmpdir("rage-visual-needle-refinement-") do |root|
  psx = File.join(root, "psx")
  native = File.join(root, "native")
  output = File.join(root, "output")
  FileUtils.mkdir_p([psx, native])
  psx_name = "timer-00100-s12.ppm"
  wrong_name = "timer-00100-f00001-s12.ppm"
  right_name = "timer-00101-s12.ppm"
  write_needle_frame(File.join(psx, psx_name), 5, 255)
  write_needle_frame(File.join(native, wrong_name), 6, 255)
  write_needle_frame(File.join(native, right_name), 5, 0)
  state = [0, 12, 100, 10, 20, 30, 40, 1, 0, 0, 0, 0, 0, 18,
           10, 20, 30, 0, 0, 0, 0, 0, 7]
  File.write(File.join(psx, "capture-manifest.csv"),
             header + "\n" + ([psx_name] + state + [100, "deadbeef"]).join(",") + "\n")
  native_rows = [wrong_name, right_name].zip([100, 101]).map do |name, timer|
    candidate = state.dup
    candidate[2] = timer
    ([name] + candidate + [100, "deadbeef"]).join(",")
  end
  File.write(File.join(native, "capture-manifest.csv"),
             header + "\n" + native_rows.join("\n") + "\n")
  command = [RbConfig.ruby, tool, "--psx-dir", psx, "--native-dir", native,
             "--output", output, "--match", "position", "--visual-refine", "1",
             "--needle-region", "0,0,4,4"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  frame = JSON.parse(File.read(File.join(output, "summary.json"))).fetch("frames").first
  abort "needle refinement selected the background instead of the needle mask" unless
    frame.fetch("native_frame") == right_name
end

puts "visual refinement separates sampled state from the displayed buffer; state gates precede ranking"
