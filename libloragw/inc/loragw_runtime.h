/* Revised BSD; see LICENSE.TXT. Exclusive-owner POC runtime reconfiguration. */
#ifndef LORAGW_RUNTIME_H
#define LORAGW_RUNTIME_H
#include "loragw_hal.h"
#include "loragw_sx1261_cad.h"
struct lgw_rx_update {
    uint8_t rf_mask;
    struct lgw_conf_rxrf_s rf[LGW_RF_CHAIN_NB];
    uint16_t if_mask;
    struct lgw_conf_rxif_s channels[LGW_IF_CHAIN_NB];
    bool demod_set;
    struct lgw_conf_demod_s demod;
};
enum lgw_reconfigure_outcome {
    LGW_RECONFIG_APPLIED, LGW_RECONFIG_INVALID, LGW_RECONFIG_RESTART_REQUIRED,
    LGW_RECONFIG_BUSY, LGW_RECONFIG_FAULT
};
struct lgw_reconfigure_result {
    enum lgw_reconfigure_outcome outcome;
    uint64_t generation;
    bool restarted;
    bool timestamps_invalidated;
};
/* Pure validation/cache update when stopped. A live change needs explicit restart
 * authorization, no scheduled/emitting TX, no active CAD/LBT/spectral scan, and EXCLUSIVE serialized
 * ownership by the caller. It interrupts ALL concentrator RX. The caller must
 * quiesce leases and invalidate its reservations/timestamp mapping on restart.
 * Failed startup restores cached old config and latches fault; no automatic retry
 * on partially initialized hardware. Reset the board before clearing that fault. */
int lgw_reconfigure_rx(const struct lgw_rx_update *update, bool allow_restart,
                       struct lgw_reconfigure_result *result);
int lgw_get_rx_configuration(struct lgw_rx_update *configuration);
uint64_t lgw_configuration_generation(void);
bool lgw_runtime_faulted(void);
int lgw_runtime_reset_acknowledge(void);
/* Guarded entry point for the CAD-only auxiliary role. Main RX is uninterrupted.
 * Existing raw diagnostic sx1261_cad_* calls still require exclusive owner care. */
int lgw_aux_cad_start(void);
#endif
