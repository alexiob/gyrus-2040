# Shared engine and Tempest implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Run Gyrus and classic Tempest through one portable engine and a retro game carousel on the existing board and desktop frontends.

**Architecture:** Neutral input/drawing/UI contracts, an immutable game registry, and a session with one active-game union. Existing hardware transports/presentation remain; immutable palettes travel with queued frames. Game-specific code owns rules and state serialization.

**Tech Stack:** C11, Pico SDK, fixed 60 Hz simulation, indexed raster renderer, SPI DMA, Sokol/Metal, existing G1 protocol.

**Spec:** `docs/superpowers/specs/2026-10-16-multigame-tempest.md`.

## Global constraints

- C11; no per-frame allocations; explicit bounds for all pools.
- 240 × 240 round output, 125 MHz CPU/peripheral, 48 MHz USB, 62.5 MHz SPI.
- Preserve 130,560-byte pixel-buffer layout and existing flash/BOOT/USB safeguards.
- Keep Gyrus simulation and standalone G1 adapter deterministic and compatible.
- Classic Tempest mechanics; optional 2000 effects alter presentation only.
- Original authored runtime assets; ignored reference downloads with provenance.

## Review focus

- Queued old-game pixels retain their original immutable palette after switching.
- A held touch or BOOT across selection does not launch/fire/zap accidentally.
- Open webs stop movement/flips at ends and never create a closing lane.
- Pause/death during a spike dive does not duplicate level advance or score.
- Selecting another game retains separate highs/settings and does not leak entities or injected pulses.

## Task 1: Research and regression baseline

**Files:** research reports/manifests, `.gitignore`, ignored `artifacts/multigame/gyrus-before/`.

- [x] Create `feat/multigame-tempest` from the current main branch.
- [x] Save the pre-refactor headless executable for regression comparisons.
- [x] Research original Tempest and Tempest 2000 independently; collect primary manuals and artwork references.
- [x] Confirm the clean baseline: `cmake --build --preset host && ctest --preset host` (21 passes).
- [x] Commit research/provenance and the spec/plan without downloaded copyrighted reference files.

## Task 2: Neutral resources and graphical frontend

**Files:** `include/engine/{display,input,draw,ui}.h`, `src/engine/{draw,font,ui,hub}.c`, Gyrus compatibility headers/render/assets and asset generator as needed.

**Interfaces:** `engine_input` preserves old input/menu values; `game_input` is its compatibility alias. `engine_ui` carries sensor/control telemetry. Shared draw primitives consume indexed pixels; shared menu rendering consumes a small value-only view. `engine_hub_render()` consumes a const session, writes pixels, and returns no mutable state.

- [x] Extract input/display contracts and exact integer graphics/font without changing Gyrus output.
- [x] Extract common pause/Settings rendering; preserve old layout and pixels for Gyrus.
- [x] Add graphical Gyrus and Tempest wordmarks and the horizontal launcher; classic/neon selector on Tempest.
- [x] Test pure rendering, palette/buffer bounds, circular safety, slide states and readable titles.

## Task 3: Classic Tempest module

**Files:** `include/tempest/game.h`, `src/games/tempest/{game,webs,render,adapter}.c`, `tests/test_tempest*.c`, `docs/tempest.md`.

**Interfaces:** one bounded `tempest_state`; init/step/render/view/setting/level/hash/state-JSON operations match the descriptor in `engine/game_api.h`. Coordinates and updates are fixed point/integer; rendering uses shared drawing/UI and an immutable palette.

- [x] Write failing tests for open/closed adjacency and eight-shot capacity; implement web data and claw/shots.
- [x] Add enemies and progression, pinning the original manual's scoring and stage introductions in tests.
- [x] Test and implement Zapper eligibility/two charges, tanker splits, rail vulnerability, pulses, persistent spikes and dangerous dives.
- [x] Test pause/death/advance, seed determinism and cosmetics leaving the hash untouched.
- [x] Add classic vector and bounded neon rendering, then inspect early/late/dive/game-over captures.

## Task 4: Session, registry and shared debug coordination

**Files:** `include/engine/{game_api,session,debug_session}.h`, `src/engine/{session,catalogue,debug_session}.c`, `src/games/gyrus/adapter.c`, `tests/test_engine*.c`.

**Interfaces:** immutable `engine_game_ops`, one `engine_state` union, selection/home/step/update/render/view entry points. `engine_update()` owns physical/debug input merging. A transport adapter exposes the existing `gyrus_debug_callbacks` over the session.

- [x] Test valid/invalid selection, union reset, independent records and input quarantine before implementing lifecycle.
- [x] Implement shared input/freeze/settings/UI coordination and descriptor dispatch; keep frontends game-agnostic.
- [x] Add game catalogue/selection/home and special input to the existing bounded parser; no second protocol.
- [x] Compare selected Gyrus directly against the saved legacy runner for state/frame equality.

## Task 5: Platform integration and touch actions

**Files:** `firmware/game_main.c`, `host/{desktop.m,headless.c}`, `src/{controls,video,debug}.c`, associated headers/tests, CMake and Python tools.

- [x] Add launcher navigation, generation-aware held-input consumption and one-shot Zapper touch/BOOT actions; retain existing control tests.
- [x] Add palette-explicit strip expansion and test queued-frame palette lifetime.
- [x] Replace direct Gyrus frontend calls with session/update/render/debug APIs.
- [x] Run all contracts and build desktop plus RP2040 targets.

## Task 6: Review, board verification and delivery

- [x] Review architecture/game rules/renderer with fresh agent context; fix actionable findings.
- [x] Check ELF/static RAM and stack sizes against existing reservations.
- [x] Flash and verify the final build on the connected board; compare device/native frames for launcher and both games.
- [x] Measure sampled renderer and simulation time at existing clocks; inspect device framebuffer captures.
- [x] Update README, protocol/game/engine docs and evidence; commit on the feature branch.

## Task 7: Tempest transition and action effects

Added to the active scope by the user's request for level intro/exit and
in-level special effects.

- [x] Add a visible level-entry phase and animated web exit/flight sequence.
- [x] Add bounded shot, hit, arrival, Pulsar and Superzapper feedback.
- [x] Keep classic vector presentation and stronger optional neon effects;
  exclude cosmetic events from canonical gameplay hashes and RNG.
- [x] Test transition timing, pause/resume, visual bounds, event purity and
  rendering cost; capture the final scenes on the board.

## Task 8: Correct Gyrus wave fidelity

Added by the user's report that entry/attack patterns and offscreen completion
do not match the original game. The refactor compatibility baseline above is
complete; arcade fidelity now intentionally supersedes the old timeout behavior.

- [ ] Verify entrance, center formation, repeated attacks/returns and last-two escape against original evidence.
- [ ] Replace global-age expiry and pool-index paths with explicit bounded wave/lifecycle data.
- [ ] Add regressions for offscreen returns, no-fire persistence, formation gating and chance-stage separation.
- [ ] Update deterministic transition scripts and documentation, review, rebuild and reflash.
