# PS5 DevBridge v0.2.0 (AI-assisted)

A background PS5 homebrew development payload on TCP **1337**, plus an optional live-log TV console. The payload stays available when the app is closed.

## New development features

- Welcome notification: version, firmware, IP and port.
- Authenticated PC activity tracking; the TV app shows AI / HOST connected or idle.
- Automated controller/screenshot tests, with optional region-based screenshot baselines.
- Own-app crash reporter: fault address, instruction/stack registers and best-effort address stack traces.
- Managed native homebrew: register, list, state, start, close, staged folder install/update and remove.
- Simple Calc native project template using ps5-homebrew-ui.
- English setup instructions and a reusable [AI homebrew prompt](docs/AI-HOMEBREW-PROMPT.md).
- Square in the TV app starts the embedded payload through an existing localhost port-9021 loader. A sandboxed native app cannot replace the required privileged loader by itself.

## Start and pairing

Load `dist/devbridge-loader.elf` with your payload manager once after each reboot/jailbreak. The daemon uses port 1337; port 9021 is only used for initial loading and the optional app bootstrap.

```text
python host/configure.py --host YOUR_PS5_IP --peer YOUR_PC_IP
python host/bridge.py status
python host/session.py
```

`local-config.json`, `loader/bridge_config.h` and `app/src/local_config.hpp` contain private pairing data. Do not publish them or your paired binary as a generic release. Generate your own pairing and rebuild. Connections are limited to the configured PC and localhost. Activity tracking confirms authenticated host traffic, not whether the operator is ChatGPT or a human.

## Build

Set `PS5_PAYLOAD_SDK` to your SDK directory, then run `bash build.sh` for the payload/helpers or `bash build.sh --with-app` for the optional native app as well. The native app also needs the pinned SDK/OpenGL dependencies used by its included build scripts. `templates/simple-calc/` is independently buildable with `make USE_CCACHE=0 BUILD_JOBS=4 app`.

## Create your own project

```text
python host/new_project.py MyHomebrew --title PPSA99342 --name "My Homebrew"
```

This copies the working calculator/UI starter into a new directory with fresh title metadata. Replace its calculator logic with your own tool or app while keeping the renderer and controller scaffolding.

## Daily commands

```text
python host/bridge.py logs
python host/bridge.py deploy your-test.elf
python host/bridge.py open-browser https://example.com/
python host/controller.py circle --ms 100
python host/screenshot.py
python host/ui_test.py tests/ui-example.json
python host/files.py put example.pkg /data/ps5-devbridge/example.pkg
python host/files.py get /data/ps5-devbridge/events.log events.log
python host/packages.py install /data/ps5-devbridge/example.pkg
python host/packages.py status CONTENT_ID
python host/packages.py uninstall PPSA12345
python host/apps.py register PPSA99339 /mnt/usb0/homebrew/PPSA99339
python host/apps.py list
python host/apps.py start PPSA99339
python host/apps.py close PPSA99339
python host/apps.py install-folder PPSA99339 templates/simple-calc/dist/PPSA99339
python host/apps.py remove PPSA99339
python host/crashes.py /data/ps5-devbridge/crashes/PPSA99339.txt calc-crash.txt
```

Folder installation uses the ShadowMountPlus API on port 10101 to register/mount. The parent homebrew directory must already exist. Update refuses a running app, uploads into a sibling staging folder, verifies every file and promotes the folder while keeping the original backup. Managed removal unregisters the title and retains its folder as a timestamped trash directory. It does not silently recursively delete saves or other app data.

Controller commands use a resident authenticated localhost worker on port 1343. The host sends only small command frames. Virtual devices are scoped to each bounded sequence and physical-controller ownership is restored afterward. Commands allow up to 32 steps, 3 seconds per hold/pause and 15 seconds total. This controller implementation is firmware-5.10-specific. PS-button injection is not implemented. `bridge.py stop` requests input cancellation.

Screenshots use held Create, then wait for and retrieve a new PNG below `/user/av_contents/photo/`. An older screenshot is never substituted. Capture restrictions can prevent screenshots. UI tests without a baseline save evidence for review; they do not invent a visual pass.

## Crash reporting

Own apps can include `sdk/crash_report.hpp` and call `devbridge_crash::install(report_path)` at startup. The parent report directory must exist. Reporting is opt-in, not a system-wide debugger. Signal-based unwinding after memory corruption is best effort and can fail; fault/register lines are written before unwinding. Preserve unstripped symbols to translate stack addresses. Reports may contain app addresses; inspect them before publishing.

## Transfers and package management

File uploads use 1 MiB chunks, staging, SHA-256 read-back verification and explicit commit. Existing destinations require `--overwrite`. Remote parents must exist. Paths are limited to `/data/` and `/mnt/usb0..7/`, with symlink/traversal checks. Disconnects leave staging, not partially committed destinations.

PKG installation uses the native system installer. An accepted request is not completion; query the returned content ID. System-title uninstall is rejected. Package handler compatibility still needs live testing for each firmware. ABI reference: [etaHEN package installation writeup](https://github.com/etaHEN/etaHEN/blob/main/PS5%20technical%20writeups/pkg-writeup.md).

## Protocol

80-byte little-endian header: magic `PS5DEV1` with terminating NUL, 64-byte pairing key, operation u32, body size u32. Operations: 1 status; 2 ELF; 3 logs; 4/9 browser; 5 legacy browser queue; 7 append log; 8 cancel input; 10 controller; 11 file read; 12 staged file write; 13 commit; 14 PKG install; 15 uninstall; 16 photo inventory; 17 PNG read; 19 package status; 20 managed apps.

Pairing uses private-LAN TCP, not TLS. This is a development bridge, not a remote command shell. Uploaded ELFs are size/layout checked. Helper outputs must be checked independently of spawn results.

## Verified in v0.2.0

Payload and native app build successfully. On the 5.10 console: managed register/list/state, close/update/start, live-log UI screenshot, file transfer, crash fault/register/stack report and a real header-baseline UI check passed. The calculator starter builds successfully. Managed removal and the app button's offline bootstrap still need a dedicated live check.

## Credits

Payload loader: ps5-payload-dev/elfldr (GPL; see UPSTREAM.md). Native UI/runtime: [BlackBearReloaded / ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui). Original notices and licenses are retained.
