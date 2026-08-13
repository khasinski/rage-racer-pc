#!/usr/bin/env ruby
# frozen_string_literal: true

frontend = File.read(ARGV.fetch(0))
mirror = File.read(ARGV.fetch(1))
host_state = File.read(ARGV.fetch(2))

abort "second frame draw environment is still initialized through a detached alias" if
  frontend.match?(/\bg_DrawEnv1\b/) || frontend.match?(/\bg_MirrorDrawEnv1\b/)

%w[g_MirrorDrawEnv0ClipY g_MirrorDrawEnv0ClipH
   g_MirrorDrawEnv1ClipY g_MirrorDrawEnv1ClipH].each do |name|
  abort "mirror clip is still written through detached alias #{name}" if mirror.include?(name)
  abort "host still allocates detached mirror alias #{name}" if host_state.include?(name)
end

abort "mirror clip does not update frame zero directly" unless
  mirror.include?("g_FrameContexts[0].environment.mirrorDraw.clip.y") &&
  mirror.include?("g_FrameContexts[0].environment.mirrorDraw.clip.h")
abort "mirror clip does not update frame one directly" unless
  mirror.include?("g_FrameContexts[1].environment.mirrorDraw.clip.y") &&
  mirror.include?("g_FrameContexts[1].environment.mirrorDraw.clip.h")

puts "frame DrawEnv fields have one typed owner on the host"
