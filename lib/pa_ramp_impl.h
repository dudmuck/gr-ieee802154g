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

#ifndef INCLUDED_IEEE802154G_PA_RAMP_IMPL_H
#define INCLUDED_IEEE802154G_PA_RAMP_IMPL_H

#include <ieee802154g/pa_ramp.h>

namespace gr {
  namespace ieee802154g {

    class pa_ramp_impl : public pa_ramp
    {
     private:
        gr_complex d_k;
        float real_max;
        bool dir_up, dir_down, new_gain;
        void ramp(void);
        int ramp_steps;
        void make_ramp_table(int);
        float *table;
        int table_idx;

     public:
      pa_ramp_impl(float rm, int steps);
      ~pa_ramp_impl();

      void set_k(float rm) {
        real_max = rm;
        free(table);
        make_ramp_table(ramp_steps);
        new_gain = true;
      }

      void set_steps(int s) {
          if (s > 0) {
            ramp_steps = s;
            free(table);
            make_ramp_table(ramp_steps);
          }
      }

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_PA_RAMP_IMPL_H */

