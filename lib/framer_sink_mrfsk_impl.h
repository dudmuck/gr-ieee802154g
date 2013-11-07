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

#ifndef INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_IMPL_H
#define INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_IMPL_H

#include <ieee802154g/framer_sink_mrfsk.h>
#include "utils_mrfsk.h"

namespace gr {
  namespace ieee802154g {

    class framer_sink_mrfsk_impl : public framer_sink_mrfsk
    {
     private:
        enum state_t { STATE_SYNC_SEARCH, STATE_HAVE_SYNC, STATE_HAVE_HEADER };
        static const int HEADERBITLEN   = 16;   // PHR size in bits
        state_t     d_state;
        msg_queue::sptr     d_target_queue;     // where to send received packet
        MRFSK_PHR_t phr;
        int d_headerbitlen_cnt;
        unsigned char d_packet_byte_index, d_packet_byte;
        uint16_t d_packetlen_cnt;
        uint8_t d_packet[aMaxPHYPacketSize];    // PSDU buffer
        uint32_t crc_32;
        uint16_t crc16_;
        uint32_t dbg_hist;
        uint16_t lfsr;

     public:
      framer_sink_mrfsk_impl(msg_queue::sptr target_queue);
      ~framer_sink_mrfsk_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_IMPL_H */

