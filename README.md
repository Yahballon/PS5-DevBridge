# PS5 DevBridge v0.2.0 (AI-assisted)

A background PS5 homebrew development payload on TCP **1337**, plus an optional live-log TV console. The payload stays available when the app is closed.

## What the payload can do

- Welcome notification: version, firmware, IP and port.
- Authenticated PC activity tracking; the TV app shows AI / HOST connected or idle.
- Automated controller/screenshot tests, with optional region-based screenshot baselines.
- Own-app crash reporter: fault address, instruction/stack registers and best-effort address stack traces.
- Managed native homebrew: register, list, state, start, close, staged folder install/update and remove.
- Simple Calc native project template using ps5-homebrew-ui.
- English setup instructions and a reusable [AI homebrew prompt](docs/AI-HOMEBREW-PROMPT.md).
- Square in the TV app starts the embedded payload through an existing localhost port-9021 loader. A sandboxed native app cannot replace the required privileged loader by itself.

## Complete capability overview

| Capability | What is included |
| --- | --- |
| Background service | Dedicated TCP 1337 service that stays running without the TV app |
| Startup information | Welcome notification, payload version, firmware, local IP and port |
| Pairing | Authentication key and configured-PC/localhost access checks |
| Connection status | Recent authenticated host activity; optional heartbeat and TV indicator |
| ELF execution | Upload SDK payload ELFs and receive their stdout/result output |
| Logs | Read daemon events and append test output; live polling on the TV console |
| Controller commands | D-pad, face buttons, Create, Options, shoulder buttons, stick clicks, touchpad, analog sticks and triggers |
| Input sequences | Up to 32 timed steps; cancellation, neutral release and controller restoration |
| Browser | Open an HTTP(S) page directly through the PS5 browser helper |
| Screenshots | Capture using Create, find the new PNG recursively and transfer it to the PC |
| Automated UI tests | Run input sequences, wait for the UI, save screenshots and compare an optional baseline/region |
| File upload/download | Chunked transfers under data/USB directories, staged uploads, SHA-256 read-back checks and explicit commit |
| PKG installation | Start installation of a local PKG, then query progress, state and errors by content ID |
| PKG uninstall | Uninstall an explicitly named non-system title through the native service |
| Homebrew registry | Register and list your own native homebrew folders and inspect running state |
| App lifecycle | Start or close an explicitly registered homebrew title |
| Native folder install/update | Verify staged files, replace the folder, retain a backup, register/mount through ShadowMountPlus |
| Managed removal | Unregister a homebrew title and retain its files in a timestamped trash folder |
| Own-app crash reports | Opt-in signal, fault address, instruction/stack registers and best-effort stack return addresses |
| Project workflow | Build, deploy, run inputs, capture screenshots and collect output through host scripts |
| Project starter | Generate a separate calculator/UI project with its own title metadata |
| Optional TV app | Live logs, endpoint, host indicator, browser button and payload-start button |

Some functions use small helper processes embedded in the payload. Controller commands use a resident worker; the PC does not need to recompile or upload a helper ELF for each input. Crash reporting must be integrated into your own app. Hardware-tested and pending functions are listed below.

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

## Public release app

`dist/PS5-DevBridge-v0.2.0-release-app.zip` is the redistributable native app. It contains neither this developer's pairing key nor a personally paired payload. It reads its key and payload from the local PS5 data folder.

1. Generate your own pairing configuration and build your payload.
2. Load that payload once with your payload manager.
3. Run `python host/prepare_release_app.py` from the paired PC. This transfers the private key and your built payload to `/data/ps5-devbridge/` without printing the key.
4. Install/mount the release app's `PPSA99337` folder. Square starts the local payload through your existing localhost loader on port 9021.

Build the public app with `bash build-release-app.sh`. The private `--with-app` build remains useful for personal development; it embeds pairing data and must not be published as a generic binary.

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

## AI prompt: connect and develop homebrew

Use a coding-agent environment with local file, shell and image-viewing tools, such as the local session used to develop this project. It must be able to reach your PS5 on your LAN. A prompt alone does not provide network access or run programs. For an API integration, your application must execute local shell tool calls and return results: [official OpenAI shell documentation](https://developers.openai.com/api/docs/guides/tools-shell).

### Connection setup

1. Load the DevBridge ELF after jailbreak, or press Square in the optional app while your localhost loader is running on port 9021.
2. Generate pairing locally with `python host/configure.py --host YOUR_PS5_IP --peer YOUR_PC_IP`, then rebuild the payload/app. Keep the pairing files private.
3. Check `python host/bridge.py status` from your PC.
4. Keep `python host/session.py` running in a separate terminal if you want the TV to show an active host session. Activity expires after 15 seconds.

### Copy this prompt into your local agent session

```text
(AI-assisted) Help me build and test my own PS5 homebrew using the PS5-DevBridge project in this workspace.

Read README.md and the host scripts before connecting. Use the local pairing configuration without printing the key. Start by checking host/bridge.py status. Work only on my explicitly named homebrew project.

Use templates/simple-calc as a working native UI starter based on blackbearreloaded/ps5-homebrew-ui. Give the app its own valid title/content ID, retain upstream licenses, and use the SDK build scripts. Do not copy generated dependencies, pairing keys or private console data into release source archives.

Build the app, install its folder through host/apps.py, then launch it. Close only that registered app before an update. Use host/controller.py for bounded inputs, host/screenshot.py for fresh screenshots and host/bridge.py logs for output. Inspect actual screenshots, fix layout problems and rebuild until the requested UI works. A process ID or successful upload is not visual proof.

For automated checks, add a host/ui_test.py manifest with explicit input steps and screenshot baselines. Preserve results in devbridge-results. If the app crashes, retrieve its opt-in crash report with host/crashes.py and keep the unstripped ELF for symbolization.

Do not modify unrelated apps, system software or kernel behavior. Do not claim an untested feature works on hardware. Explain material limitations briefly, and continue the authorized development work.
```

## Protocol

80-byte little-endian header: magic `PS5DEV1` with terminating NUL, 64-byte pairing key, operation u32, body size u32. Operations: 1 status; 2 ELF; 3 logs; 4/9 browser; 5 legacy browser queue; 7 append log; 8 cancel input; 10 controller; 11 file read; 12 staged file write; 13 commit; 14 PKG install; 15 uninstall; 16 photo inventory; 17 PNG read; 19 package status; 20 managed apps.

Pairing uses private-LAN TCP, not TLS. This is a development bridge, not a remote command shell. Uploaded ELFs are size/layout checked. Helper outputs must be checked independently of spawn results.

## Verified in v0.2.0

Payload and native app build successfully. On the 5.10 console: managed register/list/state, close/update/start, live-log UI screenshot, file transfer, crash fault/register/stack report and a real header-baseline UI check passed. The calculator starter builds successfully. Managed removal and the app button's offline bootstrap still need a dedicated live check.

## Credits

Payload loader: ps5-payload-dev/elfldr (GPL; see UPSTREAM.md). Native UI/runtime: [BlackBearReloaded / ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui). Original notices and licenses are retained.
