#!/usr/bin/env ruby
# frozen_string_literal: true

# Which of our locks are asleep?
#
# A test that passes tells you it ran. It does not tell you it would have
# noticed anything. This injects small, plausible mistakes into a source file
# one at a time, runs the tests that are supposed to cover it, and reports the
# ones nothing noticed.
#
# A surviving mutation is not automatically a hole: some are equivalent to the
# original, and some sit in code the game cannot reach. But every survivor is a
# claim to check rather than a number to quote, and a file whose survivors
# outnumber its catches is being watched by a test that only looks busy.

require "fileutils"
require "open3"
require "optparse"
require "pathname"

ROOT = Pathname.new(__dir__).parent.expand_path

options = { count: 12, build: "build", jobs: nil, tests: nil, target: nil,
            first_line: nil, last_line: nil }
OptionParser.new do |parser|
  parser.banner = "usage: rage_mutation_probe.rb --source FILE --tests REGEX [options]"
  parser.on("--source FILE", "Source file to mutate") { |v| options[:source] = v }
  parser.on("--tests REGEX", "ctest -R expression for the tests that cover it") { |v| options[:tests] = v }
  parser.on("--build DIR", "Build directory (default: build)") { |v| options[:build] = v }
  parser.on("--target NAME", "Build only this target instead of everything") { |v| options[:target] = v }
  parser.on("--count N", Integer, "How many mutations to try (default: 12)") { |v| options[:count] = v }
  parser.on("--jobs N", Integer, "Parallel jobs for build and ctest") { |v| options[:jobs] = v }
  parser.on("--lines A,B", "Only mutate lines in this range") do |v|
    a, b = v.split(",", 2).map { |part| Integer(part) }
    options[:first_line] = a
    options[:last_line] = b
  end
end.parse!

abort "rage_mutation_probe.rb: --source and --tests are required" unless
  options[:source] && options[:tests]

source = Pathname.new(options[:source])
abort "rage_mutation_probe.rb: no such file: #{source}" unless source.file?
build = Pathname.new(options[:build])
abort "rage_mutation_probe.rb: no such build directory: #{build}" unless build.directory?

# Only lines that are plain code: a mutation inside a comment or a string
# changes nothing and would be reported as a survivor, which is a lie.
def code_line?(line)
  stripped = line.strip
  return false if stripped.empty?
  return false if stripped.start_with?("/*", "*", "//", "#")
  return false if line.include?('"')
  true
end

# Small mistakes of the kind a person actually makes: a constant off by one, a
# boundary the wrong side of itself, a condition inverted.
def mutants_for(line)
  found = []
  line.scan(/(?<![\w.])(0x[0-9A-Fa-f]+|\d+)(?![\w.])/) do
    literal = Regexp.last_match(1)
    value = literal.start_with?("0x") ? Integer(literal, 16) : Integer(literal)
    next if value > 0x7FFFFFF
    replacement = literal.start_with?("0x") ? format("0x%X", value + 1) : (value + 1).to_s
    found << [literal, replacement]
  end
  found << ["<=", "<"] if line.include?("<=")
  found << [">=", ">"] if line.include?(">=")
  found << ["!=", "=="] if line.include?("!=") && !line.include?("!==")
  found
end

original = source.read
lines = original.split("\n", -1)
first = (options[:first_line] || 1) - 1
last = (options[:last_line] || lines.length) - 1

sites = []
(first..[last, lines.length - 1].min).each do |index|
  next unless code_line?(lines[index])
  mutants_for(lines[index]).each { |from, to| sites << [index, from, to] }
end

if sites.empty?
  abort "rage_mutation_probe.rb: nothing to mutate in #{source}"
end

# Spread the sample over the file rather than taking the first N, so a probe
# of a long function does not only ever test its opening.
step = [sites.length / options[:count], 1].max
chosen = sites.each_slice(step).map(&:first).first(options[:count])

def build_and_run(build, target, tests, jobs)
  make = ["cmake", "--build", build.to_s]
  make += ["--target", target] if target
  make += ["--parallel"]
  make << jobs.to_s if jobs
  _, status = Open3.capture2e(*make)
  return :build_failed unless status.success?

  run = ["ctest", "--test-dir", build.to_s, "-R", tests]
  run += ["-j", jobs.to_s] if jobs
  output, status = Open3.capture2e(*run)
  return :passed if status.success?

  failed = output.scan(/^\s*\d+ - (\S+) \(Failed\)/).flatten
  failed.empty? ? :failed : failed
end

puts "#{source}: #{sites.length} possible mutations, trying #{chosen.length}"
puts "tests: #{options[:tests]}"
puts

baseline = build_and_run(build, options[:target], options[:tests], options[:jobs])
if baseline != :passed
  abort "rage_mutation_probe.rb: the tests do not pass before mutating: #{baseline.inspect}"
end

survivors = []
catchers = Hash.new(0)
begin
  chosen.each_with_index do |(index, from, to), number|
    mutated = lines.dup
    mutated[index] = mutated[index].sub(from, to)
    if mutated[index] == lines[index]
      puts format("%2d/%d  line %-5d %-14s did not apply", number + 1, chosen.length,
                  index + 1, "#{from} -> #{to}")
      next
    end
    source.write(mutated.join("\n"))
    result = build_and_run(build, options[:target], options[:tests], options[:jobs])
    label = format("line %-5d %s", index + 1, "#{from} -> #{to}")
    case result
    when :build_failed
      puts format("%2d/%d  %-34s does not compile, skipped", number + 1, chosen.length, label)
    when :passed
      puts format("%2d/%d  %-34s SURVIVED", number + 1, chosen.length, label)
      survivors << [index + 1, from, to, lines[index].strip]
    else
      names = result == :failed ? ["(unnamed)"] : result
      names.each { |name| catchers[name] += 1 }
      puts format("%2d/%d  %-34s caught by %s", number + 1, chosen.length, label,
                  names.join(", "))
    end
  end
ensure
  source.write(original)
  build_and_run(build, options[:target], options[:tests], options[:jobs])
end

puts
tried = chosen.length
puts "#{survivors.length} of #{tried} survived"
unless catchers.empty?
  puts "caught by:"
  catchers.sort_by { |_, n| -n }.each { |name, n| puts format("  %-28s %d", name, n) }
end
unless survivors.empty?
  puts "survivors:"
  survivors.each do |line, from, to, text|
    puts format("  line %-5d %-14s %s", line, "#{from} -> #{to}", text[0, 70])
  end
end
