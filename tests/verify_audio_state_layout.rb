# frozen_string_literal: true

sources = ARGV.map { |path| [path, File.read(path)] }.to_h

sources.each do |path, source|
  if path.end_with?("update_car_drivetrain.c")
    abort "#{path}: torque lookup still depends on adjacent linker symbols" if
      source.include?("(&g_TorqueBandStart) + bandIndex") ||
      source.include?("(&g_TorqueLossBandStart) + bandIndex")
    abort "#{path}: missing explicit torque-band predecessor lookup" unless
      source.include?("g_TorqueBandEnd[bandIndex - 1]") &&
      source.include?("g_TorqueLossBandEnd[bandIndex - 1]")
    next
  end

  abort "#{path}: audio reset still derives channel fields from another global" if
    source.include?("ptr[0x78 / 4]")

  writes = source.scan(/g_MusicChannels\[i\]\.volRight\.value\s*=\s*0/).length
  abort "#{path}: missing direct right-volume reset" unless writes == 1
end
