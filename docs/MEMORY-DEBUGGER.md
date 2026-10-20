# Process memory debugger

DevBridge payload **0.2.3 / release v1.0.2.3** includes a resident memory worker.
Commands use the existing LAN endpoint on port **1337**, operation 21. The worker
listens only on localhost:1345 and starts on the first memory request. Neither a
new ELF compilation nor another ELF upload is needed for individual commands.

This is an offline memory-analysis tool. The first hardware integration test was
performed on firmware 5.10 against the included `helpers/memory/lab.cpp`, with
known health, coins, floating-point energy and a two-level pointer chain. Actual
games have not been validated. It does not include online-game protection bypasses.

## Find and narrow a value

Start the updated payload, then start the app/game you want to inspect. Run from
the project folder:

```console
python host/memory.py processes
python host/memory.py select 123
python host/memory.py maps
python host/memory.py scan --type u32 --value 100 --max-mib 32
python host/memory.py results
```

Change the value in the game, then narrow the prior scan:

```console
python host/memory.py filter decreased
python host/memory.py filter equal --value 75
python host/memory.py results
```

`changed`, `unchanged`, `increased`, `decreased`, `equal` and `not-equal` are
supported. Comparisons use the previous successful scan/filter. Float equality
is exact binary equality. Types: `u8`, `i8`, `u16`, `i16`, `u32`, `i32`, `u64`,
`i64`, `f32` and `f64`, all little-endian.

Unknown-value searches require an explicit range from `maps`:

```console
python host/memory.py scan --type f32 --start 0x200000000 --end 0x200010000
python host/memory.py filter changed
```

Scans use writable, non-executable mappings, with a 32 MiB default and 256 MiB
maximum budget. Coverage and unreadable-byte counts are reported: a bounded scan
is **not** a claim that the entire process was searched. Unknown snapshots are
stored locally; snapshots are not a coherent stop-the-world process image. More
than 100,000 matches requires a smaller range or a more specific value. Existing
sessions are retained when a filter exceeds that limit.

## Read, watch, edit and freeze

```console
python host/memory.py read 0x200000100 --type u32
python host/memory.py watch 0x200000100 --type u32 --seconds 10
python host/memory.py write 0x200000100 999 --type u32 --expect 75
python host/memory.py freeze 0x200000100 999 --type u32 --seconds 10
```

Every write includes expected old bytes and requires matching readback. This is
a compare-before-write operation, **not an atomic compare-and-swap** against a
running game. The target may update a value concurrently. Writes are limited to
16 bytes and must fit a readable/writable, non-executable mapping. No memory
protection changes or executable patches are performed. Writes are logged in
`/data/ps5-devbridge/memory.log`.

Freeze runs on the host, for at most one hour and with a minimum 100 ms interval.
It stops if the host stops. On normal exit or Ctrl+C it restores the starting
value only if the process identity still matches and the last value remains its
own. An abruptly terminated host cannot perform restoration.

## Pointer chains and profiles

```console
python host/memory.py pointers 0x200000100 --depth 2 --max-offset 0x1000 --start 0x200000000 --end 0x200010000
python host/memory.py resolve 0x200000200 0x10 0x20
python host/memory.py profile-save cheats/lab.json --game "Memory Lab" --version "1.0" --address 0x200000100 --type u32 --value 999
python host/memory.py profile-apply cheats/lab.json --version "1.0"
```

Pointer search supports up to three levels, aligned 64-bit pointers, nonnegative
offsets and at most 2,000 reported chains. Results use **absolute roots**. They
must be rediscovered/validated after an app restart; automatic ASLR/module-relative
root discovery is not included yet. Profiles record a user-supplied game/version
label and are deliberately bound to the selected PID/start-time identity. The
version label is not automatically detected from the game. Profiles from a
different process session or version label are refused.

## Session and target handling

`--session PATH` before the command selects an independent local scan session.
The default is `memory-sessions/default.json`; snapshots are kept below that
folder. These local artifacts are excluded from Git. Never reuse an old address
blindly after restarting a game. Re-selecting a process resets its scan state.

Only `eboot.bin` app processes and the included `db-memory-lab` test process are
eligible. `processes --all` shows excluded processes for diagnosis. Shell/system
processes, the memory worker, kernel addresses, unmapped/device ranges and
executable write ranges are refused. A PID plus process-start-time cookie is
verified on every command. These bounds do not identify the purpose of any
particular game value: a matching number can be a display copy rather than the
authoritative gameplay value.

The Python `Client` and `Session` classes expose the same operations to automated
homebrew development and testing. Read the selected target and coverage before
using any write operation.
