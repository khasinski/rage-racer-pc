# Save/load menu stall — 2026-09-08

The complete save generated for manual testing exposed a real menu bug. The
game continued rendering, and the payload had already loaded successfully
(`GameMenuLoadPhase = 0x3900`), but action `0x25` remained busy indefinitely.

`CardStatusSettledAfterIo` required four consecutive READY results. The real
asynchronous `PollMemoryCardStatus` resets its result to PENDING when starting
each new card-info request. These intervening PENDING results cleared the
settle counter, so it never reached four. The same helper runs after saving.

The counter now waits through PENDING results and accumulates successful
probes. Definite non-ready results still reset it. The existing test proving
that a removed card resets the count remains in place.

The menu regression now additionally uses the production asynchronous status
machine with immediate hardware-completion events, checking bounded progress
after both save and load. It fails with the old implementation and passes
with the fix. The previous 1,161,216-step synthetic menu sweep also retains its
digest. All eight focused macOS card/save tests pass; the extended menu test
also passes on Linux and ClangCL Windows through darwine.

Evidence: `build/complete-save-play-Kt4xye/` (sample, debugger state, positive
and negative test logs). No release has been published.
