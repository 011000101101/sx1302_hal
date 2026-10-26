/* Revised BSD; see LICENSE.TXT. Stopped-configuration and timing contracts. */
#include "loragw_runtime.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct lgw_rx_update update = {0};
    struct lgw_reconfigure_result result;
    update.rf_mask = 1;
    update.rf[0] = (struct lgw_conf_rxrf_s){.enable=true,.freq_hz=868100000,.type=LGW_RADIO_TYPE_SX1250};
    update.if_mask = (1U << 8) | (1U << 9);
    update.channels[8] = (struct lgw_conf_rxif_s){.enable=true,.bandwidth=BW_250KHZ,.datarate=7,.sync_word_set=true,.sync_word=0};
    update.channels[9] = (struct lgw_conf_rxif_s){.enable=true,.bandwidth=BW_125KHZ,.datarate=50000,.sync_word_set=true,.sync_word_size=4,.sync_word=0,
        .fsk_options_set=true,.fsk_channel_bw_exponent=3,.fsk_fixed_length=true,.fsk_payload_length=17};
    const uint64_t initial = lgw_configuration_generation();
    assert(lgw_reconfigure_rx(&update,false,&result) == 0 && result.outcome == LGW_RECONFIG_APPLIED);
    assert(result.generation > initial && !result.restarted && !result.timestamps_invalidated);
    struct lgw_rx_update snapshot;
    assert(lgw_get_rx_configuration(&snapshot) == 0);
    assert(snapshot.channels[8].sync_word_set && snapshot.channels[8].sync_word == 0);
    assert(snapshot.channels[9].sync_word_set && snapshot.channels[9].sync_word_size == 4 && snapshot.channels[9].fsk_fixed_length);
    const uint64_t valid = result.generation;
    update.channels[8].enable = false; /* Would commit first without full validation rollback. */
    update.channels[9].sync_word_size = 9;
    assert(lgw_reconfigure_rx(&update,false,&result) != 0 && result.outcome == LGW_RECONFIG_INVALID);
    assert(lgw_configuration_generation() == valid);
    assert(lgw_get_rx_configuration(&snapshot) == 0 && snapshot.channels[8].enable);
    assert(snapshot.channels[9].sync_word_size == 4);
    update.channels[9].sync_word_size = 1;
    update.channels[9].sync_word = 0x100; /* Must not silently truncate sync bytes. */
    assert(lgw_reconfigure_rx(&update,false,&result) != 0 && result.outcome == LGW_RECONFIG_INVALID);
    assert(lgw_configuration_generation() == valid);
    struct lgw_pkt_tx_s p = {.modulation=MOD_FSK,.datarate=1000,.size=5,.preamble=5,.fsk_sync_word_set=true,.fsk_sync_word_size=3};
    assert(lgw_time_on_air(&p) == 129);
    p.fsk_options_set = p.fsk_fixed_length = true;
    assert(lgw_time_on_air(&p) == 121);
    p.fsk_sync_word_size = 8;
    assert(lgw_time_on_air(&p) == 161);
    p.datarate = 0;
    assert(lgw_time_on_air(&p) == 0);
    p = (struct lgw_pkt_tx_s){.modulation=MOD_LORA,.bandwidth=BW_125KHZ,.datarate=12,.coderate=1,.preamble=8,.size=50,.ldro_mode_manual=true,.ldro_on=true};
    const unsigned on = lgw_time_on_air(&p); p.ldro_on = false;
    assert(lgw_time_on_air(&p) < on);
    p.bandwidth = BW_500KHZ; const unsigned off = lgw_time_on_air(&p); p.ldro_mode_manual = false;
    assert(lgw_time_on_air(&p) == off);
    struct sx1261_cad_request request = {868100000,250000,11,1,false,4,false,0};
    struct sx1261_cad_config config;
    assert(sx1261_cad_resolve_default(&request,&config) == 0);
    assert(config.symbols == 4 && config.det_peak == 24 && config.det_min == 10);
    request.explicit_sensitivity = true; request.sensitivity_dbm = -120;
    assert(sx1261_cad_resolve_default(&request,&config) == LGW_HAL_NOT_IMPLEMENTED);
    assert(!lgw_runtime_faulted());
    puts("POC configuration and timing contracts passed");
}
