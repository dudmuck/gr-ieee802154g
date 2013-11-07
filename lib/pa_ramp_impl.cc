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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "pa_ramp_impl.h"
#include <stdio.h>

namespace gr {
  namespace ieee802154g {

    pa_ramp::sptr
    pa_ramp::make(float rm, int steps)
    {
      return gnuradio::get_initial_sptr
        (new pa_ramp_impl(rm, steps));
    }

    /*
     * The private constructor
     */
    pa_ramp_impl::pa_ramp_impl(float rm, int steps)
      : gr::sync_block("pa_ramp",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
        int i;
        real_max = rm;
        d_k = gr_complex(0.0, 0.0); // start off
        dir_up = false;
        dir_down = false;

        if (steps == 0)
            steps = 1;
        ramp_steps = steps;
        table = (float *)malloc(sizeof(float) * (steps+1));
        make_ramp_table(steps);
        table_idx = 0;
        new_gain = false;
    }

    /*
     * Our virtual destructor.
     */
    pa_ramp_impl::~pa_ramp_impl()
    {
        free(table);
    }

    int
    pa_ramp_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        int i;
        uint64_t abs_N;
        uint64_t nr = nitems_read(0);
        uint64_t start, length;
        int on;
        std::vector<tag_t> tags;
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

        this->get_tags_in_range(tags, 0, nr, nr + noutput_items);

        if (tags.size() == 0) {
            for (i = 0; i < noutput_items; i++) {
                *out++ = *in++ * d_k;
                ramp();
            }
            return noutput_items;
        }

        for (unsigned int i = 0; i < tags.size(); ) {
            if (pmt::symbol_to_string(tags[i].key) == "pa_ramp") {
                int n, stop;
                start = tags[i].offset - nr;
                if (i == 0 && start > 0) {
                    for (n = 0; n < start; n++) {
                        out[n] = in[n] * d_k;
                        ramp();
                    }
                }
                on = pmt::to_long(tags[i++].value);
                if (i < tags.size()) {
                    length = (tags[i].offset - nr) - start;
                } else {
                    length = noutput_items - start;
                }
                if (on) {
                    table_idx = 1;
                    d_k = gr_complex(table[table_idx], 0.0);
                    if (table_idx < ramp_steps)
                        dir_up = true;
                } else {
                    table_idx = ramp_steps - 1;
                    d_k = gr_complex(table[table_idx], 0.0);
                    if (table_idx > 0)
                        dir_down = true;
                }
                stop = start + length;
                for (n = start; n < stop; n++) {
                    out[n] = in[n] * d_k;
                    ramp();
                }
            }   // ...if tag key is "pa_ramp"
        } // ...for all tags

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

    void
    pa_ramp_impl::ramp()
    {
        static bool tx_on = false;

        if (dir_up) {
            if (++table_idx >= ramp_steps) {
                tx_on = true;
                dir_up = false;   // done ramping
            }
            d_k = gr_complex(table[table_idx], 0.0);
        } else if (dir_down) {
            if (--table_idx <= 0)
                dir_down = false;   // done ramping
            d_k = gr_complex(table[table_idx], 0.0);
            tx_on = false;
        } else if (new_gain && tx_on) {
            // support continuous TX ON
            new_gain = false;
            d_k = gr_complex(real_max, 0.0);
        }
    }

    void
    pa_ramp_impl::make_ramp_table(int num_steps)
    {
        int i;
        float s, f;

        table = (float *)malloc(sizeof(float) * (num_steps+1));

        s = real_max / num_steps;

        f = 0;
        for (i = 0; i < num_steps; i++) {
            table[i] = f;
            f += s;
        }
        table[i] = f;
    }

  } /* namespace ieee802154g */
} /* namespace gr */

