/* -*- c++ -*- */
/* 
 * Copyright 2013 Wayne Roberts wroberts92780@gmail.com
 *
 * based off of gr-digital/lib/framer_sink_1
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
#include <gnuradio/digital/crc32.h>
#include "framer_sink_mrfsk_impl.h"
#include <stdio.h>

namespace gr {
  namespace ieee802154g {

    framer_sink_mrfsk::sptr
    framer_sink_mrfsk::make(msg_queue::sptr target_queue)
    {
      return gnuradio::get_initial_sptr
        (new framer_sink_mrfsk_impl(target_queue));
    }

    /*
     * The private constructor
     */
    framer_sink_mrfsk_impl::framer_sink_mrfsk_impl(msg_queue::sptr target_queue)
      : gr::sync_block("framer_sink_mrfsk",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(0, 0, 0)),
              d_target_queue(target_queue)
    {
        d_state = STATE_SYNC_SEARCH;
    }

    /*
     * Our virtual destructor.
     */
    framer_sink_mrfsk_impl::~framer_sink_mrfsk_impl()
    {
    }

    int
    framer_sink_mrfsk_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const unsigned char *in = (const unsigned char *) input_items[0];
        int count = 0;

        /* correlator output: LSbit (bit0) is data bit, original data delayed by 64 bits.
         * Bit 1 is flag bit, meaning data bit is first data bit following access code. */
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
                            d_state = STATE_HAVE_SYNC;
                            phr.word = in[count] & 1;
                            d_headerbitlen_cnt = 1;
                            count++;
                            break; // get out of inner while loop
                        }
                        count++;
                    }
                    break;
                case STATE_HAVE_SYNC:   // get PHR:
                    while (count < noutput_items) {
                        phr.word = (phr.word << 1) | (in[count++] & 1);
                        if (++d_headerbitlen_cnt == HEADERBITLEN) {
                            //printf(" phr.word:%04x ", phr.word);
                            //printf("frame_length:%d dw:%d ", phr.bits.frame_length, phr.bits.DW);
                            if (phr.bits.FCS) {
                                //printf("16bit-CRC\n");
                                crc16_ = INITIAL_CRC16;
                            } else {
                                //printf("32bit-CRC\n");
                                crc_32 = 0xffffffff;
                            }
                            if (phr.bits.DW)
                                lfsr = 0x1ff;
                            d_state = STATE_HAVE_HEADER;
                            d_packet_byte_index = 0;
                            d_packet_byte = 0;
                            d_packetlen_cnt = 0;
                            break;
                        }
                    } // ...while (count < noutput_items)
                    break;
                case STATE_HAVE_HEADER:
                    while (count < noutput_items) {
                        d_packet_byte = (d_packet_byte << 1) | (in[count++] & 1);
                        if (d_packet_byte_index++ == 7) {
                            if (phr.bits.DW)
                                d_packet_byte ^= get_pn9_byte(&lfsr);

                            if (phr.bits.FCS)
                                crc16_ = crc_msb_first(crc16_, &d_packet_byte, 1);
                            else if (d_packetlen_cnt < (phr.bits.frame_length - 4) )
                                crc_32 = digital_update_crc32(crc_32, &d_packet_byte, 1);

                            d_packet[d_packetlen_cnt++] = d_packet_byte;
                            d_packet_byte_index = 0;
                            if (d_packetlen_cnt >= phr.bits.frame_length) {
                                char crc_ok = 0;
                                if (phr.bits.FCS) {
                                    if (crc16_ == 0)
                                        crc_ok = 1;
                                    else
                                        printf(" crc16_:%04x\n", crc16_);
                                } else {
                                    int n = d_packetlen_cnt;
                                    uint8_t z = 0;
                                    uint32_t rx_crc;
                                    while (n < 8) {
                                        crc_32 = digital_update_crc32(crc_32, &z, 1);
                                        n++;
                                    }
                                    crc_32 = ~crc_32;
                                    rx_crc = d_packet[d_packetlen_cnt-1];
                                    rx_crc |= d_packet[d_packetlen_cnt-2] << 8;
                                    rx_crc |= d_packet[d_packetlen_cnt-3] << 16;
                                    rx_crc |= d_packet[d_packetlen_cnt-4] << 24;
                                    if (rx_crc == crc_32)
                                        crc_ok = 1;
                                    else
                                        printf("crc_32 fail: %08x vs %08x\n", crc_32, rx_crc);
                                }
                                message::sptr msg = message::make(0, phr.word, crc_ok, d_packetlen_cnt);
                                memcpy(msg->msg(), d_packet, d_packetlen_cnt);
                                d_target_queue->insert_tail(msg);   // send it
                                msg.reset();    // free it up
                                d_state = STATE_SYNC_SEARCH;
                                break;
                            } else if (d_packetlen_cnt >= sizeof(d_packet)) {
                                printf("d_packet overrun FAIL\n");
                                d_state = STATE_SYNC_SEARCH;
                                break;
                            }
                        }
                    } // ...while (count < noutput_items)
                    break;
            } // ...switch (d_state)
        } // ...while (count < noutput_items)

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace ieee802154g */
} /* namespace gr */

