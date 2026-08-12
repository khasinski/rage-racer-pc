#!/usr/bin/env ruby
# frozen_string_literal: true

abort "usage: #{$PROGRAM_NAME} PSX.LOG NATIVE.LOG TIMER" unless ARGV.length == 3
wanted_timer = Integer(ARGV[2], 0)

read_frame = lambda do |path|
  records = nil
  File.foreach(path) do |line|
    if (match = line.match(/\Avisible-frame .*\btimer=(-?\d+)/))
      records = Integer(match[1], 10) == wanted_timer ? {} : nil
    elsif records && (match = line.match(/\A((?:visible|mirror)-(?:cell|mask)) (\d+)=(.*)/))
      records[[match[1], Integer(match[2], 10)]] = match[3].strip
    end
  end
  abort "timer #{wanted_timer} absent from #{path}" unless records
  records
end

psx = read_frame.call(ARGV[0])
native = read_frame.call(ARGV[1])
keys = (psx.keys | native.keys).sort
differences = keys.map do |key|
  next if psx[key] == native[key]
  [key, psx[key] || "missing", native[key] || "missing"]
end.compact
membership = 0
active_coordinates = 0
masks = 0
differences.each do |(kind, _index), left, right|
  if kind.end_with?("mask")
    masks += 1
  else
    left_active = left.split(",").last != "-1"
    right_active = right.split(",").last != "-1"
    membership += 1 if left_active != right_active
    active_coordinates += 1 if left_active && right_active
  end
end
puts "timer=#{wanted_timer} records=#{keys.length} differences=#{differences.length}"
puts "membership=#{membership} active_coordinates=#{active_coordinates} masks=#{masks}"
differences.first(20).each do |(kind, index), left, right|
  puts "#{kind} #{index} psx=#{left} native=#{right}"
end
exit(differences.empty? ? 0 : 1)
