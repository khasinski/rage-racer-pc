# frozen_string_literal: true

sources = ARGV.map { |path| [path, File.read(path)] }.to_h

sources.each do |path, source|
  abort "#{path}: audio reset still derives channel fields from another global" if
    source.include?("ptr[0x78 / 4]")

  writes = source.scan(/g_MusicChannels\[i\]\.volRight\.value\s*=\s*0/).length
  abort "#{path}: missing direct right-volume reset" unless writes == 1
end
