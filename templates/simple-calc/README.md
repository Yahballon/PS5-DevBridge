# Simple Calc starter (AI-assisted)

A native PS5 calculator built with the GPL ps5-homebrew-ui renderer. Title ID PPSA99339. D-pad selects a key; X activates; Circle exits.

Build on Linux/WSL: `make USE_CCACHE=0 BUILD_JOBS=4 app`. Supply the PS5 SDK/OpenGL dependencies or use the included pinned dependency scripts. Output is `dist/PPSA99339/`.

Own-app crash reporting is included: `/data/ps5-devbridge/crashes/PPSA99339.txt`. Address stack frames are best effort; preserve an unstripped debug ELF to symbolize them. Reports are written only by an opted-in app and cannot reconstruct crashes of unrelated apps.

Framework: https://github.com/blackbearreloaded/ps5-homebrew-ui
