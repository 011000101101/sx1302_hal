/* Experimental SX1261 CAD command path. Revised BSD; see LICENSE.TXT. */
#include <stddef.h>
#include "loragw_sx1261.h"
#include "loragw_sx1261_cad.h"

/* Source: Semtech sx126x_driver, a10c5dfdf89788c6ac805e9fe98889de44175aa2,
 * https://github.com/Lora-net/sx126x_driver/blob/a10c5dfdf89788c6ac805e9fe98889de44175aa2/src/sx126x.c
 * SetCadParams uses symbol-count encoding, detector peak/min, CAD_ONLY exit and
 * a zero hardware RX timeout; CAD_DONE=0x0080 and CAD_DETECTED=0x0100.
 * The gateway transport includes the status dummy byte in read buffers. */
#define SET_CAD ((sx1261_op_code_t)0xC5)
#define SET_CAD_PARAMS ((sx1261_op_code_t)0x88)
#define CAD_IRQ 0x0180

int sx1261_cad_configure(const struct sx1261_cad_config *c) {
    uint8_t b[8];
    uint8_t bw, symbols;
    uint32_t frequency;
    if (c == NULL || c->frequency_hz < 150000000U || c->frequency_hz > 960000000U ||
        c->spreading_factor < 5 || c->spreading_factor > 12 || c->coding_rate < 1 || c->coding_rate > 4) return -1;
    switch (c->bandwidth_hz) {
        case 125000: bw = 4; break;
        case 250000: bw = 5; break;
        case 500000: bw = 6; break;
        default: return -1;
    }
    switch (c->symbols) {
        case 1: symbols = 0; break;
        case 2: symbols = 1; break;
        case 4: symbols = 2; break;
        case 8: symbols = 3; break;
        case 16: symbols = 4; break;
        default: return -1;
    }
    b[0] = SX1261_STDBY_RC;
    if (sx1261_reg_w(SX1261_SET_STANDBY, b, 1) != 0) return -1;
    b[0] = SX1261_PACKET_TYPE_LORA;
    if (sx1261_reg_w(SX1261_SET_PACKET_TYPE, b, 1) != 0) return -1;
    frequency = SX1261_FREQ_TO_REG(c->frequency_hz);
    b[0] = frequency >> 24; b[1] = frequency >> 16; b[2] = frequency >> 8; b[3] = frequency;
    if (sx1261_reg_w(SX1261_SET_RF_FREQUENCY, b, 4) != 0) return -1;
    b[0] = c->spreading_factor; b[1] = bw; b[2] = c->coding_rate; b[3] = c->ldro ? 1 : 0;
    if (sx1261_reg_w(SX1261_SET_MODULATION_PARAMS, b, 4) != 0) return -1;
    b[0] = 0; b[1] = 8; b[2] = 0; b[3] = 255; b[4] = 1; b[5] = 0;
    if (sx1261_reg_w(SX1261_SET_PACKET_PARAMS, b, 6) != 0) return -1;
    b[0] = 1; b[1] = 0x80; b[2] = b[3] = b[4] = b[5] = b[6] = b[7] = 0;
    if (sx1261_reg_w(SX1261_SET_DIO_IRQ_PARAMS, b, 8) != 0) return -1;
    b[0] = symbols; b[1] = c->det_peak; b[2] = c->det_min;
    b[3] = b[4] = b[5] = b[6] = 0;
    if (sx1261_reg_w(SET_CAD_PARAMS, b, 7) != 0) return -1;
    return sx1261_cad_cancel();
}

int sx1261_cad_start(void) {
    uint8_t b[2] = {0xFF, 0xFF};
    if (sx1261_reg_w(SX1261_CLR_IRQ_STATUS, b, 2) != 0) return -1;
    b[0] = 0;
    return sx1261_reg_w(SET_CAD, b, 0);
}

int sx1261_cad_poll(bool *done, bool *detected, uint16_t *irq) {
    uint8_t b[3] = {0};
    if (done == NULL || detected == NULL || irq == NULL) return -1;
    *done = false; *detected = false; *irq = 0;
    if (sx1261_reg_r(SX1261_GET_IRQ_STATUS, b, 3) != 0) return -1;
    /* Command timeout / processing error / execution failure are not CAD idle. */
    if (((b[0] >> 1) & 7) >= 3 && ((b[0] >> 1) & 7) <= 5) return -1;
    *irq = ((uint16_t)b[1] << 8) | b[2];
    *done = (*irq & 0x0080) != 0;
    *detected = *done && ((*irq & 0x0100) != 0);
    return 0;
}

int sx1261_cad_cancel(void) {
    uint8_t b[2] = {SX1261_STDBY_RC, 0};
    int result = sx1261_reg_w(SX1261_SET_STANDBY, b, 1);
    b[0] = 0xFF; b[1] = 0xFF;
    if (sx1261_reg_w(SX1261_CLR_IRQ_STATUS, b, 2) != 0) result = -1;
    return result;
}

int sx1261_cad_restore(uint32_t frequency_hz, uint8_t bandwidth) {
    uint8_t irq_masks[8] = {0};
    if (sx1261_cad_cancel() != 0 || sx1261_setup() != 0) return -1;
    if (sx1261_reg_w(SX1261_SET_DIO_IRQ_PARAMS, irq_masks, 8) != 0) return -1;
    return sx1261_set_rx_params(frequency_hz, bandwidth);
}
