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

#ifndef INCLUDED_IEEE802154G_MRFSK_SOURCE_IMPL_H
#define INCLUDED_IEEE802154G_MRFSK_SOURCE_IMPL_H

#include <ieee802154g/mrfsk_source.h>
#include "utils_mrfsk.h"

namespace gr {
  namespace ieee802154g {

    class mrfsk_source_impl : public mrfsk_source
    {
     private:
        typedef enum {
            STATE_INIT_DELAY,
            STATE_GENERATE_PACKET,
            STATE_SEND_PACKET,
            STATE_PAD,
            STATE_DELAY_START,
            STATE_DELAY,
            STATE_DONE,
            STATE_PN9_LFSR  // RF test
        } state_e;
        state_e state;
        int preamble_bytes;
        bool nrnsc, dw_en, fcs_type;
        int psdu_size;
        int delay_countdown, delay_total;
        int pkt_countdown;
        MRFSK_PHR_t phr;
        int payload_content_type;
        uint16_t lfsr;  // PN9
        uint16_t crc16;
        uint32_t crc_32;
        char M2, M1, M0, bi;    // FEC
        void generate_packet(void);
        uint8_t rf_buf[32+4096+4];   // over-the-air RF buffer
        uint16_t rf_buf_len;
        uint16_t rf_buf_sent;
        int encode_bit(uint8_t);
        uint8_t rf_bp;
        void pa_enable(bool en, int sent);

     public:
      mrfsk_source_impl(
        int num_iterations,
        int preamble_size,
        bool fec_en,
        bool dw,
        bool crc_type_16,
        char payload_type,
        int psdu_len,
        int delay_bytes
      );
      ~mrfsk_source_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_MRFSK_SOURCE_IMPL_H */

