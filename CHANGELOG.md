# Changelog — LaraRadio

All changes between the original `gutierre69/lararadio` release
and this fork (`brdelphus/lararadio`).

Original source: https://github.com/gutierre69/lararadio
Fork: https://github.com/brdelphus/lararadio

---

## [1.0.7] — 2026-09-02 — Loop button: VU meter, output routing and now playing

### Fixed

- **VU meter stayed silent while the Loop button played** — the loop's
  `QMediaPlayer` had no audio buffer feeding the meter. New
  `ButtonHole::setBuffer()` shares the same `QAudioBufferOutput` already
  used by the playlist players, so the UI level meters react to loop
  playback too.
- **"NO AR" title only turned red while the playlist was playing** —
  pressing Loop (which takes over the program with a playlist fade-out)
  left the group title grey even though audio was on air. The `flash()`
  state check now also watches the loop player, so the red on-air
  indicator reflects the loop as well.

### Changed

- **Every sound the app generates now honors the configured output
  device** — all ten ButtonHoles and the Loop gained
  `ButtonHole::setOutputDevice()` and are routed to `audio/output_device`
  at startup (previously only the two playlist players and the time
  player were). Combined with a PulseAudio combine sink
  (`lr_combo` = broadcast + default output), all playback goes to the
  Icecast broadcast *and* to the operator's headphones simultaneously.
  The pre-listen player keeps its own `audio/preview_device` and never
  goes on air.
- **Now playing reflects the Loop** — pressing Loop (which takes over
  the program) now updates the "NO AR – Reproduzindo agora" label and
  the Icecast metadata. `updateStreamMetadata()` gained an optional
  override title; empty keeps the playlist behavior.

---

## [1.0.6] — 2026-08-17 — Preview, streaming, i18n hardening

### Added

- **Audio preview (pre-listen)**: the playlist context menu gained a
  "Pré Escuta" action (it was a disabled placeholder) that plays the
  first 15 seconds of a track in a separate `QMediaPlayer`/`QAudioOutput`
  pair, on its own sink (`audio/preview_device`, empty = system default).
  Clicking the same item again stops the preview; clicking another item
  switches. Independent from the playlist and from the broadcast — nothing
  leaks into the on-air signal.
- **Streaming integration (Icecast / Shoutcast)**: the app now streams the
  broadcast sink's monitor to an Icecast/Shoutcast server through an
  ffmpeg child process (MP3 128 kbps). A new toggle button below TALK
  (share icon) starts/stops the stream and turns green while on air.
  ConfigDialog gained a "Streaming" group with server URL, user and
  password (`stream/url|user|pass`) plus stream name and description
  (`stream/name|description`, sent as `ice_name`/`ice_description`).
  The ffmpeg source client identifies itself as `LaraRadio`
  (`-user_agent`). The stream button stays disabled until the config is
  valid, and re-evaluates when the ConfigDialog closes.
- **Live on-air metadata (curr playing)**: every track/jingle change (and
  when the stream starts) the app updates the Icecast mount title via
  `/admin/metadata` with "title - artist" from the TagLib tags, using
  the source credentials. If the mount is not ready yet (ffmpeg still
  connecting right after stream start) the update retries after 2 s.
- **Configurable output device** (`audio/output_device`): the main players
  (both AudioPlayers + time player) can be pointed at a specific sink
  (e.g. the virtual `broadcast` sink) via `AudioPlayer::setOutputDevice()`.
  Empty = system default. Matching by device id or description.

### Changed

- **i18n**: `pt_BR.ts` header fixed (`language="pt_BR"`, was a hardcoded
  `br_FR`); empty translations in `en_US.ts` filled (Volumes, L, R);
  the playlist context menu actions (Mark as Next, Play This, Preview,
  Delete) were hardcoded strings and now go through `tr()`;
  `lrelease` is mandatory after every `lupdate` — a stale `.qm` makes
  new strings fall back to the pt-BR source.
- **Infra (insight)**: virtual `broadcast` sink (PipeWire null sink,
  persisted in `/etc/pipewire/pipewire.conf.d/`) and a monitoring
  loopback from `broadcast.monitor` to the system default sink (no
  hardcoded device, `/etc/pipewire/pipewire-pulse.conf.d/`). Local
  Icecast runs on port 9000 (runit service) — port 8000 was occupied by
  a docker container.

### Fixed

- **Stream toggle crash (2×)**: the `QProcess::finished` handler used the
  `m_streamProc` member, which is already null during manual shutdown —
  SIGSEGV in `deleteLater()` and later in `readAllStandardOutput()`.
  The handler now only uses `sender()`, and shutdown detaches the member
  before `kill()` (no blocking `waitForFinished`).
- **Stream button did not enable after saving config**: the ConfigDialog's
  `QSettings` only flushed to disk in its destructor (after the button
  re-evaluation); `accept()` now calls `sync()` immediately.
- **Stream button "didn't turn on"**: `-user_agent` is an http *output*
  protocol option — placed before `-i` ffmpeg 6.1 dies with
  "Option user_agent not found" before streaming anything. Moved after
  `-f mp3`, before the URL.
- **Orphan ffmpeg holding the Icecast mount**: killing the app with
  `kill -9` leaves the ffmpeg child alive holding "/live" ("Mountpoint
  in use" on reconnect). The app now keeps a PID lock
  (`/tmp/lararadio_stream.pid`), kills the orphan on the next stream
  start and clears the lock on process exit.

## [1.0.5] — 2026-07-27 — Changes on top of original 1.0.4

### Changed

#### `audioplayer.h` / `audioplayer.cpp`
- **Inheritance**: `AudioPlayer` changed from `QMediaPlayer` subclass to
  `QObject` subclass containing `QMediaPlayer *player` as a member
  (composition over inheritance). This decouples the audio backend from
  the public API and allows more flexible lifecycle management.
- **Volume**: default audio output volume changed from `0` to `1.0`
  (QMediaPlayer is now the actual audio source, see below)
- **MP3 transcoding**: added `transcodeIfNeeded()` — converts MP3 files
  with embedded album art to temporary WAV via `ffmpeg -vn` before
  playback. This prevents the `mp3float` decoder crash that occurred
  after multiple songs with album art.
- **Error handling**: connected `QMediaPlayer::errorOccurred` →
  `onPlayerError()` which emits `mediaError()` signal
- **End-of-track detection**: connected `QMediaPlayer::mediaStatusChanged`
  → emits `playbackFinished()` on `EndOfMedia`
- **Destructor**: explicit destructor to clean up `player` and `audioOutput`
- **Inline helpers**: `getPosition()`, `getDuration()`, `remainingTime()`,
  `isPlaying()`, `isPaused()`, `isStopped()`, `getVolume()`, `setVolume()`
  all simplified to single-line inline implementations
- **`isValidMediaFile()`**: static method using `ffprobe` to validate
  audio files before queueing
- **`hasError()`**: public error state flag

#### `main.cpp`
- **Crash handler**: added `crashHandler()` for `SIGSEGV`, `SIGABRT`,
  `SIGFPE` — prints diagnostic message to stderr and exits cleanly
  with code `128 + signal`. Prevents silent crashes from FFmpeg
  decoder bugs.
- **Dark theme**: activated the Fusion dark theme palette (was
  previously commented out). Dark background `#303030`, base `#242424`,
  text `#dcdcdc`, highlight `#55aaff`. Falls back to GTK3 theme when
  available.

#### `mainwindow.h` / `mainwindow.cpp`
- **`skipToNext()`**: public slot — resets both players, advances
  `current_play`, and calls `next()`. Used when a track errors out.
- **`checkAdvanceTrack()`**: public slot — advances the playlist when
  `playbackFinished` fires and both players are stopped. Single point
  of playlist advancement.
- **Error recovery**: `mediaError` signal from both `AudioPlayer` instances
  connected to `skipToNext()` — auto-skips on decoder errors
- **Playlist advance**: `playbackFinished` signal from both players
  connected to `checkAdvanceTrack()`
- **Time audio**: checks `QFile::exists()` before attempting to play
  the time announcement file. If missing, logs a warning and skips —
  prevents `SayingTimer` deadlock.
- **Timeplayer error handler**: connected `QMediaPlayer::errorOccurred`
  on the time player — clears `SayingTimer` and advances on error
- **Removed `flash()` double-advance**: the 500ms `flash()` timer no
  longer advances the playlist. Advancement is exclusively handled by
  `checkAdvanceTrack()` via `playbackFinished`. This prevents race
  conditions on short tracks (jingles) where both mechanisms could
  advance independently.
- **Silence / audio failure watchdog**: monitors playback **position
  advancement** via `updateDisplay()` (10ms timer). If the active
  player's `getPosition()` does not advance for 10 seconds (and no
  fade is active), automatically calls `skipToNext()`. Rewritten from
  the VU-meter version: the FFmpeg backend does not feed
  `QAudioBufferOutput`, so VU levels stayed at 0 while audio played,
  causing false-positive skips. Covers device failure, silent decoder
  bugs, and stuck pipes.
- **End of playlist stops instead of looping**: `repeat` now defaults
  to `false`. On the last track with repeat off, `checkAdvanceTrack()`
  stops playback (`isPlaying = false`) instead of wrapping to the
  start; the 5s pre-transition in `currentTimePosition()` also no
  longer wraps when repeat is off.
- **Segfault fix on playlist edit while playing**: `clearPlaylist()`
  emptied the playlist while `isPlaying` stayed true and
  `current_play` stayed stale, making `topLevelItem()` return
  nullptr → SIGSEGV. Now: `clearPlaylist()` resets both players and
  playback state; `updateAudioList()` null-checks `curItem`/`nextItem`
  before styling; `on_btn_remove_item_clicked()` clamps
  `current_play`/`next_play` after erase; `next()` clamps
  `current_play` before indexing.
- **Stop button works with empty playlist**: removing the last
  (currently-playing) track left the playlist empty while audio kept
  playing — `on_btn_stop_clicked()` early-returned on empty playlist
  and never stopped the players. Now it only early-returns when the
  playlist is empty **and** no player is playing. Also guarded
  `playlist[current_play]` access in `updateAudioList()` for the
  empty case (was out-of-bounds UB).

#### `buttonhole.h`
- Added explicit `#include <QMediaPlayer>` and `#include <QAudioOutput>`
  (were previously pulled indirectly via `audioplayer.h` when it
  inherited from `QMediaPlayer`; now it inherits from `QObject`)

### Added

#### `mainwindow.ui` / `mainwindow.cpp`
- **Repeat checkbox** (`chk_repeat`, left of the transport buttons,
  between the mic and play): toggles the `repeat` member that controls
  whether the playlist wraps at the end or stops on the last track.
  Default: off.

#### `buttonhole.h` / `buttonhole.cpp`
- **Loop button**: an 11th ButtonHole labeled "Loop" (left of the
  "Botoeira" label, y=574 line) with a new `setLoopMode()` flag. When
  enabled, the assigned audio restarts from the beginning on
  `EndOfMedia` (`mediaStatusChanged` → `player->play()`), replaying
  indefinitely. Assign audio via right-click → "Carregar Áudio", same
  as the other buttonholes (settings key `buttonhole/btn_Loop`). Click
  toggles play/stop like the others.

#### `mainwindow.h` / `mainwindow.cpp`
- **Drag & drop into the playlist**: `audio_list` accepts external drops
  (event filter installed on the tree). Audio files dropped from a file
  manager are added as `music` items; dropping a **folder** expands it
  into a sequence of `music` items (recursive scan, name-sorted — "one
  after another"). Non-audio files are ignored. Jingle/vinheta still
  comes only from its own tree.
- **`makePlaylistItem()` helper**: extracted from the TagLib item-building
  logic that was copy-pasted 3× (add button + 2 disabled double-click
  handlers). Now the single path for building a `Playlist` entry, reused
  by the add button and the drop. Bonus: `completeBaseName()` replaces
  the old `filename.remove(".mp3")` chain (removes only the final
  extension).
- **Multi-selection removal**: `audio_list` selection mode changed to
  `ExtendedSelection`; the remove button deletes **every selected row**
  (highest row first, keeping `current_play`/`next_play` clamped).
- **ON AIR light — integrated into the TALK button** (refactored from the
  standalone `onAirButton`, which was removed): the physical ON AIR light
  now follows the locution state. Clicking the mic (TALK) fires the
  configured script with the matching parameter — `onair/script` +
  `onair/param_on` / `onair/param_off` (defaults `on1`/`off1`, e.g.
  `usbrelay2 on1`) — ON when locution starts, OFF when it ends. Script
  path and parameters are set in ConfigDialog → Caminhos. Execution is
  skipped silently if the script path does not exist.
- **ConfigDialog scroll**: settings dialog now wraps its content in a
  `QScrollArea` so every field (including the ON AIR ones) stays reachable
  on small screens. The ON AIR script label shows a `~/bin/usbrelay2`
  example — resolved per-user, no hardcoded username.
- **Playlist ↔ Loop priority with fades**: playing a track in the playlist
  while the Loop is running stops the Loop and lets the track fade in;
  clicking the Loop while the playlist is playing fades the playlist out
  and hands the audio over to the Loop. `current_play` stays on the same
  track through either handover.
- **TALK ducks BOTH sources**: the TALK button now lowers the Loop volume
  as well as the playlist (`loopBh->maxVolume` fed by the same ducking
  logic; the ButtonHole flash timer ramps the volume gradually instead of
  instantly).
- **"Marcar como Próximo" fixed**: the context-menu action now actually
  moves the clicked track to right after the currently playing one (vector
  reorder + `current_play` shift), so it really plays next and the
  "Próxima" label updates. Previously it only set an index that was
  overwritten by `updateAudioList(true)`.
- **Playlist reorder drag (manual)**: internal drag & drop reorders the
  playlist keeping the `std::vector` in sync (Qt's `InternalMove` only
  reorders the tree — a known desync pitfall). Implemented as a
  `PlaylistTree` subclass with a manual `QDrag` carrying the source row in
  the mimeData; drop target is chosen by the half of the item under the
  cursor (top half = insert before, bottom half = insert after) and a
  drop indicator shows where it will land. The cursor shows a badge with
  the track name + duration while dragging.
- **Log / playback report export**: the app keeps an in-memory playback
  log (track plays, jingles, TALK on/off, Loop triggers, timestamps) and
  the ConfigDialog gained an "Export Log / Playback Report" button that
  writes `LaraRadio_playback_report.txt` in the user's home directory.
- **Clean i18n**: every user-facing string now goes through `tr()`
  (ButtonHole context menu, Loop button label, export dialogs). The Loop
  button's settings key is separated from its visible text (`setBtnKey`)
  so translating the label never breaks saved audio mappings.
  `languages/en_US.ts` now has 84 finished translations; `pt_BR` uses the
  source strings (pt-BR is the code's source language).
- **Autosave of settings and playlists**: new "Autosave" group in the
  ConfigDialog with an "Save settings and playlist automatically" checkbox.
  When enabled, the current playlist is written to
  `~/LaraRadio_autosave.txt` immediately on config OK, again on app close
  (`closeEvent`), and loaded at startup instead of the last manual
  playlist (`showEvent`). Settings already persist via QSettings. The
  ConfigDialog layout was reorganized (Volumes / Paths / Autosave /
  Export / notice) so no widget overlaps.

### Known issues (original, not introduced by us)
- TagLib `AudioProperties::length()` is deprecated in favor of
  `lengthInSeconds()` — 3 warnings during build
- VDPAU backend warning on Radeon GPUs (harmless, video acceleration
  only)
- GTK theme parsing warning with certain `gtk-contained-dark.css`
  versions (harmless)
