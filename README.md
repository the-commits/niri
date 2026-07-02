[![sourcehut](https://img.shields.io/badge/sourcehut-~the--commits/niri-2d6b9e?logo=sourcehut)](https://git.sr.ht/~the-commits/niri)
[![GitHub mirror](https://img.shields.io/badge/GitHub-the--commits/niri-181717?logo=github)](https://github.com/the-commits/niri)

# niri Config

My [niri](https://github.com/YaLTeR/niri) Wayland compositor configuration.

## Requirements

- `gcc` – C compiler
- `make` – Build tool
- `dconf` – Read/write GNOME settings
- `notify-send` – Desktop notifications (part of `libnotify`)

## Structure

- `src/` – C source files
- `scripts/` – Compiled binaries (gitignored)

## Scripts

### `scripts/color-scheme` (source: `src/color-scheme.c`)

Toggles the GNOME color-scheme between `prefer-dark` and `prefer-light` via
`dconf`. Bound to `MOD+SHIFT+C` in `config.kdl`.

To build everything:

```
make
```

To build just this program:

```
make color-scheme
```

## Debugging

When something doesn't work, start by isolating the problem:

1. **Check dependencies** – Are the required tools installed?
   ```
   which dconf notify-send
   ```
2. **Read the current value** – Verify `dconf` is working:
   ```
   dconf read /org/gnome/desktop/interface/color-scheme
   ```
3. **Check compilation** – Rebuild:
   ```
   make color-scheme
   ```
4. **Test the dconf command manually** – Run the command the program executes:
   ```
   dconf write /org/gnome/desktop/interface/color-scheme "'prefer-dark'"
   ```
5. **Check exit codes** – Run the binary and check the exit code:
   ```
   ./scripts/color-scheme; echo $?
   ```
