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
    { "g_SectorEndDistance" => 12,
      "g_CarSpecBars" => 16, "g_TeamLogoClut" => 32 }.each do |name, bytes|
      abort "#{path}: #{name} backing object is truncated" unless
        source.include?("unsigned char #{name}[#{bytes}]")
    end
    %w[g_GrandPrixCars g_ExtraGrandPrixCars g_TimeAttackCars].each do |name|
      abort "#{path}: #{name} must remain one complete typed car table" unless
        source.include?("CarEntry #{name}[GAME_CAR_COUNT]")
    end
    next
  end

  if path.end_with?("cd_pause_request.c")
    abort "#{path}: pause request does not pass the complete GetlocP response" unless
      source.include?("CdControl(CD_DRIVE_GET_LOCATION, 0, g_CdLocResult)")
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

  next if path.end_with?("cd_audio.c") ||
          path.end_with?("sound_runtime.c") ||
          path.end_with?("audio_initialization.c")

  abort "#{path}: audio reset still derives channel fields from another global" if
    source.include?("ptr[0x78 / 4]")

  writes = source.scan(/g_MusicChannels\[i\]\.volRight\s*=\s*0/).length
  abort "#{path}: missing direct right-volume reset" unless writes == 1
end
