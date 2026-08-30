#!/usr/bin/env ruby
# frozen_string_literal: true

source = File.read(ARGV.fetch(0))
function = source[/int WriteCapturedFrame\(const char \*path\) \{.*?^\}/m]
abort "WriteCapturedFrame is missing" unless function

sync = function.index("DrawSync(0);")
draw_page = function.index("Psyz_VideoAllocCapturedDrawPage")
frame = function.index("Psyz_VideoAllocCapturedFrame")
vram = function.index("Psyz_VideoAllocCapturedVram")
abort "capture does not synchronize the submitted ordering table" unless sync
abort "draw-page download can precede capture synchronization" unless draw_page && sync < draw_page
abort "front-buffer download can precede capture synchronization" unless frame && sync < frame
abort "VRAM download can precede capture synchronization" unless vram && sync < vram

puts "native smoke capture synchronizes GP0 execution before every readback path"
