# Tempest 2000 (Atari Jaguar, 1994): reference study

Research date: **2026-10-16**. Scope: the original Jaguar cartridge. This is a rules/visual reference for an original native-C implementation on RP2040; downloaded material is reference-only. No ROM, game executable, source-code dump, ripped sprite, font, model, sample or music was obtained.

**Current product decision:** the first Tempest game retains the 1981 arcade rules, with optional Tempest 2000-style visual effects. Jump, power-ups, AI partner, warp bonuses, keys and additional enemies below are research for a possible later game/mode, not requirements for the first game. The cosmetic renderer must not silently alter the classic game state.

## Evidence and version boundary

- **M — original Atari manual:** [scanned PDF](https://www.digitpress.com/library/manuals/jaguar/tempest%202000.pdf), local `reference/tempest/2000/manual.pdf`; 16 PDF pages. Printed page numbers are two lower than PDF page numbers after the cover. [AtariAge transcription](https://static.atariage.com/manual_html_page.php?SoftwareID=2528) is searchable but the scan is the primary artifact. Programming credit: Jeff Minter; art: Joby Wood. The manual explicitly describes the four modes and marks the added enemies/power-ups “Tempest 2000 only.”
- **P — contemporary player account:** [A.P.E., Spring 1994, issue 6](https://www.ataricompendium.com/archives/newsletters/ape/ape_spring94.pdf), printed/PDF pp.6–7. Scanned, visually read; local OCR is only a navigation aid. This is a period player’s firsthand tips, **not an Atari specification**. It supplies power-up order, bonus controls and tactics absent from M.
- **D — creator testimony:** [Jeff Minter and Atari’s Dan McNamee, GEnie conference, 29 March 1995](https://www.neperos.com/article/pwn9dxa6e78792e5), archived in Atari Explorer Online Jaguar Special Edition 03. Minter describes testing rotary control with a converted Indy 500 controller and finding the lack of flywheel weight unsatisfactory. This supports treating d-pad control as the documented retail baseline, not requiring an arcade spinner.
- **I — later creator interview:** [Brandon Sheffield interviews Jeff Minter, 4 April 2007](https://www.gamedeveloper.com/design/llamas-in-space-catching-up-with-llamasoft-s-jeff-minter). First-person context about learning polygon graphics for T2K; most of the graphics/readability discussion concerns Space Giraffe, so it must not be mistaken for a T2K engine specification.
- **S — preserved stills:** [LaunchBox Jaguar image listing](https://gamesdb.launchbox-app.com/games/images/3769-tempest-2000), local title/gameplay/high-score/game-over PNGs and a scan of the original Atari advertisement. These are visually inspected screenshots/scans, hosted by a third party. The capture hardware/emulator and scaling are not documented; do not infer native resolution or precise RGB values from them.
- **V — gameplay recording provenance:** [World of Longplays entry 13363, georgc3](https://longplays.org/infusions/longplays/longplays.php?longplay_id=13363), [recording](https://www.youtube.com/watch?v=1Bj3GM148OM). Listed system: Atari Jaguar; publication 31 October 2022; YouTube release 19 April 2023; 1:16:29. The author’s accompanying still is downloaded and inspected. The full video is linked, **not downloaded or frame-audited in this pass**; real-hardware capture is not established. A second [RetroDetect Jaguar-labelled longplay, 21 May 2015](https://www.youtube.com/watch?v=AxB-gnUXx7o) is a follow-up reference, also not frame-audited.

Never substitute DOS/Windows/Mac/Saturn ports, PlayStation Tempest X3, Tempest 3000, TxK or Tempest 4000 behavior for Jaguar evidence. Even the Jaguar “Traditional” mode is a recreation; the original 1981 arcade reference remains authoritative for the first game's rules.

## Four cartridge modes

| Mode | Documented distinction (M, printed pp.4–5) |
|---|---|
| Traditional Tempest | Arcade-style rim shooting and original enemy families. |
| Tempest Plus | Solo, AI-assisted solo, or cooperative two-player play; partners share lives. |
| Tempest 2000 | Single-player expanded campaign, collectibles, new enemies and bonus warps. |
| Tempest Duel | Competitive two-player; player mirrors reflect incoming shots but open while firing. Generator/cube objects and Flippers complicate the duel. |

The “100” campaign levels and Beastly unlock are the manual's claims. V’s uploader informally says 99 levels; that is a discrepancy, not a reason to change the documented count. Exact stage numbering, final transition and Beastly schedule remain unaudited.

## Added 2000 mechanics and progression

The critical additions over 1981 are **collectible upgrades, airborne movement/firing, a helper, bonus-round travel, additional enemy behaviors, persistent keys and a more elaborate visual treatment**. Classic firing, circumferential lane changes and the emergency SuperZapper remain the foundation. Traditional/Plus/Duel are distinct rule sets rather than switches that should be accumulated into one ambiguous game.

P p.6 gives this usual pickup sequence:

`Particle Laser → 2,000 points → Jump → 2,000 points → AI Droid → Warp Token`

P describes pickups after a few enemy kills; their starting appearance is red and they change colour as they form. It also reports that catching one during the end-of-web departure changes the next web’s first pickup to a droid. The manual lists pickup functions but **does not prescribe their order**. A [later Jaguar FAQ](https://gamefaqs.gamespot.com/jaguar/586908-tempest-2000/faqs/55664) describes an extra `Zappo 2000` between Droid and Warp, and another after Warp. That later account is a lead for gameplay verification, not proof of the exact state machine. Kill thresholds, missed-pickup behavior and power reset on death/web change need direct play measurement before implementing a faithful 2000 mode.

| Item/event | Rule/effect | Evidence status |
|---|---|---|
| Particle Laser | Upgrades attack power; P reports faster removal of enemies/spikes. | M + P documented. Exact projectile rate/damage unknown. |
| Jump Enabled | Unlocks A; steering and firing remain available in the air. | M documented; P recommends it for rim enemies and pulsing lanes. Height, duration, cooldown and immunity interval unmeasured. |
| AI Droid | Helper fire on the web; Plus can supply a helper from the outset. | M documented. P describes it as particularly useful against rim attackers. Autonomous targeting, invulnerability and lifetime are not specified by M. |
| Zappo 2000 | +2,000 points. | M documented. |
| Warp token | Immediate free zap; contributes to a three-token bonus entry. | M documented. Token persistence across deaths/web changes is not measured here. |
| Out of Here | Ends web; +5,000 points. | M documented. Exact rarity/trigger unresolved. |
| SuperZapper | First activation clears enemies; second kills a single enemy; recharges for each web. | M documented. Do not confuse this standard button action with the token's free zap. |

On successful bonus completion, M specifies **20,000 points and five-level advancement**. It says there are three bonus types; P names/describes them:

1. **Ring warp:** steer through rings, with better centring worth more; later variants use upper rings and speed changes.
2. **Tunnel surfing:** remain on a moving green path; a short departure is recoverable.
3. **Blurry rings:** steer through blurred rings; later variation adds speed changes and vertical adjustment.

Those names are P's descriptive labels, not established official menu strings. Do not invent a combat bonus arena. Bonus failure penalty, ring counts, scoring formula and exact entry/exit timing remain open.

M also documents saved start **Keys** after progress beyond level 17, returning to the last completed odd-numbered web, with up to four keys. Beastly unlock follows completion of the advertised 100-web campaign and makes the player's firing weaker/slower while enemies become more aggressive. These are progression features beyond a minimal first release.

## Enemies and hazards

M printed pp.7–8 distinguishes these families; the behavior notes below are compact factual paraphrases. P p.6 supplies the tactics column where indicated.

| Family | Functional identity | 1981 inheritance / 2000 addition |
|---|---|---|
| Flipper | Climbs, changes adjacent lane by flipping, captures at rim. | Inherited. |
| Spiker | Builds variable-height spike, retreats, repeats elsewhere; shoots. | Inherited. |
| Spike | Persistent centre-lane obstacle, shortened by hits; dangerous during exit dive. | Inherited. |
| Fuseball | Moves along lane boundaries and across lanes; boundary position resists shots. | Inherited; P warns against waiting for it at rim. |
| Pulsar | Alternates line/zigzag; pulse makes its whole lane dangerous. | Inherited; P says 2000 also moves along rim, making jump useful. |
| Tanker | Climbs a lane and releases Flippers/Fuseballs/Pulsars. | Inherited; payload variants matter separately. |
| Mutant Flipper | Faster, more aggressive Flipper. | Explicit 2000 addition. |
| Mirror | Reflects shots; vulnerable while attacking. | Explicit 2000 addition; P recommends fire-and-dodge. |
| Demon Head | Detached horn attacks player after a hit. | Explicit 2000 addition. |
| UFO | Above-web attacker requiring airborne attack. | Explicit 2000 addition. |

Treat horns, missiles, spikes and electrified lanes as different hazard categories. Their render shapes may share a vector renderer but collision logic should not be inferred from appearance. The original Atari ad supplies useful independent close-up illustrations of Demon Heads, UFOs, Pulsars, Fuseballs and Flippers.

## Controls, scoring and persistence

M printed pp.2–3: d-pad left/right moves around rim; **B fires, A jumps when enabled, C zaps**. Assignments can be changed. Keypad groups `1/4/7`, `2/5/8`, `3/6/9` select default tracking, locked and close tracking views respectively. The manual's direction wording is left=clockwise/right=counterclockwise; preserve physical user intent when adapting a new control scheme instead of treating that wording as a screen-coordinate formula.

M printed pp.6/9: initial stock is **one active plus three reserve** blasters; another reserve every **20,000** points. Its enemy table gives:

| Target | Points |
|---|---:|
| Flipper / Mutant Flipper / Demon Head | 150 |
| Spiker | 50 |
| Spike hit | 10 |
| Pulsar | 200 |
| Tanker and payload variants | 100 |
| Fuseball / Mirror / UFO | 250 / 500 / 750 |

The conditions selecting 250/500/750 are not explained by the manual; do not arbitrarily assign a distance multiplier. Start-level bonuses exist. High score, volume and key data are cartridge-persistent in the original; this does not imply that the RP2040 build must write flash on every score change.

## Webs and visual identity: observations, not extracted assets

Visually inspected S title still: black background; large cyan/blue extruded angular “TEMPEST”; iridescent, outlined “2000”; cyan/blue contour-like depth geometry behind it. The original printed ad instead uses a red/orange/yellow logo over large outlined 2000 numerals: **packaging identity is not the in-game title treatment**.

S gameplay still: triangular closed rim; strong blue lane boundaries; blue-to-magenta filled lane shading; small central hole; yellow claw-shaped player; green projectiles; sparse coloured star pixels. V’s still: an asymmetric bowed polygonal web, magenta edges, shaded sectors, large pixel burst near its core, yellow player and reserve icons, coloured numeric score. These prove shape variation and depth treatment, not a complete 100-web topology catalog. A renderer should allow arbitrary open or closed rim polylines, but whether a specific proposed shape belongs to Jaguar T2K requires a matching frame. Do not call every web a circle, or invent a fixed lane count for every shape.

M advertises polygons, particles, colour cycling and “Melt-O-Vision”; the last is a marketing effect name, not an algorithm specification. Screenshots alone cannot establish persistence length, blending equations, frame rate, audio synchronisation or camera easing. V's uploader indexes colour phases at **1:27 blue, 12:31 red, 20:42 yellow, 31:36 multicolour transparency, 40:59 purplish pink, 53:19 orange, 1:03:03 white**. These are useful future video checkpoints, explicitly uploader-supplied rather than frame-audited observations.

## RP2040 recommendations (engineering judgement)

Target assumptions supplied by project: 125 MHz CPU, 264 KiB SRAM, 240×240 round display, shared game palette, no audio hardware. These proposals are **adaptations**, not original-game facts.

| Feature | Practical adaptation | Rule impact / priority |
|---|---|---|
| Mesh colour cycling | Cycle palette indices/lookup colours; keep a stable bright rim and player contrast. | Cosmetic, high priority. |
| Depth shading | Precompute several shades for the shared hues and interpolate depth with fixed-point/LUT math. | Cosmetic, high priority. |
| Enemy explosions | Fixed pool, e.g. 32–64 short-lived points/segments; reserve brighter colours for threats/player. | Cosmetic, high priority. |
| Title depth | Original vector lettering with repeated offset contours, slow geometry rotation and bounded stars. | Cosmetic, high priority; draw new lettering rather than shipping a copied logo. |
| Warp transition | Scale the web toward camera for a brief completion transition, retaining spike collision timing from classic rules. | Cosmetic only if simulation timing/collision remain classic. |
| Camera motion | Subtle optional movement; locked view default on a tiny round display. Never move hitboxes with cosmetic shake. | Cosmetic, optional. |
| Feedback trails | Keep 2–3 previous object positions or redraw a dim outline; avoid a separate full RGB persistence surface. | Cosmetic, optional. |
| Broad blur / filled polygon overload | Avoid full-screen blur and transparent overlays in the first build; preserve rim/shot readability. | Low priority; profile before adding. |
| Jump/droid/laser/tokens/new enemies | Separate later rule configuration and explicit UI choice. | Changes gameplay; excluded from first game. |
| Music / spoken pickup calls | Use clear text/icon flashes for relevant events; no sampled soundtrack or voice assets. | Hardware adaptation, not sound emulation. |

Memory arithmetic: one 240×240 RGB565 framebuffer is **115,200 bytes (112.5 KiB)**; two use **230,400 bytes (225 KiB)**, leaving only **39 KiB** of nominal 264 KiB for everything else. An 8-bit indexed buffer is **57,600 bytes (56.25 KiB)**; a 16-colour packed buffer is **28,800 bytes (28.125 KiB)**. Shared palette lookup, bounded particle pools, ROM-resident meshes and scanline/LUT projection are sensible candidates. These figures exclude stacks, DMA buffers, code/data copied to RAM and display driver storage; they are not a measured performance promise. Keep display transfer timing and frame pacing measurable before choosing double buffering or extensive fills.

For a round screen, centre the web inside the visible circle; place score/reserves in the safe upper arc and avoid corner-only prompts. Keep cosmetic state owned by the renderer and reset it on game switch. The classic game owns spawn cadence, shots, lane positions, enemy decisions, collisions, scoring and zap availability.

## Local reference handling

`reference/tempest/2000-manifest.json` records each downloaded/derived file’s URL or parent artifact, retrieval date, byte count, SHA-256 and rights note. Downloads are intended to remain ignored; only this research report and provenance manifest should be committed by the parent task. A PDF page render/contact sheet/OCR is still copyrighted reference material, not an eligible production asset. No permission to redistribute Atari artwork, game screenshots, magazine scans, creator interviews or recordings is implied by public availability.

Open fidelity work for any future 2000 mode: frame-audit power-up chain and resets, jump timing/unsafe intervals, droid targeting/lifetime, pickup release thresholds, exact web catalogue/lane counts, variable enemy scoring, bonus failure/entry timing, final-level numbering and port differences. These open questions do not block the classic-rules plus optional-effects first game.
