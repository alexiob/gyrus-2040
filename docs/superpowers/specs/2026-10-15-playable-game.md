# Playable native Gyruss-inspired game

User authorized A-D implementation with subagents on15October2026. This extends
the approved architecture. Root owns portable simulation/renderer/integration;
parallel GPT-6.1Sol/high workers own controls, debug tooling/hosts and assets.

Build a complete single-player orbital shooter: title, start, combat, deaths,
respawn, game over, pause/quit, six-planet route, chance stages and looping
increased difficulty. Use original authored art inspired by arcade visual roles,
not ROM content. Simulation60Hz uses integer angles/radii, bounded entity pools,
seeded RNG, independent presentation and no hot-loop heap allocation.

Controls ship with selectable split hold and relative angular drag. BOOT fires.
RUN/RESET is a physical reset pin and cannot implement short-quit/5s-hold in
firmware. The user selected a pause menu with separate Quit and Reboot actions;
Reboot requires a continuous five-second hold. Input module
owns debouncing, dead zone, wrap handling and gesture cancellation. Rendered
controls must use the same hit rectangles as the input module.

Debug protocol G1 has request IDs, bounded ASCII lines, structured replies,
allowlisted state mutations, pause/step/input injection, stage skip, seed reset,
statistics and exact frozen framebuffer capture with explicit sizes and CRC.
Use the same protocol over device USB and local headless/desktop transports.
Desktop binds loopback only; no arbitrary memory/register writes.

Keep the diagnostic UF2 alongside the game UF2. The game shares portable C
rendering with a Sokol desktop app; headless runner works without graphics
libraries. Device core0 simulates/samples input, core1 expands indexed frames
into RGB565 strips and transfers SPI1 DMA at qualified62.5MHz. Immutable frame
ownership uses Pico queues/barriers. Core1 enables flash-safe lockout so BOOT
sampling can temporarily release flashCS safely. Avoid raising CPU clock.

Game tests cover deterministic replay, capacities, angle wrap, collisions,
score/phase/route transitions and chance safety. Input/debug workers supply
focused tests; root runs full sanitizer host suite and ARM builds. Use debug
protocol to capture and inspect actual screenshots, inject input, skip levels,
read state, and exercise long scripted runs. Flash only connected identified
board; ask for physical BOOT/touch/visual feedback when necessary. Record
observations versus software measurements and retain known hardware limits.
