/* Experimental exclusive-owner SX1261 CAD diagnostics. Revised BSD; see LICENSE.TXT. */
#ifndef LORAGW_SX1261_CAD_H
#define LORAGW_SX1261_CAD_H
#include <stdbool.h>
#include <stdint.h>

struct sx1261_cad_config {
    uint32_t frequency_hz;
    uint32_t bandwidth_hz;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    bool ldro;
    uint8_t symbols;
    uint8_t det_peak;
    uint8_t det_min;
};

struct sx1261_cad_request {
    uint32_t frequency_hz;
    uint32_t bandwidth_hz;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    bool ldro;
    uint8_t symbols;
    bool explicit_sensitivity;
    double sensitivity_dbm;
};
/* Pure default resolution. -3 means explicit dBm conversion not implemented;
 * -1 unsupported/invalid profile. No register writes on either path. */
int sx1261_cad_resolve_default(const struct sx1261_cad_request *request,
                              struct sx1261_cad_config *resolved);
bool sx1261_cad_is_active(void);
/* Caller confirms a completed physical radio reset. */
void sx1261_cad_reset_acknowledge(void);

/* Caller owns the connected auxiliary radio exclusively, with no active LBT or
 * spectral scan. No concentrator/global startup configuration is changed.
 * These calls use polled IRQs; caller supplies monotonic timeout/cancellation. */
int sx1261_cad_configure(const struct sx1261_cad_config *config);
int sx1261_cad_start(void);
int sx1261_cad_poll(bool *done, bool *detected, uint16_t *irq);
int sx1261_cad_cancel(void);
/* Restore the caller's explicit known LBT RX profile, not an inferred snapshot.
 * Caller must restart its LBT scan if needed. Failure requires reset/reinit. */
int sx1261_cad_restore(uint32_t frequency_hz, uint8_t hal_bandwidth);
#endif
