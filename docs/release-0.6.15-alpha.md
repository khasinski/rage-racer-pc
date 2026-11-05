# Rage Racer PC 0.6.15-alpha

This alpha fixes Windows startup problems reported for 0.6.14-alpha. Gameplay
and content are unchanged.

## Changes

- **Microsoft Defender false positive.** On VirusTotal, Microsoft flagged the
  0.6.14 `Rage-Racer.exe` as `Trojan:Win32/Wacatac.C!ml`. That detection comes
  from a machine-learning heuristic and can block or quarantine the game on
  some PCs. On Windows, the in-game HD pack downloader now uses the WinHTTP
  networking built into Windows instead of a bundled curl/TLS client. Its
  security rules are unchanged: HTTPS with certificate validation, no
  redirects, and SHA-256 verification. The executable also carries full
  Windows version information.
- **Missing Vulkan driver.** If a PC had no Vulkan-capable graphics driver
  (for example in a Remote Desktop session, with the basic display driver, or
  in some virtual machines), the game used to keep running in the background
  without a window. It now shows a startup error that explains the
  requirement. This check runs before the disc image picker.
- Every Windows build is now checked automatically before release. The checks
  launch the packaged game from folders with spaces and non-ASCII characters
  and test disc error handling and the HD download path. They also start the
  modern renderer on a software Vulkan driver.

## Install and play

Extract the archive for your platform and open the game. On first launch, choose
only your legally obtained Rage Racer CUE or Track 01 BIN; native renderer assets
are generated automatically. The optional HD pack is downloaded from the in-game
Mods page. No separate asset-conversion command or image-codec installation is
required. The editor is a separate optional download.

On Windows the game needs a graphics driver with Vulkan support. Install the
latest driver from NVIDIA, AMD or Intel if the startup error asks for it.

## Known limits

The Windows executable is not code-signed yet, so SmartScreen may still show
an "unknown publisher" prompt on first launch. If your antivirus still flags
the game, please report it in the issue tracker with the product name and
detection name.
