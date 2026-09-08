# 0.6.4-alpha GitHub issue audit

Audited all 15 open issues in `khasinski/rage-racer-pc`. An implemented fix is
separate from confirmation on the reporter's hardware. Issues are not closed
merely because a similarly named unit test passes.

| Issue | Candidate handling | Evidence / remaining work |
| --- | --- | --- |
| [#25](https://github.com/khasinski/rage-racer-pc/issues/25) crashes, sky, menu music | Multiple fixes; crash report remains a release gate | Capture/VRAM lifetime, bounded visibility and transfer staging fixes; macOS/Linux three-lap modern Overpass probes and macOS PAL music tempo pass. Windows software-rendered image has correct sky; a separate 426x240 software run completes three laps with clean teardown. |
| [#23](https://github.com/khasinski/rage-racer-pc/issues/23) race crashes, Time Attack HUD, CD audio | HUD and audio paths implemented; crash confirmation pending | `race_hud_layout`, `race_hud_placement`, `bgm_select_control`, `cd_audio_runtime`; Windows software-rendered three-lap route passes; accelerated driver confirmation remains open. |
| [#22](https://github.com/khasinski/rage-racer-pc/issues/22) duplicate final-stretch speech | Fixed during this cleanup | `lap_and_finish` asserts no repeated encouragement and one finish queue; `scenario_after_finish` passes live menu recovery and speech ordering. |
| [#21](https://github.com/khasinski/rage-racer-pc/issues/21) red Windows sky | Original sky replay implemented | Sky identity/background/geometry tests pass; current Windows SwiftShader image inspected alongside macOS/Metal and Linux/Vulkan: no red-sky defect in the controlled scene. Hardware drivers remain untested. |
| [#20](https://github.com/khasinski/rage-racer-pc/issues/20) missing native assets at startup | Automatic C importer implemented | macOS CUE startup and BIN three-lap run pass; Linux CUE three-lap run and clean BIN startup pass, all using automatic import. File-manager/picker acceptance remains separate. |
| [#18](https://github.com/khasinski/rage-racer-pc/issues/18) asset browser feedback | Informational, not a defect | Standalone game no longer requires the old manual extraction workflow. |
| [#17](https://github.com/khasinski/rage-racer-pc/issues/17) Pegase cabin texture | Texture/material fixes implemented | Live macOS `pegase_cabin` image regression passes. |
| [#16](https://github.com/khasinski/rage-racer-pc/issues/16) zero Time Attack records | Default records restored and validated | `record_defaults` covers regional/course/direction defaults; existing user saves are distinct from newly initialized records. |
| [#14](https://github.com/khasinski/rage-racer-pc/issues/14) multiple controllers | Activity-based device selection implemented | `input_device_select` covers activity, ties, disconnect and no devices. Physical docked Steam Deck + wireless combination not available here. |
| [#12](https://github.com/khasinski/rage-racer-pc/issues/12) automatic for manual-only cars | Open enhancement/mod request | Retail restrictions remain; affected cars need validated automatic shift thresholds. Not claimed fixed. |
| [#11](https://github.com/khasinski/rage-racer-pc/issues/11) texture-filter documentation | Documented | README and shipped INI list `nearest` and `linear`, with descriptions. |
| [#10](https://github.com/khasinski/rage-racer-pc/issues/10) sample logo palette | Sample palette composition implemented | `team_logo` checks both palette halves, canvas and invalid selections; `team_logo_clut` checks upload source. |
| [#8](https://github.com/khasinski/rage-racer-pc/issues/8) CHD | libchdr integration implemented | `chd_track_layout` passes arithmetic/bounds checks; requires a real CHD startup test for full release acceptance. |
| [#6](https://github.com/khasinski/rage-racer-pc/issues/6) wheels | Raw SDL wheel support implemented | Configurable steering/pedals/buttons use NeGcon path. No physical wheel is attached; force feedback is not promised. |
| [#4](https://github.com/khasinski/rage-racer-pc/issues/4) adjustable camera | Configurable turn lookahead implemented | `[camera] chase_turn_lookahead=0..1`; camera tests pass. Default zero preserves retail behavior. |

The issue list contains enhancement requests and an informational report as well
as defects. Outstanding hardware checks and #12 must stay explicit; this audit
is not a claim that every GitHub issue is fixed.
