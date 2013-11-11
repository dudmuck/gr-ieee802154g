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


#ifndef INCLUDED_IEEE802154G_MRFSK_SOURCE_H
#define INCLUDED_IEEE802154G_MRFSK_SOURCE_H

#include <ieee802154g/api.h>
#include <gnuradio/sync_block.h>

#define PAYLOAD_TYPE_INCR_BYTE  0   // wi-sun interop
#define PAYLOAD_TYPE_PN9        1   // TUV conformance
#define PAYLOAD_TYPE_CRC_TEST   2
#define PAYLOAD_PN9_FOREVER     3   // not packet, LFSR only

namespace gr {
  namespace ieee802154g {

    /*!
     * \brief MR-FSK packet generator
     * \ingroup ieee802154g
     *
     * \details
     * This block generates MR-FSK packets compliant to IEEE-802.15.4g-2012.
     */
    class IEEE802154G_API mrfsk_source : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<mrfsk_source> sptr;

      /*!  \brief create instance of MR-FSK packet generator
       *
       * \param num_iterations How many packets to sent
       * \param preamble_size Length of preamble in octets
       * \param fec_en enables FEC NRNSC encoding
       * \param dw enables PN9 whitening of PSDU
       * \param crc_type_16 selects CRC type
       * \param payload_type content of PSDU payload, see PAYLOAD_* definitions
       * \param psdu_len length of PSDU in octets (including MFR/CRC)
       * \param delay_bytes slows rate of packets: length of time between packtes in octets, during which TX power is off
       */
      static sptr make(
        int num_iterations,
        int preamble_size,
        bool fec_en,
        bool dw,
        bool crc_type_16,
        char payload_type,
        int psdu_len,
        int delay_bytes
      );
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_MRFSK_SOURCE_H */

