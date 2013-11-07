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


#ifndef INCLUDED_IEEE802154G_PA_RAMP_H
#define INCLUDED_IEEE802154G_PA_RAMP_H

#include <ieee802154g/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace ieee802154g {

    /*!
     * \brief <+description of block+>
     * \ingroup ieee802154g
     *
     */
    class IEEE802154G_API pa_ramp : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<pa_ramp> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ieee802154g::pa_ramp.
       *
       * To avoid accidental use of raw pointers, ieee802154g::pa_ramp's
       * constructor is in a private implementation
       * class. ieee802154g::pa_ramp::make is the public interface for
       * creating new instances.
       */
      static sptr make(float rm, int steps);

      /*!
       * \brief Set the multipication factor during packet transmit
       */
      //virtual float k(float rm) const = 0;

      /*!
       * \brief Set the multipication factor during packet transmit
       */
      virtual void set_k(float rm) = 0;
      virtual void set_steps(int s) = 0;
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_PA_RAMP_H */

