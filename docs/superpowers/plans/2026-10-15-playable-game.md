# Playable native game implementation plan

> **For agentic workers:** use the explicitly requested subagent workflow with root integration. Worker files are individually owned; shared interfaces in include/gyrus/game.h.

**Goal:** Deliver playable C firmware, a Sokol desktop host, original optimized assets, selectable controls, and a working debug protocol/tool.
**Architecture:** Approved portable C simulation/indexed renderer, Pico DMA display backend and Sokol desktop host; fixed60Hz logic independent of rendering.
**Spec:** ../specs/2026-10-15-playable-game.md

## Constraints and review focus

125MHz CPU/peri,62.5MHz SPI, GPIO13 reset, fixed pools, no pixel-loop allocations.
Game_input turn{-1,0,1}, angle_delta signed16, explicitfire/start/pause/quit.
Check input wrap/release, physical reset impossibility, QSPI sampling across
cores, frame ownership, screenshot framing, bounds/state mutation, game
termination and route progression, SRAM headroom and severe scheduling stalls.

- [ ] A controls worker: inspect schematics, document mapping, implement/test split and angular drag, safe BOOT sampling and on-screen actions.
- [ ] B instrumentation worker: bounded G1 protocol/tests, exact frame capture, Python client, headless and Sokol adapters.
- [ ] C assets worker: author sprites/font/planets/palette, reproducible conversion and native-size preview/size report.
- [ ] D root: tests first for simulation, implement combat/route/chance/hazards/scoring/lives/title and pure renderer.
- [ ] Root integrate all CMake targets, device indexed frame/strip ownership and controls/protocol wiring.
- [ ] Run full host tests, scripted debug integration, inspect captures, clean ARM build/map; fix failures.
- [ ] Independent review, fix important issues; build/flash game, verify actual G1 state/input/step/skip/capture and physical controls; record results.
