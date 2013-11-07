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

#ifndef INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_NRNSC_IMPL_H
#define INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_NRNSC_IMPL_H

#include <ieee802154g/framer_sink_mrfsk_nrnsc.h>
#include "utils_mrfsk.h"

#define NUM_WEIGHTS     4

namespace gr {
  namespace ieee802154g {

    class framer_sink_mrfsk_nrnsc_impl : public framer_sink_mrfsk_nrnsc
    {
     private:
        enum state_t { STATE_SYNC_SEARCH, STATE_HAVE_SYNC, STATE_HAVE_HEADER };
        state_t     d_state;
        msg_queue::sptr     d_target_queue;     // where to send received packet
        uint32_t rf_buf;
        char rf_buf_bitlen_cnt;
        uint32_t dbg_hist;
        uint8_t ppui, pui;
        void decode_ui(uint8_t);
        int weights[NUM_WEIGHTS];
        void shift_weights(void);
        void push_bit(void);
        uint8_t phr_psdu_buf[2050];
        uint16_t phr_psdu_buf_idx;
        int phr_psdu_buf_idx_stop;
        uint8_t db_bp;
        MRFSK_PHR_t phr;
        uint16_t crc16;
        uint32_t crc_32;
        uint16_t lfsr;

     public:
      framer_sink_mrfsk_nrnsc_impl(msg_queue::sptr target_queue);
      ~framer_sink_mrfsk_nrnsc_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_NRNSC_IMPL_H */

