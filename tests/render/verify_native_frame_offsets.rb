#!/usr/bin/env ruby
# frozen_string_literal: true

source_dir = File.expand_path(ARGV.fetch(0))
offenders = []

Dir[File.join(source_dir, "src/main/PAL/main/**/*.c")].sort.each do |path|
  File.readlines(path).each_with_index do |line, index|
    next unless line.match?(/g_DrawBuffer\s*\+\s*0xCC|base\s*\+=\s*0xCC/i)

    offenders << "#{path.delete_prefix(source_dir + '/')}:#{index + 1}: #{line.strip}"
  end
end

abort "PS1 frame-layout offsets used on native pointers:\n#{offenders.join("\n")}" unless
  offenders.empty?

puts "Native drawing code uses typed frame-context ordering tables"
