# Changelog — Modificações no LaraRadio

Todas as alterações entre o release original `gutierre69/lararadio`
e este fork (`brdelphus/lararadio`).

Original: https://github.com/gutierre69/lararadio
Fork: https://github.com/brdelphus/lararadio

---

## [1.0.7] — 2026-09-02 — Botão Loop: VU meter, roteamento de saída e now playing

### Corrigido

- **VU meter ficava mudo com o Loop tocando** — o `QMediaPlayer` do loop
  não tinha buffer de áudio alimentando o medidor. O novo
  `ButtonHole::setBuffer()` compartilha o mesmo `QAudioBufferOutput` já
  usado pelos players da playlist, então o VU da UI reage à reprodução
  do loop também.
- **O título "NO AR" só acendia (vermelho) com a playlist tocando** —
  apertar o Loop (que assume o programa com fade out da playlist)
  deixava o título do grupo cinza mesmo com áudio no ar. A checagem de
  estado do `flash()` agora observa também o player do loop, então o
  indicador vermelho reflete o loop.

### Alterado

- **Todo som gerado pelo app agora respeita o device de saída
  configurado** — os dez ButtonHoles e o Loop ganharam
  `ButtonHole::setOutputDevice()` e são roteados ao `audio/output_device`
  no startup (antes só os dois players da playlist e o time player
  eram). Combinado com um sink combinado do PulseAudio
  (`lr_combo` = broadcast + saída padrão), toda a reprodução vai ao
  broadcast do Icecast *e* ao fone do operador ao mesmo tempo. A
  pré-escuta mantém o `audio/preview_device` próprio e nunca vai ao ar.
- **Now playing reflete o Loop** — apertar o Loop (que assume o
  programa) agora atualiza o label "NO AR – Reproduzindo agora" e a
  metadata do Icecast. A `updateStreamMetadata()` ganhou um título de
  override opcional; vazio mantém o comportamento da playlist.

---

## [1.0.6] — 2026-08-17 — Pré-escuta, streaming, i18n endurecido

### Adicionado

- **Pré-escuta (preview)**: o menu de contexto da playlist ganhou a ação
  "Pré Escuta" (era um placeholder desabilitado) que toca os 15s iniciais
  de uma faixa num par separado de `QMediaPlayer`/`QAudioOutput`, em sink
  próprio (`audio/preview_device`, vazio = padrão do sistema). Clicar de
  novo no mesmo item para a prévia; clicar noutro troca. Independente da
  playlist e do broadcast — nada vaza pro ar.
- **Streaming (Icecast/Shoutcast)**: o app transmite o monitor do sink de
  broadcast pro servidor via processo filho ffmpeg (MP3 128 kbps). Um
  botão novo abaixo do TALK (ícone de compartilhamento) liga/desliga o
  stream e fica verde no ar. O ConfigDialog ganhou o grupo "Streaming"
  com URL do servidor, usuário e senha (`stream/url|user|pass`) mais nome
  e descrição do stream (`stream/name|description`, enviados como
  `ice_name`/`ice_description`). O source se identifica como `LaraRadio`
  (`-user_agent`). O botão de stream fica desabilitado até a config ser
  válida e reavalia ao fechar o ConfigDialog.
- **Metadata ao vivo (curr playing)**: a cada troca de música/vinheta (e
  ao ligar o stream) o app atualiza o título do mount no Icecast via
  `/admin/metadata` com "título - artista" das tags TagLib, usando as
  credenciais do source. Se o mount ainda não estiver pronto (ffmpeg
  conectando logo após ligar), a atualização tenta de novo em 2s.
- **Dispositivo de saída configurável** (`audio/output_device`): os
  players principais (os 2 AudioPlayers + hora certa) podem apontar pra um
  sink específico (ex: o sink virtual `broadcast`) via
  `AudioPlayer::setOutputDevice()`. Vazio = padrão do sistema. Casa por id
  ou descrição do dispositivo.

### Alterado

- **i18n**: header do `pt_BR.ts` corrigido (`language="pt_BR"`, era um
  `br_FR` hardcoded); traduções vazias do `en_US.ts` preenchidas (Volumes,
  L, R); as ações do menu de contexto da playlist (Marcar como Próximo,
  Tocar Este, Pré Escuta, Apagar) eram strings literais e agora passam por
  `tr()`; `lrelease` é obrigatório após todo `lupdate` — `.qm` velho faz
  strings novas caírem pro source pt-BR.
- **Infra (insight)**: sink virtual `broadcast` (null sink do PipeWire,
  persistido em `/etc/pipewire/pipewire.conf.d/`) e loopback de
  monitoring de `broadcast.monitor` pro sink padrão do sistema (sem
  dispositivo hardcoded, `/etc/pipewire/pipewire-pulse.conf.d/`). Icecast
  local na porta 9000 (serviço runit) — a 8000 estava ocupada por um
  container docker.

### Corrigido

- **Crash no toggle do stream (2×)**: o handler do `QProcess::finished`
  usava o member `m_streamProc`, que já é nulo no desligamento manual —
  SIGSEGV em `deleteLater()` e depois em `readAllStandardOutput()`. O
  handler agora usa só `sender()`, e o desligamento desvincula o member
  antes do `kill()` (sem `waitForFinished` bloqueante).
- **Botão de stream não ativava após salvar config**: o `QSettings` do
  ConfigDialog só gravava no disco no destrutor (depois da reavaliação do
  botão); o `accept()` agora chama `sync()` na hora.
- **Botão de stream "não ligava"**: `-user_agent` é opção do protocolo
  http de SAÍDA — antes do `-i` o ffmpeg 6.1 morre com "Option user_agent
  not found" antes de streamar. Movido pra depois do `-f mp3`, antes da
  URL.
- **ffmpeg órfão segurando o mount do Icecast**: matar o app com `kill -9`
  deixa o ffmpeg filho vivo segurando "/live" ("Mountpoint in use" ao
  reconectar). O app agora mantém lock de PID
  (`/tmp/lararadio_stream.pid`), mata o órfão no próximo ligar e limpa o
  lock quando o processo termina.

## [1.0.5] — 2026-07-27 — Modificações sobre o original 1.0.4

### Alterado

#### `audioplayer.h` / `audioplayer.cpp`
- **Herança**: `AudioPlayer` mudou de subclasse de `QMediaPlayer` para
  subclasse de `QObject` contendo `QMediaPlayer *player` como membro
  (composição sobre herança). Desacopla o backend de áudio da API
  pública e permite gerenciamento de ciclo de vida mais flexível.
- **Volume**: volume padrão do `QAudioOutput` mudou de `0` para `1.0`
  (QMediaPlayer agora é a fonte de áudio real)
- **Transcodificação MP3**: adicionado `transcodeIfNeeded()` — converte
  MP3s com album art embedado para WAV temporário via `ffmpeg -vn`
  antes da reprodução. Previne o crash do decoder `mp3float` que
  ocorria após várias músicas com album art.
- **Tratamento de erro**: conectado `QMediaPlayer::errorOccurred` →
  `onPlayerError()` que emite o sinal `mediaError()`
- **Detecção de fim de faixa**: conectado
  `QMediaPlayer::mediaStatusChanged` → emite `playbackFinished()`
  em `EndOfMedia`
- **Destrutor**: destrutor explícito para limpar `player` e `audioOutput`
- **Helpers inline**: `getPosition()`, `getDuration()`, `remainingTime()`,
  `isPlaying()`, `isPaused()`, `isStopped()`, `getVolume()`, `setVolume()`
  simplificados para implementações inline de uma linha
- **`isValidMediaFile()`**: método estático usando `ffprobe` para
  validar arquivos de áudio antes de enfileirar
- **`hasError()`**: flag pública de estado de erro

#### `main.cpp`
- **Crash handler**: adicionado `crashHandler()` para `SIGSEGV`,
  `SIGABRT`, `SIGFPE` — imprime mensagem descritiva no stderr e
  sai com código `128 + signal`. Previne crashes silenciosos do
  decoder FFmpeg.
- **Tema escuro**: ativada a palette do tema escuro Fusion (estava
  comentada). Fundo `#303030`, base `#242424`, texto `#dcdcdc`,
  destaque `#55aaff`. Usa GTK3 quando disponível.

#### `mainwindow.h` / `mainwindow.cpp`
- **`skipToNext()`**: slot público — reseta ambos players, avança
  `current_play` e chama `next()`. Usado quando uma faixa dá erro.
- **`checkAdvanceTrack()`**: slot público — avança a playlist quando
  `playbackFinished` é emitido e ambos os players estão parados.
  Ponto único de avanço da playlist.
- **Recuperação de erro**: sinal `mediaError` de ambos `AudioPlayer`
  conectado a `skipToNext()` — auto-avança em erros do decoder
- **Avanço da playlist**: sinal `playbackFinished` de ambos os players
  conectado a `checkAdvanceTrack()`
- **Hora certa**: verifica `QFile::exists()` antes de tocar o arquivo
  de hora. Se não existir, loga warning e pula sem ativar
  `SayingTimer` — previne deadlock da playlist.
- **Timeplayer error handler**: conectado `QMediaPlayer::errorOccurred`
  no timeplayer — limpa `SayingTimer` e avança em caso de erro
- **Removido avanço duplicado do `flash()`**: o timer de 500ms `flash()`
  não avança mais a playlist. O avanço é feito exclusivamente por
  `checkAdvanceTrack()` via `playbackFinished`. Impede condições de
  corrida em faixas curtas (vinhetas) onde ambos os mecanismos
  avançavam independentemente.
- **Watchdog de silêncio / falha de áudio**: monitora o **avanço de
  posição** via `updateDisplay()` (timer de 10ms). Se o `getPosition()`
  do player ativo não avança por 10 segundos (sem fade ativo), chama
  `skipToNext()` automaticamente. Reescrito da versão por VU meter: o
  backend FFmpeg não alimenta o `QAudioBufferOutput`, então o nível do
  VU ficava em 0 com o áudio tocando, causando saltos falsos. Cobre
  falha de dispositivo, bugs silenciosos do decoder e pipes travados.
- **Fim de playlist para em vez de repetir**: `repeat` agora é `false`
  por padrão. Na última faixa com repeat desligado,
  `checkAdvanceTrack()` para a reprodução (`isPlaying = false`) em vez
  de voltar ao início; a pré-transição de 5s em `currentTimePosition()`
  também não faz mais wrap quando o repeat está desligado.
- **Fix de segfault ao editar playlist durante reprodução**:
  `clearPlaylist()` esvaziava a playlist com `isPlaying` ainda true e
  `current_play` desatualizado, fazendo `topLevelItem()` retornar
  nullptr → SIGSEGV. Agora: `clearPlaylist()` reseta os dois players e
  o estado de reprodução; `updateAudioList()` verifica null de
  `curItem`/`nextItem` antes de estilizar;
  `on_btn_remove_item_clicked()` ajusta `current_play`/`next_play`
  após apagar; `next()` limita `current_play` antes de indexar.
- **Botão Stop funciona com playlist vazia**: remover a última faixa
  (em reprodução) deixava a playlist vazia com áudio ainda tocando —
  `on_btn_stop_clicked()` retornava cedo e nunca parava os players.
  Agora só retorna cedo se a playlist estiver vazia **e** nenhum
  player tocando. Também protegido o acesso `playlist[current_play]`
  no `updateAudioList()` para o caso vazio (era UB fora dos limites).

#### `buttonhole.h`
- Adicionado `#include <QMediaPlayer>` e `#include <QAudioOutput>`
  explicitamente (eram puxados indiretamente via `audioplayer.h`
  quando ele herdava de `QMediaPlayer`)

### Adicionado

#### `mainwindow.ui` / `mainwindow.cpp`
- **Checkbox Repeat** (`chk_repeat`, à esquerda dos botões de
  transporte, entre o microfone e o play): liga/desliga o membro
  `repeat` que controla se a playlist faz wrap no fim ou para na
  última faixa. Padrão: desligado.

#### `buttonhole.h` / `buttonhole.cpp`
- **Botão Loop**: um 11º ButtonHole com texto "Loop" (à esquerda do
  label "Botoeira", linha y=574) com a nova flag `setLoopMode()`. Quando
  ativo, o áudio atribuído reinicia do início no `EndOfMedia`
  (`mediaStatusChanged` → `player->play()`), tocando indefinidamente.
  Atribua o áudio via botão direito → "Carregar Áudio", igual aos
  outros buttonholes (settings key `buttonhole/btn_Loop`). Clique
  alterna tocar/parar como os demais.

#### `mainwindow.h` / `mainwindow.cpp`
- **Drag & drop na playlist**: `audio_list` aceita drops externos (event
  filter instalado na tree). Arquivos de áudio soltos do gerenciador de
  arquivos entram como itens `music`; arrastar uma **pasta** expande em
  uma sequência de itens `music` (varredura recursiva, ordenada por nome
  — "uma atrás da outra"). Arquivos não-áudio são ignorados. Vinheta
  continua vindo só da caixa dela.
- **Helper `makePlaylistItem()`**: extraído da lógica de montagem via
  TagLib que estava copiada 3× (botão add + 2 handlers de double-click
  desligados). Agora é o caminho único pra montar um item `Playlist`,
  usado pelo botão add e pelo drop. Bônus: `completeBaseName()`
  substitui a cadeia de `filename.remove(".mp3")` (remove só a extensão
  final).
- **Remoção múltipla**: o `audio_list` mudou pra `ExtendedSelection`; o
  botão de remover apaga **todos os selecionados** (da maior row pra
  menor, mantendo `current_play`/`next_play` limitados).
- **Luz ON AIR — integrada no botão TALK** (refatorado do botão
  `onAirButton` standalone, que foi removido): a luz física de ON AIR
  agora acompanha o estado de locução. Clicar no microfone (TALK) dispara
  o script configurado com o parâmetro correspondente — `onair/script` +
  `onair/param_on` / `onair/param_off` (defaults `on1`/`off1`, ex:
  `usbrelay2 on1`) — ON ao iniciar a locução, OFF ao terminar. Script e
  parâmetros configurados em ConfigDialog → Caminhos. Execução é ignorada
  silenciosamente se o caminho do script não existir.
- **Scroll no ConfigDialog**: o diálogo de configuração agora envolve o
  conteúdo numa `QScrollArea` pra todos os campos (inclusive os do ON
  AIR) ficarem acessíveis em telas pequenas. O exemplo do script mostra
  `~/bin/usbrelay2` — resolvido por usuário, sem nome hardcoded.
- **Prioridade Playlist ↔ Loop com fades**: tocar uma faixa na playlist
  com o Loop rodando para o Loop e deixa a faixa fazer fade in; clicar no
  Loop com a playlist tocando faz fade out da playlist e passa o áudio pro
  Loop. `current_play` continua na mesma faixa nas duas trocas.
- **TALK abaixa as DUAS fontes**: o botão TALK agora abaixa o volume do
  Loop além da playlist (`loopBh->maxVolume` alimentado pela mesma lógica
  de ducking; o flash timer do ButtonHole sobe/desce o volume gradualmente,
  não de sopetão).
- **"Marcar como Próximo" corrigido**: a ação do menu de contexto agora
  move de verdade a faixa clicada pra logo depois da que está tocando
  (reorder no vector + ajuste de `current_play`), então ela toca em
  seguida e o label "Próxima" atualiza. Antes só setava um índice que o
  `updateAudioList(true)` sobrescrevia.
- **Reordenar playlist arrastando (manual)**: o drag & drop interno
  reordena a playlist mantendo o `std::vector` sincronizado (o
  `InternalMove` do Qt reordena só a árvore — armadilha de dessincronização
  conhecida). Implementado como subclass `PlaylistTree` com `QDrag` manual
  carregando a linha de origem no mimeData; o alvo do drop é escolhido pela
  metade do item sob o cursor (metade de cima = insere antes, metade de
  baixo = insere depois) e um indicador de drop mostra onde vai cair. O
  cursor mostra um badge com o nome da faixa + duração enquanto arrasta.
- **Export de log / playback report**: o app mantém um log de reprodução em
  memória (plays de faixa, vinhetas, TALK on/off, acionamentos do Loop,
  timestamps) e o ConfigDialog ganhou um botão "Exportar Log / Playback
  Report" que grava `LaraRadio_playback_report.txt` no home do usuário.
- **i18n limpo**: toda string visível agora passa por `tr()`
  (menu de contexto do ButtonHole, label do botão Loop, diálogos de
  export). A chave de settings do botão Loop foi separada do texto visível
  (`setBtnKey`) para que traduzir o label nunca quebre os mapeamentos de
  áudio salvos. `languages/en_US.ts` agora tem 84 traduções finished;
  `pt_BR` usa as strings do source (pt-BR é a língua do código).
- **Autosave de configurações e playlist**: novo groupBox "Autosave" no
  ConfigDialog com checkbox "Salvar configurações e playlist
  automaticamente". Quando ativado, a playlist atual é gravada em
  `~/LaraRadio_autosave.txt` imediatamente ao clicar OK nas configs, de
  novo ao fechar o app (`closeEvent`), e carregada na abertura no lugar da
  última playlist manual (`showEvent`). Configs já persistem via QSettings.
  O layout do ConfigDialog foi reorganizado (Volumes / Caminhos / Autosave
  / Exportar / aviso) sem sobreposição de widgets.

### Problemas conhecidos (originais, não introduzidos por nós)
- `TagLib::AudioProperties::length()` deprecated — usar
  `lengthInSeconds()` (3 warnings no build)
- Warning do VDPAU em GPUs Radeon (inofensivo, aceleração de vídeo)
- Warning de parsing do tema GTK com certas versões de
  `gtk-contained-dark.css` (inofensivo)
