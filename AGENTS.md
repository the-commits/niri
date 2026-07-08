# AGENTS.md — niri config

Personal [niri](https://github.com/YaLTeR/niri) Wayland compositor configuration by Magnus Åberg.

## No tests / CI / linting

This is a dotfiles repo. Do not attempt to run tests, lint, or typecheck.

## Build (compiled C helpers)

```
make              # build all src/*.c → scripts/<stem> binaries
make color-scheme # build a single binary
make clean        # remove all compiled binaries
```

- C sources in `src/` use **no libc** — raw `syscall()`, custom `_start` instead of `main`.
- Makefile uses extreme size-optimization flags (`-nostartfiles -Os -s ...` + post-link `strip`).
- `scripts/` is gitignored (compiled output).

## Architecture

- **`config.kdl`** — main entrypoint loaded by niri. Defines inputs, outputs, ~50 keybindings, startup apps, layout, animations, window rules, environment variables.
- **`src/color-scheme.c`** → `scripts/color-scheme` — toggles GNOME dark/light mode via `dconf`, sends desktop notification. Bound to `MOD+SHIFT+C`.
- **`src/cliphist-picker.c`** → `scripts/cliphist-picker` — clipboard history picker with sensitive-entry support. Pipes `cliphist list` into `wofi`, handles paste-once for sensitive entries, deletes them after use. Bound to `CTRL+SHIFT+V`.
- **`src/cliphist-tag-kdeconnect.c`** → `scripts/cliphist-tag-kdeconnect` — long-running daemon that monitors D-Bus for KDE Connect clipboard shares and tags them as sensitive entries in cliphist.
- **`src/monitor_ctl.c`** → `scripts/monitor_ctl` — DDC/CI monitor input switch. Uses `ddcutil` to toggle between DP/DVI on Fujitsu B24W-7.

## Keybindings (notable)

- `MOD+SHIFT+C` — toggle color scheme (the compiled C binary)
- Vim-style navigation: H/J/K/L for left/down/up/right
- `CTRL+SHIFT+1/2/3` — screenshot area/screen/window
- `MOD+SHIFT+P` — power off monitors

## Git

- Primary remote: `git.sr.ht:~the-commits/niri`
- Mirror: `github.com:the-commits/config-niri.git`
- Do not push without asking.
