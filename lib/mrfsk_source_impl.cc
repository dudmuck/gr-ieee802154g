/* -*- c++ -*- */
/* 
 * Copyright 2013 wroberts92780@gmail.com
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "mrfsk_source_impl.h"
#include <stdio.h>


namespace gr {
  namespace ieee802154g {

    mrfsk_source::sptr
    mrfsk_source::make(
        int num_iterations,
        int preamble_size,
        bool fec_en,
        bool dw,
        bool crc_type_16,
        char payload_type,
        int psdu_len,
        int delay_bytes
    ) {
      return gnuradio::get_initial_sptr
        (new mrfsk_source_impl(num_iterations, preamble_size, fec_en, dw, crc_type_16, payload_type, psdu_len, delay_bytes));
    }

    /*
     * The private constructor
     */
    mrfsk_source_impl::mrfsk_source_impl(
        int num_iterations,
        int preamble_size,
        bool fec_en,
        bool dw,
        bool crc_type_16,
        char payload_type,
        int psdu_len,
        int delay_bytes
    ) : gr::sync_block("mrfsk_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(unsigned char)))
    {
        preamble_bytes = preamble_size;
        nrnsc = fec_en;
        dw_en = dw;
        fcs_type = crc_type_16;
        delay_total = delay_bytes;
        pkt_countdown = num_iterations;

        payload_content_type = payload_type;
        if (payload_content_type == PAYLOAD_TYPE_CRC_TEST) {
            if (crc_type_16)
                psdu_size = 5;
            else
                psdu_size = 7;
        } else {
            psdu_size = psdu_len;
            // make sure length is at least enough to hold crc
            if (crc_type_16) {
                if (psdu_size < 2)
                    psdu_size = 2;
            } else {
                if (psdu_size < 4)
                    psdu_size = 4;
            }
        }

        state = STATE_INIT_DELAY;
        delay_countdown = 50;   // radio sink device startup time?
    }

    /*
     * Our virtual destructor.
     */
    mrfsk_source_impl::~mrfsk_source_impl()
    {
    }

    int
    mrfsk_source_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        int i, sent = 0;
        unsigned char *out = (unsigned char *) output_items[0];

        for (i = 0; i < noutput_items; i++) {
            switch (state) {
                case STATE_INIT_DELAY:
                    if (payload_content_type == PAYLOAD_PN9_FOREVER) {
                        lfsr = 0x1ff;
                        out[i] = get_pn9_byte(&lfsr);
                        state = STATE_PN9_LFSR;
                        pa_enable(true, sent);
                    } else {
                        /* perhaps the TX sink needs a few samples to start up? */
                        out[i] = 0xff;
                        if (--delay_countdown == 0)
                            state = STATE_GENERATE_PACKET;
                    }
                    break;
                case STATE_GENERATE_PACKET:
                    generate_packet();
                    rf_buf_sent = 0;
                    out[i] = rf_buf[rf_buf_sent++];
                    pa_enable(true, sent);
                    state = STATE_SEND_PACKET;
                    break;
                case STATE_SEND_PACKET:
                    out[i] = rf_buf[rf_buf_sent++];
                    if (rf_buf_sent >= rf_buf_len)
                        state = STATE_PAD;
                    break;
                case STATE_PAD:
                    /* prevent corruption of last bit */
                    out[i] = 0x00;
                    state = STATE_DELAY_START;
                    break;
                case STATE_DELAY_START:
                    out[i] = 0x00;
                    pa_enable(false, sent);
                    delay_countdown = delay_total;
                    state = STATE_DELAY;
                    state = STATE_DELAY;
                    break;
                case STATE_DELAY:
                    out[i] = 0x00;
                    if (--delay_countdown <= 0) {
                        if (--pkt_countdown == 0) {
                            state = STATE_DONE;
                            return sent;
                        } else
                            state = STATE_GENERATE_PACKET;
                    }
                    break;
                case STATE_DONE:
                    return -1;
                case STATE_PN9_LFSR:
                    out[i] = get_pn9_byte(&lfsr);
                    break;
            } // ..switch (state)
            sent++;
        } // ...for (i = 0; i < noutput_items; i++)

        // Tell runtime system how many output items we produced.
        return sent;
    }

    void
    mrfsk_source_impl::pa_enable(bool en, int sent)
    {
        const uint64_t offset = this->nitems_written(0) + sent;
        pmt::pmt_t key = pmt::string_to_symbol("pa_ramp");
        pmt::pmt_t value = pmt::from_long( en ? 1 : 0 );
        this->add_item_tag(0, offset, key, value);
    }

    /* encode_bit: Section 18.1.2.4, NRNSC Figure 124 of 802.15.4g-2012 */
    int
    mrfsk_source_impl::encode_bit(uint8_t bit)
    {
        char ui1, ui0;

        M2 = M1;
        M1 = M0;
        M0 = bi;
        bi = bit ? 1 : 0;

        ui1 = (bi + M1 + M2) % 2;
        ui0 = (bi + M0 + M1 + M2) % 2;

        if (ui1) {
            rf_buf[rf_buf_len] &= ~rf_bp;
        } else {
            rf_buf[rf_buf_len] |= rf_bp;
        }

        rf_bp >>= 1;
        if (rf_bp == 0) {
            rf_bp = 0x80;
            if (++rf_buf_len >= (int)sizeof(rf_buf))
                return -1;
        }

        if (ui0) {
            rf_buf[rf_buf_len] &= ~rf_bp;
        } else {
            rf_buf[rf_buf_len] |= rf_bp;
        }

        rf_bp >>= 1;
        if (rf_bp == 0) {
            rf_bp = 0x80;
            if (++rf_buf_len >= (int)sizeof(rf_buf))
                return -1;
        }

        return 0;
    }

    void
    mrfsk_source_impl::generate_packet()
    {
        int i;
        uint16_t phr_psdu_buf_idx;
        uint8_t phr_psdu_buf[2050];
        uint8_t *psdu_buf = &phr_psdu_buf[2];
        int psdu_buf_idx;
        uint8_t payload_incr_octet;
        uint16_t psdu_size_wo_crc;

        /* generate preamble */
        for (rf_buf_len = 0; rf_buf_len < preamble_bytes; )
            rf_buf[rf_buf_len++] = 0x55;

        if (nrnsc) {
            rf_buf[rf_buf_len++] = 0x6f;
        } else
            rf_buf[rf_buf_len++] = 0x90;

        rf_buf[rf_buf_len++] = 0x4e;

        phr.word = 0;
        phr.bits.DW = dw_en;
        phr.bits.FCS = fcs_type;
        phr.bits.frame_length = psdu_size;

        phr_psdu_buf[0] = phr.word >> 8;
        phr_psdu_buf[1] = phr.word & 0xff;

        if (phr.bits.FCS)
            psdu_size_wo_crc = psdu_size - 2;
        else
            psdu_size_wo_crc = psdu_size - 4;

        psdu_buf_idx = 0;
        switch (payload_content_type) {
            case PAYLOAD_TYPE_CRC_TEST: // 0x400056
                psdu_buf[psdu_buf_idx++] = 0x40;
                psdu_buf[psdu_buf_idx++] = 0x00;
                psdu_buf[psdu_buf_idx++] = 0x56;
                break;
            case PAYLOAD_TYPE_PN9:      // TUV conformance
                lfsr = 0x1ff;
                for (i = 0; i < psdu_size_wo_crc; i++)
                    psdu_buf[psdu_buf_idx++] = get_pn9_byte(&lfsr);
                break;
            case PAYLOAD_TYPE_INCR_BYTE:   // wi-sun interop
                payload_incr_octet = 0;
                for (i = 0; i < psdu_size_wo_crc; i++)
                    psdu_buf[psdu_buf_idx++] = reverse_octet(payload_incr_octet++);
                /* reverse: IEEE seems prefer LSbit first */
                break;
            /* different payloads could be added here */
        } // ..switch (payload_content_type)


        if (phr.bits.FCS) {
            crc16 = INITIAL_CRC16;
            crc16 = crc_msb_first(crc16, psdu_buf, psdu_buf_idx);
            psdu_buf[psdu_buf_idx++] = crc16 >> 8;
            psdu_buf[psdu_buf_idx++] = crc16 & 0xff;
        } else {
            uint8_t z = 0;
            crc_32 = INITIAL_CRC32;
            crc_32 = digital_update_crc32(crc_32, psdu_buf, psdu_buf_idx);
            i = psdu_buf_idx;
            while (i < 4) {
                crc_32 = digital_update_crc32(crc_32, &z, 1);
                i++;
            }
            crc_32 = ~crc_32;
            psdu_buf[psdu_buf_idx++] = crc_32 >> 24;
            psdu_buf[psdu_buf_idx++] = crc_32 >> 16;
            psdu_buf[psdu_buf_idx++] = crc_32 >> 8;
            psdu_buf[psdu_buf_idx++] = crc_32 & 0xff;
        }

        if (phr.bits.DW) {
            lfsr = 0x1ff;
            for (i = 0; i < psdu_buf_idx; i++)
                psdu_buf[i] ^= get_pn9_byte(&lfsr);
        }

        phr_psdu_buf_idx = psdu_buf_idx + PHR_LENGTH;

        if (nrnsc) {
            uint16_t rf_buf_len_start = rf_buf_len;
            M2 = 0;
            M1 = 0;
            M0 = 0;
            bi = 0;
            rf_bp = 0x80;
            uint8_t rf_bit_cnt = 0;
            for (i = 0; i < phr_psdu_buf_idx; i++) {
                uint8_t bp;
                for (bp = 0x80; bp != 0; bp >>= 1) {
                    if (encode_bit(phr_psdu_buf[i] & bp)) {
                        printf("fec abort\n");
                        return;
                    }
                    rf_bit_cnt += 2;
                    if (rf_bit_cnt == 32) {
                        rf_bit_cnt = 0;
                        interleave(&rf_buf[rf_buf_len-4]);
                    }
                }
            }

            if (encode_bit(0)) {    // tail
                fprintf(stderr, "encode_bit fail\n");
                return;
            }
            if (encode_bit(0)) {    // tail
                fprintf(stderr, "encode_bit fail\n");
                return;
            }
            if (encode_bit(0)) {    // tail
                fprintf(stderr, "encode_bit fail\n");
                return;
            }
            if (phr_psdu_buf_idx & 1) {
                if (encode_bit(0)) {    // 5bit pad : 0
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 5bit pad : 1
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 5bit pad : 2
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 5bit pad : 3
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 5bit pad : 4
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }

            } else {
                if (encode_bit(0)) {    // 13bit pad : 0
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 1
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 2
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 3
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 4
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 5
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 6
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 7
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 8
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 9
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(0)) {    // 13bit pad : 10
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 11
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
                if (encode_bit(1)) {    // 13bit pad : 12
                    fprintf(stderr, "encode_bit fail\n  ");
                    return;
                }
            }
            interleave(&rf_buf[rf_buf_len-4]);
        } else {
            /* PHR and PSDU is sent as-is over the air */
            memcpy(rf_buf+rf_buf_len, phr_psdu_buf, phr_psdu_buf_idx);
            rf_buf_len += phr_psdu_buf_idx;
        }
    } // ..generate_packet()

  } /* namespace ieee802154g */
} /* namespace gr */

