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


#ifndef INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_H
#define INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_H

#include <ieee802154g/api.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/msg_queue.h>

namespace gr {
  namespace ieee802154g {

    /*!
     * \brief MR-FSK uncoded framer
     * \ingroup ieee802154g
     *
     * \details
     *  msg_queue.type() is always 0 for uncoded
     *  msg_queue.arg1() contains PHR (PHY header)
     *  msg_queue.arg2() is 1 for good CRC, or 0 for CRC calculation mismatch
     */
    class IEEE802154G_API framer_sink_mrfsk : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<framer_sink_mrfsk> sptr;

      /*!
       * \brief create instance of MR-FSK uncoded framer
       * \param target_queue the message queue for parsed packets
       */
      static sptr make(msg_queue::sptr target_queue);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_FRAMER_SINK_MRFSK_H */

