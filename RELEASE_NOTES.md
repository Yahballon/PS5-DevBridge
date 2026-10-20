# PS5 DevBridge v1.0.2.2

- Rebuilt from current key-free LAN sources (payload version 0.2.2).
- Removes the legacy pairing requirement; no prepare_release_app.py setup needed.
- Includes the matching payload inside the native app. Square starts it through localhost port 9021.
- Updates the native UI to OpenGL 3.3 for the source-built SDK.
- Adds automated release builds, graphics version checks and checks against the old pairing message.
- Preserves port 1337 and all existing controller, file, screenshot and app-management commands.
