# Features Under Review and Development

- [x] Export of logs and playback reports
- [x] Drag and drop support for playlist items
- [ ] Playlist scheduling system (automatic programming by time/date)
- [x] Automatic insertion of jingles at configurable intervals (N jingles every Y tracks)
- [ ] Migrate QMediaPlayer → libmpv (mpv_handle* wrapper in AudioPlayer/ButtonHole/timeplayer/preview)
      — enables real-time audio filters (af-command), per-sink output
      (audio-device), may drop transcodeIfNeeded; retest fade/crossfade
      and playlist advance
- [ ] Built-in graphical equalizer (10 bands 31Hz–16kHz, real-time sliders
      via af-command on mpv — depends on the migration above)
- [x] Automatic saving of settings and playlists
- [x] Streaming integration (Icecast / Shoutcast) — on/off button, URL/user/pass + name/description, curr playing via /admin/metadata
- [x] Automatic silence or audio failure detection
- [x] Audio preview (pre-listen) — context menu, 15s, own sink (audio/preview_device)
- [x] Select output device for broadcast and monitoring separately — broadcast sink (audio/output_device) + monitoring loopback on default sink
- [x] Add items to the playlist below a pre-selected entry
