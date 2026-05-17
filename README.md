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

To rebuild after editing:

```
gcc -o scripts/color-scheme src/color-scheme.c
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
3. **Check compilation** – Rebuild with full warnings:
   ```
   gcc -Wall -Wextra -o /tmp/test src/color-scheme.c && /tmp/test
   ```
4. **Insert debug prints** – Add `fprintf(stderr, "debug: ...\n");` to trace
   execution flow and variable values.
5. **Test commands manually** – Run the `dconf` or `notify-send` commands the
   script would execute to see their exact output/errors.
6. **Check exit codes** – If the script returns early (`return 1`), follow up
   with `echo $?` to confirm.
