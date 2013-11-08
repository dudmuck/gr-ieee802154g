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

#ifndef INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_IMPL_H
#define INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_IMPL_H

#include <ieee802154g/preamble_detector.h>

#define NUM_MIDS        4

typedef struct {
    float min_val;
    int   min_val_at;
    float max_val;
    int   max_val_at;
} side_t;

namespace gr {
  namespace ieee802154g {

    class preamble_detector_impl : public preamble_detector
    {
     private:
        enum state_e { STATE_NONE, STATE_HAVE_PREAMBLE };
        state_e state;

        int sps;
        float sps_half;
        int sps_x2;

        int s_tol;
        int preamble_cnt;

        int zcu_sum_cnt;
        int zcd_sum_cnt;

        float zcu_sum;
        float zcd_sum;

        float sample_point_a;
        float sample_point_b;
        int int_sample_point_a;
        int int_sample_point_b;

        int prev_zcu_at;
        int prev_zcd_at;

        int work_2ui(bool first, const float *in);

        float *ui_buf;
        bool ui_buf_new;

        int dbg_num_zeros;

        bool zcu_forced;
        bool zcd_forced;

        float mids[NUM_MIDS];
        int mid_idx;
        float mid_avg;
        float get_mid(float a, float b);
        float f_offset; // AFC

     public:
      preamble_detector_impl(int samples_per_symbol);
      ~preamble_detector_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace ieee802154g
} // namespace gr

#endif /* INCLUDED_IEEE802154G_PREAMBLE_DETECTOR_IMPL_H */

