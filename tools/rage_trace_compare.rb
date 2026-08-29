#!/usr/bin/env ruby
# frozen_string_literal: true

# Does a change move the game?
#
# Unit tests pin behaviour against a model of the inputs; this pins it against
# the real ones. It builds two revisions, drives each through the same races
# and the same menu screens, and compares what the cars did and what the
# screens drew.
#
# The control comes first and is not optional: the revision under test is built
# twice and compared with itself. An observable that fails that says nothing
# about the two revisions, and the run stops rather than report a difference
# that is only the compiler. The car traces pass that test; audio call traces
# do not, because SPU voice allocation depends on host timing, which is why
# they are not among the observables here.

require "digest"
require "fileutils"
require "open3"
require "optparse"
require "pathname"
require "tmpdir"

ROOT = Pathname.new(__dir__).parent.expand_path

# The builds are kept by default: they are the expensive part, and the next
# comparison usually shares at least one of the two revisions.
options = { races: true, menus: true, clean: false, jobs: nil,
            work: Pathname.new("/tmp/rage-trace-compare") }
OptionParser.new do |parser|
  parser.banner = "usage: rage_trace_compare.rb --base REV [--head REV] [options]"
  parser.on("--base REV", "Revision to compare against") { |v| options[:base] = v }
  parser.on("--head REV", "Revision under test (default: HEAD)") { |v| options[:head] = v }
  parser.on("--no-races", "Skip the race traces") { options[:races] = false }
  parser.on("--no-menus", "Skip the menu screens") { options[:menus] = false }
  parser.on("--work DIR", "Where to put worktrees and builds") { |v| options[:work] = Pathname.new(v) }
  parser.on("--jobs N", Integer, "Parallel build jobs") { |v| options[:jobs] = v }
  parser.on("--clean", "Remove the worktrees and builds afterwards") { options[:clean] = true }
end.parse!

abort "rage_trace_compare.rb: --base is required" unless options[:base]
options[:head] ||= "HEAD"

# Four races and both menu sweeps. The races differ in class and course so that
# a change confined to one of them still shows, and one is driven to a real
# finish so that the lap and finish code runs to its end.
RACES = [
  { name: "class4-course0", klass: 4, course: 0, frames: 5000 },
  { name: "class4-course1", klass: 4, course: 1, frames: 5000 },
  { name: "class2-course2", klass: 2, course: 2, frames: 5000 },
  { name: "class4-finish",  klass: 4, course: 0, frames: 3500, finish: 2000 },
].freeze

MENUS = { "frontend" => "RAGE_PORT_SMOKE_MENU_SWEEP",
          "options" => "RAGE_PORT_SMOKE_OPTION_SWEEP" }.freeze

def run!(command, chdir: ROOT)
  output, status = Open3.capture2e(*command, chdir: chdir.to_s)
  return output if status.success?

  warn output
  abort "rage_trace_compare.rb: #{command.join(' ')} failed"
end

# A worktree of its own per build, so the working copy is never checked out
# from under whoever is running this.
def build(revision, tag, work, jobs)
  tree = work / "tree-#{tag}"
  build_dir = work / "build-#{tag}"
  if tree.directory?
    # Kept from a previous run, which may have been comparing something else.
    run!(["git", "checkout", "--detach", "--force", revision], chdir: tree)
  else
    run!(["git", "worktree", "add", "--detach", tree.to_s, revision])
  end
  run!(["git", "submodule", "update", "--init", "--recursive"], chdir: tree)
  run!(["cmake", "-S", tree.to_s, "-B", build_dir.to_s, "-DCMAKE_BUILD_TYPE=Release"])
  make = ["cmake", "--build", build_dir.to_s, "--target", "rage-racer-smoke", "--parallel"]
  make << jobs.to_s if jobs
  run!(make)
  build_dir / "rage-racer-smoke"
end

# What a race did: where every car was, how fast, which way round and whether
# it was touching anything, every frame, plus the player's own physics and the
# lap the race ended on.
def race_trace(executable, race)
  environment = {
    "SDL_AUDIODRIVER" => "dummy",
    "RAGE_PORT_DISABLE_HOST_INPUT" => "1",
    "RAGE_PORT_SYNC_RANDOM" => "1",
    "RAGE_PORT_CAR_STATE_TRACE" => "1",
    "RAGE_PORT_CAR_MOTION_TRACE" => "1",
    "RAGE_PORT_INPUT_SCRIPT" => "200-9000:CROSS",
  }
  if race[:finish]
    environment["RAGE_PORT_SMOKE_FINISH_FRAME"] = race[:finish].to_s
    environment["RAGE_PORT_SMOKE_FRAMES"] = race[:frames].to_s
  end
  arguments = [executable.to_s, "--scenario", "race-scenario.ini",
               "--set", "race.class=#{race[:klass]}",
               "--set", "race.course=#{race[:course]}",
               "--set", "run.frames=#{race[:frames]}",
               "--set", "video.renderer=classic"]
  summary, trace, status = Open3.capture3(environment, *arguments, chdir: ROOT.to_s)
  warn "rage_trace_compare.rb: #{race[:name]} exited #{status.exitstatus}" unless status.success?
  lines = trace.lines.grep(/\A(car-state|car-motion) /)
  lines + summary.scan(/race_phase=[0-9-]+ progress=[0-9-]+ lap=[0-9-]+/).map { |m| "#{m}\n" }
end

# What a menu drew: every screen the sweep visits, a frame every twenty-five
# ticks, by content rather than by pixel count.
def menu_trace(executable, sweep)
  Dir.mktmpdir("rage-menu-") do |directory|
    environment = {
      "SDL_AUDIODRIVER" => "dummy",
      "RAGE_PORT_DISABLE_HOST_INPUT" => "1",
      "RAGE_PORT_SYNC_RANDOM" => "1",
      "RAGE_PORT_SMOKE_FRAMES" => "2200",
      "RAGE_PORT_INPUT_SCRIPT" => "400:START,500:DOWN,520:CROSS",
      sweep => "1",
      "RAGE_PORT_SMOKE_CAPTURE_DIR" => directory,
      "RAGE_PORT_SMOKE_CAPTURE_SCENE" => "8",
      "RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN" => "200",
      "RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX" => "1400",
      "RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE" => "25",
    }
    _, trace, status = Open3.capture3(environment, executable.to_s, chdir: ROOT.to_s)
    warn "rage_trace_compare.rb: #{sweep} exited #{status.exitstatus}" unless status.success?
    frames = Pathname.new(directory).children.select(&:file?).sort_by { |f| f.basename.to_s }
    frames.map { |frame| "#{frame.basename} #{Digest::MD5.file(frame)}\n" } +
      trace.lines.grep(/\Asmoke menu sweep /)
  end
end

def observables(executable, options)
  result = {}
  RACES.each { |race| result["race #{race[:name]}"] = race_trace(executable, race) } if options[:races]
  MENUS.each { |name, sweep| result["menu #{name}"] = menu_trace(executable, sweep) } if options[:menus]
  result
end

# A difference is only worth reporting once the observable has shown it can
# tell two builds of the same source apart from nothing at all.
def report(name, control_a, control_b, base, head)
  if control_a != control_b
    differing = control_a.zip(control_b).count { |a, b| a != b }
    puts format("  %-22s UNSTABLE: two builds of the same source differ in %d lines",
                name, differing)
    return :unstable
  end
  if base == head
    puts format("  %-22s same (%d lines)", name, head.length)
    :same
  else
    differing = base.zip(head).count { |a, b| a != b }
    puts format("  %-22s MOVED: %d of %d lines differ", name, differing, head.length)
    :moved
  end
end

work = options[:work]
FileUtils.mkdir_p(work)

puts "building #{options[:base]}, #{options[:head]}, and #{options[:head]} again as the control"
base_executable = build(options[:base], "base", work, options[:jobs])
head_executable = build(options[:head], "head", work, options[:jobs])
control_executable = build(options[:head], "control", work, options[:jobs])

puts "running"
base = observables(base_executable, options)
head = observables(head_executable, options)
control = observables(control_executable, options)

puts
verdicts = head.keys.map do |name|
  report(name, head[name], control[name], base[name], head[name])
end

if options[:clean]
  %w[base head control].each do |tag|
    run!(["git", "worktree", "remove", "--force", (work / "tree-#{tag}").to_s])
  end
  FileUtils.rm_rf(work)
end

puts
if verdicts.include?(:unstable)
  puts "at least one observable cannot tell the two revisions apart from the compiler; " \
       "the rest of this comparison is not evidence"
  exit 2
elsif verdicts.include?(:moved)
  puts "#{options[:head]} moves the game against #{options[:base]}"
  exit 1
else
  puts "#{options[:head]} leaves the game where #{options[:base]} had it"
  exit 0
end
