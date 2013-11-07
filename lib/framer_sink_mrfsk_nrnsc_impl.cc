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
#include "framer_sink_mrfsk_nrnsc_impl.h"
#include <stdio.h>

namespace gr {
  namespace ieee802154g {

    framer_sink_mrfsk_nrnsc::sptr
    framer_sink_mrfsk_nrnsc::make(msg_queue::sptr target_queue)
    {
      return gnuradio::get_initial_sptr
        (new framer_sink_mrfsk_nrnsc_impl(target_queue));
    }

    /*
     * The private constructor
     */
    framer_sink_mrfsk_nrnsc_impl::framer_sink_mrfsk_nrnsc_impl(msg_queue::sptr target_queue)
      : gr::sync_block("framer_sink_mrfsk_nrnsc",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(0, 0, 0)),
              d_target_queue(target_queue)
    {
        d_state = STATE_SYNC_SEARCH;
    }

    /*
     * Our virtual destructor.
     */
    framer_sink_mrfsk_nrnsc_impl::~framer_sink_mrfsk_nrnsc_impl()
    {
    }

    int
    framer_sink_mrfsk_nrnsc_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const unsigned char *in = (const unsigned char *) input_items[0];
        int count = 0;

        while (count < noutput_items) {
            switch (d_state) {
                case STATE_SYNC_SEARCH: // get SFD:
                    while (count < noutput_items) {
                        dbg_hist <<= 1;
                        if (in[count] & 1)
                            dbg_hist |= 1;
                        else
                            dbg_hist &= ~1;
                        if (in[count] & 0x2) {  // correlator flag set?
                            int n;
                            d_state = STATE_HAVE_SYNC;
                            rf_buf = in[count++] & 1;
                            rf_buf_bitlen_cnt = 1;
                            for (n = NUM_WEIGHTS-1; n >= 0; n--)
                                weights[n] = 0;
                            phr_psdu_buf_idx = 0;
                            db_bp = 0x80;
                            phr_psdu_buf_idx_stop = -1;
                            break;
                        }
                        count++;
                    }
                    break;
                case STATE_HAVE_SYNC:   //
                    while (count < noutput_items) {
                        rf_buf = (rf_buf << 1) | (in[count++] & 1);
                        if (++rf_buf_bitlen_cnt == 32) {
                            rf_buf_bitlen_cnt = 0;
                            int s;
                            interleave_u32(&rf_buf);
                            if (phr_psdu_buf_idx == 0) { // first time:
                                ppui = rf_buf >> 30;
                                pui = (rf_buf >> 28) & 3;
                                s = 26;
                                decode_ui((rf_buf >> s) & 3);
                                shift_weights();
                                s -= 2;
                                for (; s >= 0; s -= 2) {
                                    decode_ui((rf_buf >> s) & 3);
                                    push_bit();
                                    shift_weights();
                                }
                            } else {
                                for (s = 30; s >= 0; s -= 2) {
                                    decode_ui((rf_buf >> s) & 3);
                                    push_bit();
                                    shift_weights();
                                    if (phr_psdu_buf_idx_stop == -1) {
                                        if (phr_psdu_buf_idx == PHR_LENGTH) {
                                            phr.word = phr_psdu_buf[0] << 8;
                                            phr.word |= phr_psdu_buf[1];
                                            printf("%02x%02x PHR:%04x phr.bits.frame_length:%d\n", phr_psdu_buf[0], phr_psdu_buf[1], phr.word, phr.bits.frame_length);
                                            if (phr.bits.FCS) {
                                                crc16 = INITIAL_CRC16;
                                            } else {
                                                crc_32 = INITIAL_CRC32;
                                            }
                                            if (phr.bits.DW)
                                                lfsr = 0x1ff;
                                            phr_psdu_buf_idx_stop = phr.bits.frame_length + PHR_LENGTH;
                                        }
                                    } else if (phr_psdu_buf_idx >= phr_psdu_buf_idx_stop) {
                                        int i;
                                        char crc_ok = 0;
                                        if (phr.bits.FCS) {
                                            if (crc16 == 0)
                                                crc_ok = 1;
                                            else
                                                printf("crc16:%04x\n", crc16);
                                        } else {
                                            int zs = phr_psdu_buf_idx - PHR_LENGTH - 4;
                                            uint8_t z = 0;
                                            uint32_t rx_crc;
                                            // run crc32 over zeros if less that 4 bytes usable payload
                                            while (zs < 4) {
                                                crc_32 = digital_update_crc32(crc_32, &z, 1);
                                                zs++;
                                            }
                                            crc_32 = ~crc_32;
                                            rx_crc = phr_psdu_buf[phr_psdu_buf_idx-4] << 24;
                                            rx_crc += phr_psdu_buf[phr_psdu_buf_idx-3] << 16;
                                            rx_crc += phr_psdu_buf[phr_psdu_buf_idx-2] << 8;
                                            rx_crc += phr_psdu_buf[phr_psdu_buf_idx-1];
                                            if (rx_crc == crc_32)
                                                crc_ok = 1;
                                            else
                                                printf("crc_32 fail: %08x vs %08x\n", crc_32, rx_crc);
                                        }
                                        message::sptr msg = message::make(1, phr.word, crc_ok, phr_psdu_buf_idx-PHR_LENGTH);
                                        memcpy(msg->msg(), phr_psdu_buf+PHR_LENGTH, phr_psdu_buf_idx-PHR_LENGTH);
                                        d_target_queue->insert_tail(msg);   // send it
                                        msg.reset();    // free it up
                                        d_state = STATE_SYNC_SEARCH;
                                        break;
                                    }
                                }
                                if (d_state != STATE_HAVE_SYNC)
                                    break;
                            }
                        } // ..if have one interleaved section
                    } // ...while (count < noutput_items)
                    break;
            } // ...switch (d_state)
        } // ...while (count < noutput_items)

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

    void
    framer_sink_mrfsk_nrnsc_impl::decode_ui(uint8_t ui)
    {
        bool w3;

        ui &= 3;
        if (ui & 2) {
            if ( (pui >> 1) ^ (pui & 1) )
                w3 = !(ppui >> 1) ^ (ppui & 1);
            else
                w3 = (ppui >> 1) ^ (ppui & 1);
        } else {
            if ( (pui >> 1) ^ (pui & 1) )
                w3 = (ppui >> 1) ^ (ppui & 1);
            else
                w3 = !(ppui >> 1) ^ (ppui & 1);
        }
        if (w3)
            weights[3]++;
        else
            weights[3]--;

        if ( (ui >> 1) ^ (ui & 1) )
            weights[2]++;
        else
            weights[2]--;

        if ( (pui >> 1) ^ (pui & 1) )
            weights[1]++;
        else
            weights[1]--;

        if ( (ppui >> 1) ^ (ppui & 1) )
            weights[0]++;
        else
            weights[0]--;

        ppui = pui;
        pui = ui;
        
    }

    void
    framer_sink_mrfsk_nrnsc_impl::shift_weights()
    {
        weights[0] = weights[1];
        weights[1] = weights[2];
        weights[2] = weights[3];
        weights[3] = 0;
    }

    void
    framer_sink_mrfsk_nrnsc_impl::push_bit(void)
    {
        if (weights[0] > 0) {
            phr_psdu_buf[phr_psdu_buf_idx] |= db_bp;
            //printf("push_bit 1 @%02x %d\n", db_bp, phr_psdu_buf_idx);
        } else {
            phr_psdu_buf[phr_psdu_buf_idx] &= ~db_bp;
            //printf("push_bit 0 @%02x %d\n", db_bp, phr_psdu_buf_idx);
        }

        db_bp >>= 1;
        if (db_bp == 0x00) {
            db_bp = 0x80;
            if (phr_psdu_buf_idx_stop != -1) {
                if (phr.bits.DW)
                    phr_psdu_buf[phr_psdu_buf_idx] ^= get_pn9_byte(&lfsr);

                if (phr.bits.FCS)
                    crc16 = crc_msb_first(crc16, phr_psdu_buf+phr_psdu_buf_idx, 1);
                else if (phr_psdu_buf_idx < (phr_psdu_buf_idx_stop-4))
                    crc_32 = digital_update_crc32(crc_32, phr_psdu_buf+phr_psdu_buf_idx, 1);
            }
            if (++phr_psdu_buf_idx >= (int)sizeof(phr_psdu_buf)) {
                printf("[41mpush_bit phr_psdu_buf_idx[0m\n");
            }
        }
    }

  } /* namespace ieee802154g */
} /* namespace gr */

