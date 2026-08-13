# frozen_string_literal: true

source = File.read(ARGV.fetch(0))
abort "raster compare does not replay the same log through PsyZ" unless
  source.include?("rage-gp0-replay") && source.include?("native.ppm")
abort "raster compare does not replay through the Ruby PS1 GPU" unless
  source.include?('require "psx"') && source.include?("words.each { |word| gpu.gp0(word) }")
abort "raster compare lacks packet isolation" unless
  source.include?("--last-packet") && source.include?("--only-packet")
abort "raster compare ignores RGB555 precision" unless
  source.include?("significant_pixels") && source.include?("rmse_rgb5") &&
  source.include?(">> 3")
abort "raster compare does not rank connected visual defects" unless
  source.include?("diff.ppm") && source.include?("components.sort_by!") &&
  source.include?("bbox:") && source.include?("sample:")
abort "raster report cannot prove artifact provenance" unless
  source.include?("Digest::SHA256.file") && source.include?("shelljoin") &&
  source.include?("vram_sha256:") && source.include?("log_sha256:")

puts "GP0 raster comparison reuses one VRAM/log oracle across Ruby PS1 and PsyZ"
