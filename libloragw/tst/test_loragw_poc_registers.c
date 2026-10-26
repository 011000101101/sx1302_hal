/* Revised BSD; see LICENSE.TXT. Recording register contract, no radio required. */
#include "loragw_hal.h"
#include "loragw_sx1302.h"
#include "loragw_reg.h"
#include <assert.h>
#include <stdio.h>
static int32_t values[2048];
static unsigned writes;
int lgw_reg_w(uint16_t id, int32_t value) { assert(id < 2048); values[id] = value; ++writes; return 0; }
int main(void) {
    uint8_t first,second;
    for (unsigned word = 0; word < 256; ++word)
        assert(sx1302_syncword_symbols(word,0,&first,&second) == 0);
    assert(sx1302_syncword_symbols(0,0,&first,&second) == 0 && first == 0 && second == 0);
    assert(sx1302_syncword_symbols(0x34,0,&first,&second) == 0 && first == 6 && second == 8);
    assert(sx1302_syncword_symbols(0x2b,0x11,&first,&second) == 0 && first == 5 && second == 23);

    assert(sx1302_lora_syncword(true,7,0,true) == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FRAME_SYNCH0_PEAK1_POS] == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FRAME_SYNCH1_PEAK2_POS] == 0);
    assert(values[SX1302_REG_RX_TOP_FRAME_SYNCH0_SF7TO12_PEAK1_POS_SF7TO12] == 6);
    assert(sx1302_lora_syncword(true,7,0,false) == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FRAME_SYNCH0_PEAK1_POS] == 6);
    assert(sx1302_lora_syncword(true,7,0x2b,true) == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FRAME_SYNCH0_PEAK1_POS] == 4);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FRAME_SYNCH1_PEAK2_POS] == 22);
    struct lgw_conf_rxif_s fsk = {0};
    fsk.sync_word_size = 3; fsk.sync_word = 0xC194C1; fsk.datarate = 50000;
    assert(sx1302_fsk_configure(&fsk) == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_PKT_MODE] == 1);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_CRC_EN] == 1);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_DCFREE_ENC] == 2);
    fsk.fsk_options_set = fsk.fsk_fixed_length = fsk.fsk_no_crc = fsk.fsk_crc_ibm = fsk.fsk_no_whitening = true;
    fsk.fsk_payload_length = 17; fsk.fsk_channel_bw_exponent = 2;
    fsk.sync_word_size = 8; fsk.sync_word = 0;
    assert(sx1302_fsk_configure(&fsk) == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_PKT_MODE] == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_CRC_EN] == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_CRC_IBM] == 1);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_0_DCFREE_ENC] == 0);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_CFG_1_CH_BW_EXPO] == 2);
    assert(values[SX1302_REG_RX_TOP_LORA_SERVICE_FSK_FSK_PKT_LENGTH_PKT_LENGTH] == 17);
    unsigned before = writes; fsk.sync_word_size = 0;
    assert(sx1302_fsk_configure(&fsk) != 0 && writes == before);
    puts("POC register contracts passed");
}
