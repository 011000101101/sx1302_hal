# Lorawantastic P2 HAL extensions

Fork API `LORAWANTASTIC_HAL_API_VERSION = 0x00020000`; rebuild all applications.
The stock source API defaults remain available, but structs gained fields and
are not ABI-compatible with old binaries. This fork is for a serialized,
exclusive owner of the concentrator and auxiliary radio.

## Startup and packet configuration

`lgw_conf_rxif_s::sync_word_set` separates a supplied value from a resource
configured default. Service LoRa accepts every 8-bit selector, including 0x00.
The old nonzero `sync_word` convention remains supported for existing callers.
TX uses `lgw_pkt_tx_s::synch_word_set` for the same distinction. No supplied zero
is replaced, rejected or clamped. `sx1302_syncword_symbols` resolves selectors
and the two fine bits consistently for service RX and per-packet TX.

Service RX `lora_options_set` enables IQ inversion, explicit LDRO selection and
preamble count. Existing implicit-header length/CRC/coding-rate settings remain
available. Defaults preserve the former service profile. Multisf RX remains the
SX1302's global SF mask and public/private sync selection; independent arbitrary
sync selectors per multisf channel are not supported.

FSK RX `fsk_options_set` enables fixed/variable length, length, CRC presence and
CCITT/IBM selection, whitening, and the raw `fsk_channel_bw_exponent` register.
3 is the existing 125-kHz reference. Other raw exponents are programmable but
are NOT advertised as characterized RF bandwidths; verify their actual response
before a high-level provider exposes such a profile. RX sync words are right
aligned, size 1..8, with explicit zero supported via `sync_word_set`.

FSK packet TX independently selects framing, whitening, CRC and optional sync
value/size without modifying RX/default context. Fixed framing omits the length
byte in the TX FIFO. Bitrate/deviation, preamble, carrier, timing and power use
the existing packet fields. Airtime follows fixed framing, selected sync length,
CRC, and actual preamble defaults. LoRa airtime now honors explicit LDRO and
uses the same BW/SF-dependent automatic setting as TX.

Forwarder startup JSON adds `chan_Lora_std.invert_pol`, `.ldro`, `.preamble`, and
presence-aware `.synch_word`. FSK `.options: true` enables `.fixed_length`,
`.payload_length`, `.no_crc`, `.crc_ibm`, `.no_whitening` and `.bandwidth_exponent`.
`.sync_word` is a hexadecimal string with `.sync_word_size`; strings avoid loss
of precision for 64-bit values. The existing downlink `sync` field now preserves
zero. Other extended FSK TX fields are available through the C API.

## Runtime reconfiguration

`loragw_runtime.h` exposes a cached snapshot and masked RF/IF/demod update.
Validation happens before stopping hardware and restores the complete cached
snapshot if any requested setting is invalid. Enabled IFs must reference an
enabled RF chain. Stopped updates commit without hardware I/O.

Live updates require explicit restart authorization and caller-owned quiescence
of ALL affected clients. Scheduled/emitting TX and auxiliary CAD/LBT/spectral scan reject the
update. There is no unsupported live register retune disguised as a safe change:
all concentrator RX is interrupted by a restart. The result supplies generation,
restart and timestamp-invalidation information; the provider must invalidate
reservations and counter-to-clock mappings. Configuration APIs are not thread-safe.

A failed stop/start enters a fault state. Failed startup restores the old cached
profile, closes partially opened resources and does not retry over unknown
hardware. The owner resets the board physically before calling
`lgw_runtime_reset_acknowledge` and reinitializing. Software acknowledgement is
not proof that a physical reset happened. No automatic recovery is claimed.

## Auxiliary CAD

`sx1261_cad_resolve_default` is pure. It resolves the initial four-actual-symbol
SF7..12, BW125/250/500-kHz default profiles from pinned Meshtastic/RadioLib
settings. Explicit dBm sensitivity returns NOT_IMPLEMENTED before writes. These
are reference defaults, not measured absolute calibration. Existing raw diagnostic
controls remain available to an expert exclusive owner.

`lgw_aux_cad_start` checks half-duplex concentrator TX state, including scheduled
TX, before scanning. Conversely `lgw_send` rejects while CAD is active. Main RX
continues. LBT/spectral scan and CAD exclude each other; failed transitions keep
ownership until explicit stop/reset. Caller restores its known auxiliary profile
and restarts LBT as required before subsequent transmission. CAD-only is exposed;
no auxiliary packet RX or automatic CAD-to-RX implementation is claimed.

## Verification

Build an isolated source copy, then run `libloragw/tst/run_poc_host_contracts.sh`.
These record register writes and exercise cached transactions/default resolution
and airtime without hardware. They do not prove RF performance or successful
live restart/rollback. Existing full HAL/forwarder builds remain the baseline.
Actual Pi CAD, RF profile, LBT coexistence and restart/failure recovery tests are
required and are supplied by the POC project's manual test instructions.
