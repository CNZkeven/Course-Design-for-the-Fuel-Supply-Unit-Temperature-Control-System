# J-Link One-Click Download Script Design

Date: 2026-03-17

## Summary
Add a minimal, one-click download workflow on Linux (bash) that programs the already-built HEX file to the target MCU using J-Link. The flow will be a root-level `download` script (executable bit set) that invokes `JLinkExe` with a simple command file `download.jlink`.

## Goals
- Provide a single-click or one-command way to flash the existing `build/ITEMP.hex`.
- Use J-Link (not ST-LINK) and keep dependencies minimal.
- Keep the workflow stable across terminal working directories by using script-relative paths.

## Non-Goals
- Do not build or rebuild the project.
- Do not change CMake, build system, or VS Code debug settings.
- Do not support multiple J-Link devices or device selection logic.

## Approach (Recommended)
Use `JLinkExe` with a command script so the flow is transparent and easy to modify later. The shell script provides preflight checks and a friendly error if dependencies or files are missing.

## Components
1. `download` (executable shell script)
- Verifies `JLinkExe` is in `PATH`.
- Verifies `build/ITEMP.hex` exists.
- Resolves its own directory and runs `JLinkExe -CommandFile download.jlink` from that directory.
- Script is committed with executable permission to satisfy “single-click” usage.

2. `download.jlink` (J-Link Commander script)
- Sets device to `STM32F407ZG`.
- Uses `SWD` interface and `speed auto`.
- Exact command sequence:
  - `device STM32F407ZG`
  - `if SWD`
  - `speed auto`
  - `connect`
  - `r`
  - `loadfile build/ITEMP.hex`
  - `r`
  - `g`
  - `exit`

## Data Flow
- User clicks `download` (or runs `./download`).
- Script checks prerequisites and calls `JLinkExe`.
- `JLinkExe` reads `download.jlink` and programs `build/ITEMP.hex`.
- J-Link exits with status; any errors are shown in the terminal output.

## Error Handling
- If `JLinkExe` is missing, exit with a clear message.
- If `build/ITEMP.hex` is missing, exit with a clear message.
- J-Link errors are surfaced directly to the user for diagnosis.

## Configuration Defaults
- Device: `STM32F407ZG`
- Interface: `SWD`
- Speed: `auto`
- HEX path: `build/ITEMP.hex` (relative to project root)

## Validation
- Manual: run `./download` from the project root and confirm J-Link output ends without errors and the board runs the new firmware.
- Manual: run `./download` from a different working directory to confirm script-relative path stability.

## Future Extensions (Optional)
- Add a `--serial` option if multiple J-Link devices are used.
- Add a `--hex` option to override the target file.
- Add optional build step or verify step if needed.

## Notes
- No extra `JLinkExe` flags are required beyond `-CommandFile`.
