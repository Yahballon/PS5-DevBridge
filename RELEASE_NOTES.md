# PS5 DevBridge v1.0.2.3

- Adds a resident process-memory debugger through the existing port 1337.
- Supports 8/16/32/64-bit signed/unsigned integers and float32/float64 scans.
- Adds unknown-value snapshots and changed/unchanged/increased/decreased/exact filters.
- Adds live read/watch, expected-value checked edits, readback verification and bounded freeze.
- Adds basic pointer-chain search/resolution and session/version-labelled local profiles.
- Rejects stale process identities, kernel addresses, device/unmapped ranges and executable writes.
- Adds host/protocol regressions and a dedicated Memory Lab hardware test.
- Retains key-free LAN operation and the matching embedded payload.

Validated on firmware 5.10 against the included Memory Lab, not an actual game.
Pointer roots and profiles are absolute and session-bound; automatic module-relative/ASLR resolution is not included.
