#!/usr/bin/env ruby
# frozen_string_literal: true

frontend = File.read(ARGV.fetch(0))
mirror = File.read(ARGV.fetch(1))
host_state = File.read(ARGV.fetch(2))
tachometer = File.read(ARGV.fetch(3))

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

%w[g_TachoNeedlePrim0PageA g_TachoNeedlePrim0
   g_TachoNeedlePrim1PageA g_TachoNeedlePrim1PageB
   g_TachoNeedlePrim1 g_RaceHudSprite11U0].each do |name|
  abort "tachometer still uses detached frame-context alias #{name}" if tachometer.include?(name)
  abort "host still allocates detached frame-context alias #{name}" if host_state.include?(name)
end

abort "tachometer packets do not use the typed frame-context owner" unless
  tachometer.include?("&g_FrameContexts[0].layout.raceHud") &&
  tachometer.include?("&g_FrameContexts[1].layout.raceHud") &&
  tachometer.include?("tachometerDrawModes[0]") &&
  tachometer.include?("tachometerDrawModes[1]") &&
  tachometer.include?("tachometerFace")

puts "frame environment and HUD packets have one typed owner on the host"
