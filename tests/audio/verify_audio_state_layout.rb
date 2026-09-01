# frozen_string_literal: true

sources = ARGV.map { |path| [path, File.read(path)] }.to_h

# Retail state is split across one file per owning subsystem, and the objects
# checked below are spread over several of them, so they are folded back into
# the single segment they were before the split.
host_state = sources.select { |path, _| File.basename(path).start_with?("host_state") }
sources = sources.reject { |path, _| host_state.key?(path) }
sources = { "src/port/host_state.c" => host_state.values.join("\n") }.merge(sources)

sources.each do |path, source|
  if path.end_with?("host_state.c")
    abort "#{path}: CdlGetlocP response must remain one eight-byte backing object" unless
      source.include?("unsigned char g_CdLocResult[8]")
    %w[g_CdLocMinute g_CdLocSecond].each do |name|
      abort "#{path}: #{name} incorrectly detaches a byte from CdlGetlocP response" if
        source.match?(/^unsigned char #{name}\b/)
    end
    abort "#{path}: best-sector backing object is smaller than its game declaration" unless
      source.include?("unsigned char g_BestSectorTimes[96]")
    { "g_RankedCars" => 32, "g_SectorEndDistance" => 12,
      "g_CarSpecBars" => 16, "g_TeamLogoClut" => 32,
      "g_PadShiftMasks" => 32, "g_GrandPrixCars" => 104,
      "g_ExtraGrandPrixCars" => 104, "g_TimeAttackCars" => 104 }.each do |name, bytes|
      abort "#{path}: #{name} backing object is truncated" unless
        source.include?("unsigned char #{name}[#{bytes}]")
    end
    { "g_CamPathOffsetDelta" => 12, "g_CamPathOffsetStart" => 12,
      "g_CamPathOffset" => 12, "g_CamPathAngleDelta" => 16,
      "g_CamPathAngleStart" => 16, "g_CamPathAngle" => 16 }.each do |name, bytes|
      abort "#{path}: #{name} backing object is truncated" unless
        source.include?("unsigned char #{name}[#{bytes}]")
    end
    next
  end

  if path.end_with?("cd_audio.c")
    abort "#{path}: pause request does not pass the complete GetlocP response" unless
      source.include?("CdControl(0x11, 0, g_CdLocResult)")
    abort "#{path}: pause snapshot does not use GetlocP relative MSF bytes" unless
      source.include?("g_CdLocResult[2]") && source.include?("g_CdLocResult[3]")
    next
  end

  if path.end_with?("track.h")
    abort "#{path}: chase yaw is detached from its retail camera-path alias" unless
      source.include?("#define g_ChaseYawPrev g_CamPathAngleDelta[CAMPATH_YAW]")
    next
  end

  if path.end_with?("records.c")
    abort "#{path}: default sector records are not initialized" unless
      source.include?("g_BestSectorTimes[series][course][slot]") &&
      source.include?("defaultLapTimes[series * 4 + course]")
    next
  end

  if path.end_with?("update_car_drivetrain.c")
    abort "#{path}: torque lookup still depends on adjacent linker symbols" if
      source.include?("(&g_TorqueBandStart) + bandIndex") ||
      source.include?("(&g_TorqueLossBandStart) + bandIndex")
    explicit_predecessors =
      source.include?("g_TorqueBandEnd[bandIndex - 1]") &&
      source.include?("g_TorqueLossBandEnd[bandIndex - 1]")
    shared_predecessor_helper =
      source.include?("bandEnds[bandIndex - 1]") &&
      source.include?("BandStartIndex(g_TorqueBandEnd, bandIndex)") &&
      source.include?("BandStartIndex(g_TorqueLossBandEnd, bandIndex)")
    abort "#{path}: missing explicit torque-band predecessor lookup" unless
      explicit_predecessors || shared_predecessor_helper
    next
  end

  abort "#{path}: audio reset still derives channel fields from another global" if
    source.include?("ptr[0x78 / 4]")

  writes = source.scan(/g_MusicChannels\[i\]\.volRight\.value\s*=\s*0/).length
  abort "#{path}: missing direct right-volume reset" unless writes == 1
end
