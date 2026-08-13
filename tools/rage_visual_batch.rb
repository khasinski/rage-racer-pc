#!/usr/bin/env ruby
# frozen_string_literal: true

# Pair timer-named emulator/native captures and build per-frame visual bundles.
# The emulator sequence is expensive and can remain cached while the native
# directory is replaced after each renderer change.

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"
require "rbconfig"
require "etc"

REGION_PRESETS = {
  "road" => "0,55,250,150",
  "mirror" => "84,16,152,40",
  "mirror-road" => "84,40,152,16",
  "mirror-frame" => "80,12,160,48",
  "rank" => "0,0,84,64",
  "record" => "236,0,84,64",
  "tacho" => "225,145,95,95",
  "time" => "0,188,125,52",
  "hud" => "0,176,320,64"
}.freeze
DIAGNOSTIC_PRESETS = {
  "road" => { clear_region: "0,100,250,100", black_region: "0,55,250,145",
              rank: "clear", require_main_visible_cells: true },
  "mirror" => { dynamic_mirror_region: "full",
                rank: "clear", require_mirror_visible_cells: true },
  "mirror-road" => { dynamic_mirror_region: "lower",
                rank: "clear", require_mirror_visible_cells: true },
  "tacho" => { needle_region: "250,160,45,42", rank: "needle" },
  "hud" => { black_region: "0,176,320,64" }
}.freeze

options = { region: nil, hotspots: 8, radius: 12, match: "timer",
            max_position_distance: 64.0, max_view_distance: 32.0,
            max_speed_delta: 16, max_angle_delta: 32,
            max_lateral_delta: 32, max_rival_distance: Float::INFINITY,
            max_projection_delta: Float::INFINITY,
            max_mirror_projection_delta: Float::INFINITY,
            max_tacho_rpm_delta: Float::INFINITY,
            require_tacho_flash: false,
            require_rival_render_state: false,
            max_display_timer_delta: Float::INFINITY,
            max_timer_delta: Float::INFINITY,
            max_anim_timer_delta: 0,
            require_scenery_variants: true,
            visual_refine: 0,
            require_random_seed: false,
            require_visible_cells: false,
            require_main_visible_cells: false,
            require_mirror_visible_cells: false,
            require_main_visible_list: false,
            require_mirror_visible_list: false,
            clear_region: nil, black_region: nil, needle_region: nil,
            artifact_radius: 2, rank: "rmse",
            skip_unmatched: false, alignment_only: false,
            jobs: [Etc.nprocessors, 8].min, top: nil }
explicit_options = {}
OptionParser.new do |parser|
  parser.banner = "usage: rage_visual_batch.rb --psx-dir DIR --native-dir DIR --output DIR [options]"
  parser.on("--psx-dir DIR") { |value| options[:psx_dir] = value }
  parser.on("--native-dir DIR") { |value| options[:native_dir] = value }
  parser.on("--output DIR") { |value| options[:output] = value }
  parser.on("--region X,Y,W,H") { |value| options[:region] = value }
  parser.on("--preset NAME", REGION_PRESETS.keys,
            "named region: #{REGION_PRESETS.keys.join(', ')}") do |value|
    options[:region] = REGION_PRESETS.fetch(value)
    options[:preset] = value
  end
  parser.on("--clear-region X,Y,W,H") do |value|
    options[:clear_region] = value
    explicit_options[:clear_region] = true
  end
  parser.on("--black-region X,Y,W,H") do |value|
    options[:black_region] = value
    explicit_options[:black_region] = true
  end
  parser.on("--needle-region X,Y,W,H") do |value|
    options[:needle_region] = value
    explicit_options[:needle_region] = true
  end
  parser.on("--artifact-radius N", Integer) do |value|
    options[:artifact_radius] = value
  end
  parser.on("--hotspots N", Integer) { |value| options[:hotspots] = value }
  parser.on("--hotspot-radius N", Integer) { |value| options[:radius] = value }
  parser.on("--match MODE", %w[timer position],
            "pair by identical timer filename or nearest runtime position") do |value|
    options[:match] = value
  end
  parser.on("--max-position-distance N", Float,
            "skip position matches farther apart than N world units") do |value|
    options[:max_position_distance] = value
  end
  parser.on("--max-view-distance N", Float,
            "skip camera positions farther apart than N world units") do |value|
    options[:max_view_distance] = value
  end
  parser.on("--max-speed-delta N", Integer,
            "skip states whose car speeds differ by more than N") do |value|
    options[:max_speed_delta] = value
  end
  parser.on("--max-angle-delta N", Integer,
            "skip states whose body pitch/roll/yaw differ by more than N") do |value|
    options[:max_angle_delta] = value
  end
  parser.on("--max-lateral-delta N", Integer,
            "skip track lateral offsets farther apart than N") do |value|
    options[:max_lateral_delta] = value
  end
  parser.on("--max-rival-distance N", Float,
            "skip matches whose four rivals exceed this aggregate X/Z distance") do |value|
    options[:max_rival_distance] = value
  end
  parser.on("--max-projection-delta N", Float,
            "skip states whose projection fingerprint differs by more than N") do |value|
    options[:max_projection_delta] = value
  end
  parser.on("--max-mirror-projection-delta N", Float,
            "skip states whose rear-view matrix differs by more than N") do |value|
    options[:max_mirror_projection_delta] = value
  end
  parser.on("--max-tacho-rpm-delta N", Float,
            "skip states whose displayed tachometer RPM differs by more than N") do |value|
    options[:max_tacho_rpm_delta] = value
    explicit_options[:max_tacho_rpm_delta] = true
  end
  parser.on("--require-tacho-flash",
            "require the same tachometer needle flash phase") do
    options[:require_tacho_flash] = true
  end
  parser.on("--require-rival-render-state",
            "require all 11 cars' canonical render state to match") do
    options[:require_rival_render_state] = true
  end
  parser.on("--max-timer-delta N", Integer,
            "skip state matches farther than N game-timer ticks") do |value|
    options[:max_timer_delta] = value
  end
  parser.on("--max-anim-timer-delta N", Integer,
            "allow this wrapped 7-bit animation-phase distance (default: 0)") do |value|
    options[:max_anim_timer_delta] = value
  end
  parser.on("--ignore-scenery-variants",
            "allow different animated-scenery variants (static/HUD diagnosis only)") do
    options[:require_scenery_variants] = false
  end
  parser.on("--require-random-seed",
            "reject state pairs whose RNG seeds differ") do
    options[:require_random_seed] = true
  end
  parser.on("--require-visible-cells",
            "reject pairs whose main or mirror visible-cell masks differ") do
    options[:require_visible_cells] = true
  end
  parser.on("--require-main-visible-cells",
            "reject pairs whose main-pass visible-cell masks differ") do
    options[:require_main_visible_cells] = true
    explicit_options[:require_main_visible_cells] = true
  end
  parser.on("--require-mirror-visible-cells",
            "reject pairs whose mirror-pass visible-cell masks differ") do
    options[:require_mirror_visible_cells] = true
    explicit_options[:require_mirror_visible_cells] = true
  end
  parser.on("--require-main-visible-list",
            "reject pairs whose complete main visible-cell lists differ") do
    options[:require_main_visible_list] = true
  end
  parser.on("--require-mirror-visible-list",
            "reject pairs whose complete mirror visible-cell lists differ") do
    options[:require_mirror_visible_list] = true
  end
  parser.on("--visual-refine N", Integer,
            "choose the lowest-RMSE native image within N timer ticks") do |value|
    options[:visual_refine] = value
  end
  parser.on("--max-display-timer-delta N", Integer,
            "limit the presented native frame's timer offset from PSX") do |value|
    options[:max_display_timer_delta] = value
  end
  parser.on("--skip-unmatched",
            "skip PSX rows with no eligible native state; report every rejection") do
    options[:skip_unmatched] = true
  end
  parser.on("--alignment-only",
            "write eligible state pairs without running image comparisons") do
    options[:alignment_only] = true
  end
  parser.on("--rank METRIC", %w[rmse clear black needle surface],
            "rank output by RMSE, clear, black, needle, or surface mismatch") do |value|
    options[:rank] = value
    explicit_options[:rank] = true
  end
  parser.on("--jobs N", Integer, "parallel frame comparisons") do |value|
    options[:jobs] = value
  end
  parser.on("--top N", Integer, "print only the N worst frames") do |value|
    options[:top] = value
  end
end.parse!

DIAGNOSTIC_PRESETS.fetch(options[:preset], {}).each do |key, value|
  options[key] = value unless explicit_options[key]
end

abort "--psx-dir, --native-dir and --output are required" unless
  options.values_at(:psx_dir, :native_dir, :output).all?
abort "--jobs must be positive" unless options[:jobs].positive?
abort "--top must be positive" if options[:top] && !options[:top].positive?

psx_dir = Pathname(options[:psx_dir]).expand_path
native_dir = Pathname(options[:native_dir]).expand_path
output = Pathname(options[:output]).expand_path
FileUtils.mkdir_p(output)

def manifest_rows(directory)
  path = directory / "capture-manifest.csv"
  abort "missing capture manifest: #{path}" unless path.file?
  lines = File.readlines(path, chomp: true)
  header = lines.shift&.split(",")&.map(&:to_sym)
  abort "empty capture manifest: #{path}" unless header
  lines.map do |line|
    values = line.split(",", -1)
    row = header.zip(values).to_h
    # Manifests may carry opaque diagnostic payloads (for example complete
    # car structs encoded as hex). Matching only consumes scalar state fields;
    # keep every other column verbatim so extending capture diagnostics cannot
    # break the visual pipeline.
    numeric_fields = %i[
      frame scene timer x z speed progress lap body_yaw body_pitch body_roll
      track_lateral model_yaw mirror_y view_x view_y view_z view_angle_x
      view_angle_y view_angle_z environment_mode4 scratch_env_mode4
      proj_m00 proj_m01 proj_m02 proj_m10 proj_m11 proj_m12 proj_m20 proj_m21
      proj_m22 proj_x0 proj_y0 proj_x1 proj_y1 proj_order
      mirror_m00 mirror_m01 mirror_m02 mirror_m10 mirror_m11 mirror_m12
      mirror_m20 mirror_m21 mirror_m22
      main_visible_hash mirror_visible_hash main_visible_list_hash mirror_visible_list_hash
      random_seed anim_timer rival0_x rival0_z rival1_x rival1_z rival2_x
      tacho_rpm tacho_needle_flash
      camera_view_mode camera_button
      course_index grand_prix_class grand_prix_mode player_car_index
      texture_page_wanted texture_cursor_row texture_target_row
      course_object_count course_objects_hash
      anim_scenery_variant anim_scenery2_variant
      rival_render_hash drawable_rival_count
      rival2_z rival3_x rival3_z rival0_speed rival0_progress rival0_yaw
      rival0_lateral rival0_collision rival0_active rival1_speed
      rival1_progress rival1_yaw rival1_lateral rival1_collision rival1_active
      rival2_speed rival2_progress rival2_yaw rival2_lateral rival2_collision
      rival2_active rival3_speed rival3_progress rival3_yaw rival3_lateral
      rival3_collision rival3_active
    ]
    numeric_fields.each do |key|
      row[key] = Integer(row[key]) if row.key?(key) && !row[key].empty?
    end
    %i[random_seed main_visible_hash mirror_visible_hash
       main_visible_list_hash mirror_visible_list_hash course_objects_hash
       rival_render_hash].each do |key|
      row[key] &= 0xffff_ffff if row.key?(key)
    end
    image = directory / row[:filename]
    image.file? ? row.merge(path: image) : nil
  end.compact
end

def capture_surface(rows, label)
  surfaces = rows.map { |row| row[:capture_surface] }.compact.uniq
  abort "#{label} manifest mixes capture surfaces: #{surfaces.join(', ')}" if surfaces.length > 1
  surfaces.first
end

def ppm_capture_stats(path, threshold = 8)
  data = File.binread(path)
  header_end = data.index("\n255\n")
  return { pixels: 0, nonblack: 0 } unless header_end
  pixels = data.byteslice((header_end + 5)..)
  { pixels: pixels.bytesize / 3,
    nonblack: pixels.bytes.each_slice(3).count do |rgb|
      rgb.length == 3 && rgb.max > threshold
    end }
end

def usable_capture?(row)
  # A transient emulator readback can yield a valid 320x240 PPM containing an
  # entirely black framebuffer.  It is not a visual reference: accepting it
  # makes every HUD/road pixel appear to be a native-only artifact.
  stats = ppm_capture_stats(row[:path])
  stats[:pixels] != 320 * 240 || stats[:nonblack].positive?
end

def image_rmse(psx, native, region)
  command = ["magick", "compare", "-metric", "RMSE"]
  if region
    x, y, width, height = region.split(",", 4).map { |part| Integer(part) }
    command.concat(["-extract", "#{width}x#{height}+#{x}+#{y}"])
  end
  command.concat([psx.to_s, native.to_s, "null:"])
  _stdout, metric, status = Open3.capture3(*command)
  abort "visual alignment failed for #{native}: #{metric}" unless
    [0, 1].include?(status.exitstatus)
  metric[/\(([0-9.eE+-]+)\)/, 1]&.to_f || Float::INFINITY
end

def needle_mismatch(psx, native, region)
  left, top, width, height = region.split(",", 4).map { |part| Integer(part) }
  crop = "#{width}x#{height}+#{left}+#{top}"
  mask = "(r>0.392)&&(r>g+0.235)&&(r>b+0.235)"
  command = ["magick",
             "(", psx.to_s, "-alpha", "off", "-crop", crop, "+repage",
             "-fx", mask, ")",
             "(", native.to_s, "-alpha", "off", "-crop", crop, "+repage",
             "-fx", mask, ")",
             "-compose", "difference", "-composite",
             "-format", "%[fx:mean]", "info:"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "needle alignment failed for #{native}: #{stderr}" unless status.success?
  stdout.to_f
end

if options[:match] == "position"
  projection_fields = %i[
    proj_m00 proj_m01 proj_m02 proj_m10 proj_m11 proj_m12
    proj_m20 proj_m21 proj_m22 proj_x0 proj_y0 proj_x1 proj_y1
  ]
  mirror_projection_fields = %i[
    mirror_m00 mirror_m01 mirror_m02 mirror_m10 mirror_m11 mirror_m12
    mirror_m20 mirror_m21 mirror_m22
  ]
  state_metrics = lambda do |candidate, psx|
    dx = candidate[:x] - psx[:x]
    dz = candidate[:z] - psx[:z]
    dvx = candidate[:view_x] - psx[:view_x]
    dvy = candidate[:view_y] - psx[:view_y]
    dvz = candidate[:view_z] - psx[:view_z]
    angle_delta = %i[body_yaw body_pitch body_roll].map do |key|
      next unless candidate.key?(key) && psx.key?(key)
      ((candidate[key] - psx[key] + 2048) % 4096 - 2048).abs
    end.compact.max || 0
    rival_distance = (0...4).sum do |index|
      x_key = :"rival#{index}_x"
      z_key = :"rival#{index}_z"
      next 0 unless candidate.key?(x_key) && psx.key?(x_key)
      Math.hypot(candidate[x_key] - psx[x_key], candidate[z_key] - psx[z_key])
    end
    projection_delta = projection_fields.sum do |key|
      candidate.key?(key) && psx.key?(key) ? (candidate[key] - psx[key]).abs : 0
    end
    mirror_projection_delta = mirror_projection_fields.sum do |key|
      candidate.key?(key) && psx.key?(key) ? (candidate[key] - psx[key]).abs : 0
    end
    {
      dx: dx, dz: dz, position_distance: Math.hypot(dx, dz),
      view_distance: Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz),
      speed_delta: candidate[:speed] - psx[:speed], angle_delta: angle_delta,
      lateral_delta: candidate.key?(:track_lateral) && psx.key?(:track_lateral) ?
        (candidate[:track_lateral] - psx[:track_lateral]).abs : 0,
      rival_distance: rival_distance, projection_delta: projection_delta,
      mirror_projection_delta: mirror_projection_delta,
      tacho_rpm_present: candidate.key?(:tacho_rpm) && psx.key?(:tacho_rpm),
      tacho_rpm_delta: candidate.key?(:tacho_rpm) && psx.key?(:tacho_rpm) ?
        (candidate[:tacho_rpm] - psx[:tacho_rpm]).abs : 0,
      tacho_flash_present: candidate.key?(:tacho_needle_flash) &&
        psx.key?(:tacho_needle_flash),
      tacho_flash_equal: !candidate.key?(:tacho_needle_flash) ||
        !psx.key?(:tacho_needle_flash) ||
        candidate[:tacho_needle_flash] == psx[:tacho_needle_flash],
      rival_render_state_present: candidate.key?(:rival_render_hash) &&
        psx.key?(:rival_render_hash),
      rival_render_state_equal: !candidate.key?(:rival_render_hash) ||
        !psx.key?(:rival_render_hash) ||
        candidate[:rival_render_hash] == psx[:rival_render_hash],
      camera_view_equal: !candidate.key?(:camera_view_mode) ||
        !psx.key?(:camera_view_mode) ||
        candidate[:camera_view_mode] == psx[:camera_view_mode],
      timer_delta: (candidate[:timer] - psx[:timer]).abs,
      anim_timer_delta: if candidate.key?(:anim_timer) && psx.key?(:anim_timer)
                          raw = (candidate[:anim_timer] - psx[:anim_timer]) % 128
                          [raw, 128 - raw].min
                        else
                          0
                        end,
      scenery_variants_equal: !candidate.key?(:anim_scenery_variant) ||
        !psx.key?(:anim_scenery_variant) ||
        candidate[:anim_scenery_variant] == psx[:anim_scenery_variant],
      projection_phase_equal: !candidate.key?(:proj_order) || !psx.key?(:proj_order) ||
        candidate[:proj_order] == psx[:proj_order],
      main_visible_cells_equal: !candidate.key?(:main_visible_hash) ||
        !psx.key?(:main_visible_hash) || candidate[:main_visible_hash] == psx[:main_visible_hash],
      mirror_visible_cells_equal: !candidate.key?(:mirror_visible_hash) ||
        !psx.key?(:mirror_visible_hash) || candidate[:mirror_visible_hash] == psx[:mirror_visible_hash],
      main_visible_list_equal: !candidate.key?(:main_visible_list_hash) ||
        !psx.key?(:main_visible_list_hash) ||
        candidate[:main_visible_list_hash] == psx[:main_visible_list_hash],
      mirror_visible_list_equal: !candidate.key?(:mirror_visible_list_hash) ||
        !psx.key?(:mirror_visible_list_hash) ||
        candidate[:mirror_visible_list_hash] == psx[:mirror_visible_list_hash]
    }
  end
  eligible = lambda do |metrics|
    metrics[:position_distance] <= options[:max_position_distance] &&
      metrics[:view_distance] <= options[:max_view_distance] &&
      metrics[:speed_delta].abs <= options[:max_speed_delta] &&
      metrics[:angle_delta] <= options[:max_angle_delta] &&
      metrics[:lateral_delta] <= options[:max_lateral_delta] &&
      metrics[:rival_distance] <= options[:max_rival_distance] &&
      metrics[:projection_delta] <= options[:max_projection_delta] &&
      metrics[:mirror_projection_delta] <= options[:max_mirror_projection_delta] &&
      (!explicit_options[:max_tacho_rpm_delta] || metrics[:tacho_rpm_present]) &&
      metrics[:tacho_rpm_delta] <= options[:max_tacho_rpm_delta] &&
      (!options[:require_tacho_flash] ||
        (metrics[:tacho_flash_present] && metrics[:tacho_flash_equal])) &&
      (!options[:require_rival_render_state] ||
        (metrics[:rival_render_state_present] &&
         metrics[:rival_render_state_equal])) &&
      metrics[:camera_view_equal] &&
      metrics[:timer_delta] <= options[:max_timer_delta] &&
      metrics[:anim_timer_delta] <= options[:max_anim_timer_delta] &&
      (!options[:require_scenery_variants] || metrics[:scenery_variants_equal]) &&
      (!options[:require_visible_cells] ||
        (metrics[:main_visible_cells_equal] && metrics[:mirror_visible_cells_equal])) &&
      (!options[:require_main_visible_cells] || metrics[:main_visible_cells_equal]) &&
      (!options[:require_mirror_visible_cells] || metrics[:mirror_visible_cells_equal]) &&
      (!options[:require_main_visible_list] || metrics[:main_visible_list_equal]) &&
      (!options[:require_mirror_visible_list] || metrics[:mirror_visible_list_equal]) &&
      metrics[:projection_phase_equal]
  end
  rejection_reasons = lambda do |metrics, candidate, psx|
    reasons = []
    reasons << "position=#{format('%.1f', metrics[:position_distance])}>#{options[:max_position_distance]}" if
      metrics[:position_distance] > options[:max_position_distance]
    reasons << "view=#{format('%.1f', metrics[:view_distance])}>#{options[:max_view_distance]}" if
      metrics[:view_distance] > options[:max_view_distance]
    reasons << "speed=#{metrics[:speed_delta].abs}>#{options[:max_speed_delta]}" if
      metrics[:speed_delta].abs > options[:max_speed_delta]
    reasons << "angle=#{metrics[:angle_delta]}>#{options[:max_angle_delta]}" if
      metrics[:angle_delta] > options[:max_angle_delta]
    reasons << "lateral=#{metrics[:lateral_delta]}>#{options[:max_lateral_delta]}" if
      metrics[:lateral_delta] > options[:max_lateral_delta]
    reasons << "rivals=#{format('%.1f', metrics[:rival_distance])}>#{options[:max_rival_distance]}" if
      metrics[:rival_distance] > options[:max_rival_distance]
    reasons << "projection=#{metrics[:projection_delta]}>#{options[:max_projection_delta]}" if
      metrics[:projection_delta] > options[:max_projection_delta]
    reasons << "mirror_projection=#{metrics[:mirror_projection_delta]}>#{options[:max_mirror_projection_delta]}" if
      metrics[:mirror_projection_delta] > options[:max_mirror_projection_delta]
    reasons << "tacho_rpm=#{metrics[:tacho_rpm_delta]}>#{options[:max_tacho_rpm_delta]}" if
      metrics[:tacho_rpm_delta] > options[:max_tacho_rpm_delta]
    reasons << "missing_tacho_rpm" if explicit_options[:max_tacho_rpm_delta] &&
      !metrics[:tacho_rpm_present]
    reasons << "missing_tacho_flash" if options[:require_tacho_flash] &&
      !metrics[:tacho_flash_present]
    reasons << "tacho_flash" if options[:require_tacho_flash] &&
      metrics[:tacho_flash_present] && !metrics[:tacho_flash_equal]
    reasons << "missing_rival_render_state" if options[:require_rival_render_state] &&
      !metrics[:rival_render_state_present]
    reasons << "rival_render_state" if options[:require_rival_render_state] &&
      metrics[:rival_render_state_present] && !metrics[:rival_render_state_equal]
    reasons << "camera_view" unless metrics[:camera_view_equal]
    reasons << "timer=#{metrics[:timer_delta]}>#{options[:max_timer_delta]}" if
      metrics[:timer_delta] > options[:max_timer_delta]
    reasons << "projection_phase" unless metrics[:projection_phase_equal]
    reasons << "main_visible_cells" if
      (options[:require_visible_cells] || options[:require_main_visible_cells]) &&
      !metrics[:main_visible_cells_equal]
    reasons << "mirror_visible_cells" if
      (options[:require_visible_cells] || options[:require_mirror_visible_cells]) &&
      !metrics[:mirror_visible_cells_equal]
    reasons << "main_visible_list" if options[:require_main_visible_list] &&
      !metrics[:main_visible_list_equal]
    reasons << "mirror_visible_list" if options[:require_mirror_visible_list] &&
      !metrics[:mirror_visible_list_equal]
    reasons << "anim_phase=#{metrics[:anim_timer_delta]}>#{options[:max_anim_timer_delta]}" if
      metrics[:anim_timer_delta] > options[:max_anim_timer_delta]
    reasons << "scenery_variants" if options[:require_scenery_variants] &&
      !metrics[:scenery_variants_equal]
    reasons << "random_seed" if options[:require_random_seed] &&
      candidate.key?(:random_seed) && psx.key?(:random_seed) &&
      candidate[:random_seed] != psx[:random_seed]
    reasons
  end
  psx_states = manifest_rows(psx_dir)
  native_states = manifest_rows(native_dir)
  psx_surface = capture_surface(psx_states, "PSX")
  native_surface = capture_surface(native_states, "native")
  if psx_surface && native_surface && psx_surface != native_surface
    abort "capture surface mismatch: PSX=#{psx_surface}, native=#{native_surface}"
  end
  blank_psx_states, psx_states = psx_states.partition { |state| !usable_capture?(state) }
  abort "capture manifest contains no usable frames" if psx_states.empty? || native_states.empty?
  native_by_scene_lap = native_states.group_by { |state| [state[:scene], state[:lap]] }
  rejected = blank_psx_states.map do |state|
    { psx: state[:filename], native: nil, reasons: ["blank_reference"] }
  end
  pairs = psx_states.map do |psx|
    scene_candidates = native_by_scene_lap.fetch([psx[:scene], psx[:lap]], [])
    if scene_candidates.empty?
      unless options[:skip_unmatched]
        abort "no native state candidate for #{psx[:filename]}"
      end
      rejected << {
        psx: psx[:filename], native: nil,
        reasons: ["no native candidate for scene=#{psx[:scene]} lap=#{psx[:lap]}"]
      }
      next
    end
    candidates = scene_candidates.select do |candidate|
      metrics = state_metrics.call(candidate, psx)
      rejection_reasons.call(metrics, candidate, psx).empty?
    end
    if candidates.empty?
      nearest = scene_candidates.min_by do |candidate|
        metrics = state_metrics.call(candidate, psx)
        metrics[:position_distance] + metrics[:view_distance] * 2 +
          metrics[:speed_delta].abs * 4 + metrics[:angle_delta] +
          metrics[:lateral_delta] + metrics[:projection_delta] * 4
          + metrics[:mirror_projection_delta] * 4
      end
      metrics = state_metrics.call(nearest, psx)
      rejected << {
        psx: psx[:filename], native: nearest[:filename],
        reasons: rejection_reasons.call(metrics, nearest, psx)
      }
      next
    end
    native_state = candidates.min_by do |candidate|
      dx = candidate[:x] - psx[:x]
      dz = candidate[:z] - psx[:z]
      dvx = candidate[:view_x] - psx[:view_x]
      dvy = candidate[:view_y] - psx[:view_y]
      dvz = candidate[:view_z] - psx[:view_z]
      speed = (candidate[:speed] - psx[:speed]).abs
      yaw = ((candidate[:body_yaw] - psx[:body_yaw] + 2048) % 4096 - 2048).abs
      pitch = if candidate.key?(:body_pitch) && psx.key?(:body_pitch)
                ((candidate[:body_pitch] - psx[:body_pitch] + 2048) % 4096 - 2048).abs
              else
                0
              end
      roll = if candidate.key?(:body_roll) && psx.key?(:body_roll)
               ((candidate[:body_roll] - psx[:body_roll] + 2048) % 4096 - 2048).abs
             else
               0
             end
      lateral = if candidate.key?(:track_lateral) && psx.key?(:track_lateral)
                  (candidate[:track_lateral] - psx[:track_lateral]).abs
                else
                  0
                end
      seed_penalty = if candidate.key?(:random_seed) && psx.key?(:random_seed) &&
                        candidate[:random_seed] != psx[:random_seed]
                       512
                     else
                       0
                     end
      phase_penalty = if candidate.key?(:anim_timer) && psx.key?(:anim_timer) &&
                         (candidate[:anim_timer] - psx[:anim_timer]) % 128 != 0
                        1024
                      else
                        0
                      end
      # The VBlank checkpoint can observe the renderer during its mirror pass,
      # whose yaw is the main camera plus exactly 180 degrees.  Modulo 2048
      # compares the underlying view without turning that phase difference
      # into a false state mismatch.
      view_yaw = ((candidate[:view_angle_y] - psx[:view_angle_y] + 1024) % 2048 - 1024).abs
      rival_distance = (0...4).sum do |index|
        x_key = :"rival#{index}_x"
        z_key = :"rival#{index}_z"
        next 0 unless candidate.key?(x_key) && psx.key?(x_key)
        Math.hypot(candidate[x_key] - psx[x_key],
                   candidate[z_key] - psx[z_key])
      end
      projection_delta = %i[
        proj_m00 proj_m01 proj_m02 proj_m10 proj_m11 proj_m12
        proj_m20 proj_m21 proj_m22 proj_x0 proj_y0 proj_x1 proj_y1
      ].sum do |key|
        candidate.key?(key) && psx.key?(key) ? (candidate[key] - psx[key]).abs : 0
      end
      Math.hypot(dx, dz) + Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz) * 2 +
        speed * 4 + yaw / 4.0 + pitch / 2.0 + roll / 2.0 +
        lateral / 4.0 + view_yaw * 2 + rival_distance * 2 +
        seed_penalty + phase_penalty + projection_delta * 4 +
        (candidate[:timer] - psx[:timer]).abs
    end
    native = native_state
    if options[:visual_refine] > 0
      nearby = scene_candidates.select do |candidate|
        (candidate[:timer] - native_state[:timer]).abs <= options[:visual_refine] &&
          (candidate[:timer] - psx[:timer]).abs <= options[:max_display_timer_delta]
      end
      refined = nearby.min_by do |candidate|
        if options[:needle_region]
          needle_mismatch(psx[:path], candidate[:path], options[:needle_region])
        else
          image_rmse(psx[:path], candidate[:path], options[:region])
        end
      end
      native = refined unless refined.nil?
    end
    # The manifest describes the state sampled at VBlank, but its screenshot
    # can still show the preceding front buffer. Keep state validation tied to
    # the simulation match while visual refinement selects that displayed
    # image independently (including its preceding animation phase).
    dx = native_state[:x] - psx[:x]
    dz = native_state[:z] - psx[:z]
    dvx = native_state[:view_x] - psx[:view_x]
    dvy = native_state[:view_y] - psx[:view_y]
    dvz = native_state[:view_z] - psx[:view_z]
    position_distance = Math.hypot(dx, dz)
    view_distance = Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz)
    speed_delta = native_state[:speed] - psx[:speed]
    angle_delta = %i[body_yaw body_pitch body_roll].map do |key|
      next unless native_state.key?(key) && psx.key?(key)
      ((native_state[key] - psx[key] + 2048) % 4096 - 2048).abs
    end.compact.max || 0
    lateral_delta = if native_state.key?(:track_lateral) && psx.key?(:track_lateral)
                      (native_state[:track_lateral] - psx[:track_lateral]).abs
                    else
                      0
                    end
    rival_distance = (0...4).sum do |index|
      x_key = :"rival#{index}_x"
      z_key = :"rival#{index}_z"
      next 0 unless native_state.key?(x_key) && psx.key?(x_key)
      Math.hypot(native_state[x_key] - psx[x_key],
                 native_state[z_key] - psx[z_key])
    end
    projection_delta = projection_fields.sum do |key|
      native_state.key?(key) && psx.key?(key) ?
        (native_state[key] - psx[key]).abs : 0
    end
    projection_phase_equal = %i[proj_order].all? do |key|
      !native_state.key?(key) || !psx.key?(key) || native_state[key] == psx[key]
    end
    {
      psx: psx[:path], native: native[:path],
      label: "#{File.basename(psx[:filename], '.ppm')}__#{File.basename(native[:filename], '.ppm')}",
      dynamic_region: if options[:dynamic_mirror_region] &&
                         psx.key?(:mirror_y) && native.key?(:mirror_y)
                        top = [psx[:mirror_y], native[:mirror_y], 0].max
                        bottom = [psx[:mirror_y] + 36,
                                  native[:mirror_y] + 36, 240].min
                        if options[:dynamic_mirror_region] == "lower"
                          top += (bottom - top) / 2
                        end
                        [86, top, 148, [bottom - top, 0].max].join(",")
                      end,
      state_delta: {
        x: dx, z: dz, distance: position_distance,
        view_distance: view_distance,
        speed: speed_delta,
        angle: angle_delta,
        timer: native_state[:timer] - psx[:timer],
        display_timer: native[:timer] - psx[:timer],
        body_pitch: native_state.key?(:body_pitch) && psx.key?(:body_pitch) ?
          native_state[:body_pitch] - psx[:body_pitch] : nil,
        body_roll: native_state.key?(:body_roll) && psx.key?(:body_roll) ?
          native_state[:body_roll] - psx[:body_roll] : nil,
        track_lateral: native_state.key?(:track_lateral) && psx.key?(:track_lateral) ?
          native_state[:track_lateral] - psx[:track_lateral] : nil,
        rival_distance: rival_distance,
        projection_delta: projection_delta,
        mirror_projection_delta: state_metrics.call(native_state, psx)[:mirror_projection_delta],
        tacho_rpm: native_state.key?(:tacho_rpm) && psx.key?(:tacho_rpm) ?
          native_state[:tacho_rpm] - psx[:tacho_rpm] : nil,
        tacho_flash_equal: native_state.key?(:tacho_needle_flash) &&
          psx.key?(:tacho_needle_flash) ?
          native_state[:tacho_needle_flash] == psx[:tacho_needle_flash] : nil,
        projection_phase_equal: projection_phase_equal,
        main_visible_cells_equal: state_metrics.call(native_state, psx)[:main_visible_cells_equal],
        mirror_visible_cells_equal: state_metrics.call(native_state, psx)[:mirror_visible_cells_equal],
        random_seed_equal: native_state.key?(:random_seed) && psx.key?(:random_seed) ?
          native_state[:random_seed] == psx[:random_seed] : nil,
        anim_timer: native_state.key?(:anim_timer) && psx.key?(:anim_timer) ?
          native_state[:anim_timer] - psx[:anim_timer] : nil
      }
    }
  end.compact
  rejection_counts = rejected.flat_map { |row| row[:reasons] }
    .each_with_object(Hash.new(0)) do |reason, counts|
      counts[reason.sub(/[=>].*/, "")] += 1
    end.sort_by { |reason, count| [-count, reason] }.to_h
  if pairs.empty?
    if options[:alignment_only]
      summary = {
        psx_directory: psx_dir.to_s, native_directory: native_dir.to_s,
        match: options[:match], alignment_only: true, matched_frames: 0,
        rejected_frames: rejected.length, rejection_counts: rejection_counts,
        rejections: rejected, frames: []
      }
      File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
      puts "aligned=0 rejected=#{rejected.length}"
      exit
    end
    details = rejected.first(5).map do |row|
      "#{row[:psx]} -> #{row[:native]}: #{row[:reasons].join(', ')}"
    end
    counts = rejection_counts.map { |reason, count| "#{reason}=#{count}" }.join(", ")
    abort (["no state-aligned frame pairs within configured limits",
            "rejection counts: #{counts}"] + details).join("\n")
  end
else
  timer_key = lambda do |path|
    match = path.basename.to_s.match(/\Atimer-(\d+)(?:-f\d+)?-s(\d+)\.ppm\z/)
    match && [Integer(match[1], 10), Integer(match[2], 10)]
  end
  collect = lambda do |directory|
    directory.glob("timer-*.ppm").each_with_object(Hash.new { |hash, key| hash[key] = [] }) do |path, groups|
      key = timer_key.call(path)
      groups[key] << path if key
    end
  end
  psx_frames = collect.call(psx_dir)
  native_frames = collect.call(native_dir)
  keys = (psx_frames.keys & native_frames.keys).sort
  abort "no matching timer[-fFRAME]-sSCENE captures" if keys.empty?
  pairs = keys.map do |key|
    psx, native = psx_frames.fetch(key).product(native_frames.fetch(key)).min_by do |psx_candidate, native_candidate|
      image_rmse(psx_candidate, native_candidate, options[:region])
    end
    timer, scene = key
    { psx: psx, native: native,
      label: format("timer-%05d-s%02d", timer, scene), state_delta: nil }
  end
end

if options[:alignment_only]
  frames = pairs.map do |pair|
    {
      frame: pair[:label], psx_frame: pair[:psx].basename.to_s,
      native_frame: pair[:native].basename.to_s,
      state_delta: pair[:state_delta]
    }
  end
  summary = {
    psx_directory: psx_dir.to_s, native_directory: native_dir.to_s,
    match: options[:match], alignment_only: true,
    matched_frames: frames.length,
    rejected_frames: defined?(rejected) && rejected ? rejected.length : 0,
    rejection_counts: defined?(rejection_counts) && rejection_counts ? rejection_counts : {},
    rejections: defined?(rejected) && rejected ? rejected : [], frames: frames
  }
  File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
  puts "aligned=#{frames.length} rejected=#{summary[:rejected_frames]}"
  exit
end

compare = Pathname(__dir__) / "rage_visual_compare.rb"
work = Queue.new
pairs.each_with_index { |pair, index| work << [index, pair] }
rows = Array.new(pairs.length)
errors = Queue.new
[options[:jobs], pairs.length].min.times.map do
  Thread.new do
    loop do
      index, pair = work.pop(true)
  frame_output = output / pair[:label]
  command = [RbConfig.ruby, compare.to_s,
             "--psx", pair[:psx].to_s,
             "--native", pair[:native].to_s,
             "--output", frame_output.to_s,
             "--hotspots", options[:hotspots].to_s,
             "--hotspot-radius", options[:radius].to_s,
             "--artifact-radius", options[:artifact_radius].to_s]
  region = pair[:dynamic_region] || options[:region]
  clear_region = pair[:dynamic_region] || options[:clear_region]
  black_region = pair[:dynamic_region] || options[:black_region]
  command.concat(["--region", region]) if region
  command.concat(["--clear-region", clear_region]) if clear_region
  command.concat(["--black-region", black_region]) if black_region
  command.concat(["--needle-region", options[:needle_region]]) if options[:needle_region]
  stdout, stderr, status = Open3.capture3(*command)
      unless status.success?
        errors << "comparison failed for #{pair[:label]}:\n#{stdout}#{stderr}"
        next
      end
  report = JSON.parse(File.read(frame_output / "report.json"))
      psx_state = pair[:psx].sub_ext(".psxstate")
      if psx_state.file?
        FileUtils.cp(psx_state, frame_output / "reference.psxstate")
      end
      psx_frame_number = pair[:psx].basename.to_s[/\-f(\d+)\-s\d+\.ppm\z/, 1]&.to_i
      replay_pre = nil
      replay_frames = nil
      if psx_frame_number
        candidates = psx_dir.glob("*-f*-s*.psxstate").each_with_object([]) do |state, found|
          frame = state.basename.to_s[/\-f(\d+)\-s\d+\.psxstate\z/, 1]&.to_i
          found << [frame, state] if frame && frame < psx_frame_number
        end
        previous = candidates.max_by(&:first)
        if previous
          replay_frames = psx_frame_number - previous[0]
          replay_pre = frame_output / "replay-pre.psxstate"
          FileUtils.cp(previous[1], replay_pre)
        end
      end
      rows[index] = {
    frame: pair[:label],
    psx_frame: pair[:psx].basename.to_s,
    psx_state: psx_state.file? ? (frame_output / "reference.psxstate").to_s : nil,
    psx_replay_pre_state: replay_pre&.to_s,
    psx_replay_frames: replay_frames,
    native_frame: pair[:native].basename.to_s,
    state_delta: pair[:state_delta],
    normalized_rmse: report.fetch("normalized_rmse"),
    normalized_region_rmse: report["normalized_region_rmse"],
    native_only_clear: report.fetch("native_only_clear").fetch("count"),
    native_only_clear_component: report.fetch("native_only_clear").fetch("largest_component"),
    native_only_black: report.fetch("native_only_black").fetch("count"),
    native_only_black_component: report.fetch("native_only_black").fetch("largest_component"),
    surface_divergence: report.fetch("surface_divergence").fetch("count"),
    surface_divergence_component: report.fetch("surface_divergence").fetch("largest_component"),
    tachometer_needle: report.fetch("tachometer_needle"),
    worst_hotspot: report.fetch("hotspots").first,
    bundle: frame_output.to_s
      }
    rescue ThreadError
      break
    rescue StandardError => error
      errors << "comparison worker failed: #{error.full_message}"
    end
  end
end.each(&:join)
abort errors.pop unless errors.empty?

summary = {
  psx_directory: psx_dir.to_s,
  native_directory: native_dir.to_s,
  match: options[:match],
  rank: options[:rank],
  matched_frames: rows.length,
  rejected_frames: defined?(rejected) && rejected ? rejected.length : 0,
  rejection_counts: defined?(rejection_counts) && rejection_counts ? rejection_counts : {},
  rejections: defined?(rejected) && rejected ? rejected : [],
  frames: rows
}
File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
rank_rmse = lambda do |row|
  row[:normalized_region_rmse] || row[:normalized_rmse]
end
ranked = if options[:rank] == "clear"
           rows.sort_by do |row|
             [-row[:native_only_clear], -rank_rmse.call(row).to_f]
           end
         elsif options[:rank] == "black"
           rows.sort_by do |row|
             [-row[:native_only_black_component], -row[:native_only_black],
              -rank_rmse.call(row).to_f]
           end
         elsif options[:rank] == "needle"
           rows.sort_by do |row|
             [-row[:tachometer_needle].fetch("mismatch_pixels"),
              -rank_rmse.call(row).to_f]
           end
         elsif options[:rank] == "surface"
           rows.sort_by do |row|
             [-row[:surface_divergence_component], -row[:surface_divergence],
              -rank_rmse.call(row).to_f]
           end
         else
           rows.sort_by { |row| -rank_rmse.call(row).to_f }
         end
(options[:top] ? ranked.first(options[:top]) : ranked).each do |row|
  hotspot = row[:worst_hotspot]
  location = hotspot ? " hotspot=#{hotspot['x']},#{hotspot['y']}" : ""
  delta = row[:state_delta]
  alignment = delta ? format(" distance=%.1f view_distance=%.1f " \
                             "rival_distance=%.1f projection_delta=%.1f speed_delta=%d " \
                             "anim_delta=%s main_cells=%s mirror_cells=%s " \
                             "timer_delta=%d display_timer_delta=%d",
                             delta[:distance], delta[:view_distance],
                             delta[:rival_distance],
                             delta[:projection_delta],
                             delta[:speed], delta[:anim_timer] || "-",
                             delta[:main_visible_cells_equal].nil? ? "-" : delta[:main_visible_cells_equal],
                             delta[:mirror_visible_cells_equal].nil? ? "-" : delta[:mirror_visible_cells_equal],
                             delta[:timer],
                             delta[:display_timer]) : ""
  clear = options[:clear_region] ?
    " native_only_clear=#{row[:native_only_clear]}" : ""
  black = options[:black_region] ?
    " native_only_black=#{row[:native_only_black]}" \
    " largest_black_component=#{row[:native_only_black_component]}" : ""
  needle = options[:needle_region] ?
    format(" needle_mismatch=%d needle_iou=%s",
           row[:tachometer_needle].fetch("mismatch_pixels"),
           row[:tachometer_needle]["iou"]&.then { |value| "%.3f" % value } || "n/a") : ""
  surface = " surface_divergence=#{row[:surface_divergence]}" \
            " largest_surface_component=#{row[:surface_divergence_component]}"
  metric = rank_rmse.call(row)
  metric_name = row[:normalized_region_rmse] ? "region_RMSE" : "RMSE"
  puts format("%s %s=%.6f%s%s%s%s%s%s", row[:frame], metric_name, metric,
              location, clear, black, needle, surface, alignment)
end
