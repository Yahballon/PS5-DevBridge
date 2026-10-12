# Connect a local coding agent to PS5 DevBridge (AI-assisted)

Use a coding-agent environment with local file, shell and image-viewing tools, such as the local session used to develop this project. It must be able to reach your PS5 on your LAN. A prompt alone does not provide network access or run programs. For an API integration, your application must execute local shell tool calls and return results: [official OpenAI shell documentation](https://developers.openai.com/api/docs/guides/tools-shell).

## Setup

1. Load the DevBridge ELF after jailbreak, or press Square in the optional app while your localhost loader is running on port 9021.
2. Generate pairing locally with `python host/configure.py --host YOUR_PS5_IP --peer YOUR_PC_IP`, then rebuild the payload/app. Keep the pairing files private.
3. Check `python host/bridge.py status` from your PC.
4. Keep `python host/session.py` running in a separate terminal if you want the TV to show an active host session. Activity expires after 15 seconds.

## Copy this prompt into your local agent session

```text
(AI-assisted) Help me build and test my own PS5 homebrew using the PS5-DevBridge project in this workspace.

Read README.md and the host scripts before connecting. Use the local pairing configuration without printing the key. Start by checking host/bridge.py status. Work only on my explicitly named homebrew project.

Use templates/simple-calc as a working native UI starter based on blackbearreloaded/ps5-homebrew-ui. Give the app its own valid title/content ID, retain upstream licenses, and use the SDK build scripts. Do not copy generated dependencies, pairing keys or private console data into release source archives.

Build the app, install its folder through host/apps.py, then launch it. Close only that registered app before an update. Use host/controller.py for bounded inputs, host/screenshot.py for fresh screenshots and host/bridge.py logs for output. Inspect actual screenshots, fix layout problems and rebuild until the requested UI works. A process ID or successful upload is not visual proof.

For automated checks, add a host/ui_test.py manifest with explicit input steps and screenshot baselines. Preserve results in devbridge-results. If the app crashes, retrieve its opt-in crash report with host/crashes.py and keep the unstripped ELF for symbolization.

Do not modify unrelated apps, system software or kernel behavior. Do not claim an untested feature works on hardware. Explain material limitations briefly, and continue the authorized development work.
```
