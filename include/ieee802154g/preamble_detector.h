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


#ifndef INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_H
#define INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_H

#include <ieee802154g/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace ieee802154g {

    /*!
     * \brief Preamble detector for 2-(G)FSK
     * \ingroup ieee802154g
     * \details
     * This decimating block is intended to be driven by quadrature demodulator.
     * This block outputs to binary slicer.
     * This block detects 0xaa/0x55 patters on input to establish frequency offset
     * and bit-phase so optimal sample point is used for output to slicer
     */
    class IEEE802154G_API preamble_detector : virtual public gr::sync_decimator
    {
     public:
      typedef boost::shared_ptr<preamble_detector> sptr;

      /*!
       * \brief create new instance of 2-(G)FSK preamble detector
       * \param samples_per_symbol bit_rate = samp_rate / samples_per_symbol
       */
      static sptr make(int samples_per_symbol);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_H */

