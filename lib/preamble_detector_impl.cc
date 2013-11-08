/* -*- c++ -*- */
/* 
 * Copyright 2013 wroberts
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
 * 
 *************************************************************************************
 * This preamble detector is intended only for preamble detection of (G)FSK
 * packets, where the preamble is a repeating 0xAA or 0x55 pattern at start of packet.
 * It is intended to take the place of clock_recovery_mm_ff for this specific use.
 * The input of this block should be driven by quad demod, and output to binary slicer.
 * Samples per symbol is best in the range 3 to 20.
 *
 * You might find it desirable to use a simple squelch before the quad demod to reduce
 * unwanted activity from background noise.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
//#define P_DEBUG 1

#include <gnuradio/io_signature.h>
#include "preamble_detector_impl.h"
//#ifdef P_DEBUG
#include <stdio.h>
//#endif /* P_DEBUG */


namespace gr {
  namespace ieee802154g {

    preamble_detector::sptr
    preamble_detector::make(int samples_per_symbol)
    {
      return gnuradio::get_initial_sptr
        (new preamble_detector_impl(samples_per_symbol));
    }

    /*
     * The private constructor
     */
    preamble_detector_impl::preamble_detector_impl(int samples_per_symbol)
      : gr::sync_decimator("preamble_detector",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)), samples_per_symbol),
                sps(samples_per_symbol)
    {
        state = STATE_NONE;
        mid_idx = 0;
        s_tol = sps / 4;
        if (s_tol == 0)
            s_tol = 1;
        s_tol++;
#ifdef P_DEBUG
        printf("s_tol:%d\n", s_tol);
#endif /* P_DEBUG */
        preamble_cnt = 0;

        zcu_sum_cnt = 0;
        zcd_sum_cnt = 0;

        sps_half = sps / 2.0;
        sps_x2 = sps * 2;

        ui_buf = (float *) malloc(sizeof(float)*sps_x2);
        ui_buf_new = false;

        int_sample_point_a = sps;
        int_sample_point_b = sps;

        zcu_forced = false;
        zcd_forced = false;

        f_offset = 0;
    }

    /*
     * Our virtual destructor.
     */
    preamble_detector_impl::~preamble_detector_impl()
    {
        free(ui_buf);
    }

    int
    preamble_detector_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const float *in = (const float *) input_items[0];
        float *out = (float *) output_items[0];

        int j = 0, i = 0;
        bool first = true;
        int idx;

        if (ui_buf_new) {
            // fill 2nd half of ui_buf
            for (idx = 0; idx < sps; idx++) {
                ui_buf[idx+sps] = in[idx];
            }

            if (work_2ui(first, ui_buf))
                return -1;
            first = false;
            out[i] = ui_buf[j+int_sample_point_b];
            out[i++] -= f_offset;
            j += sps;

            ui_buf_new = false;
        }

        for (; i < noutput_items; ) {
            if (i < (noutput_items-1)) {
                if (work_2ui(first, &in[j]))
                    return -1;
                out[i] = in[j+int_sample_point_a];
                out[i++] -= f_offset;
                out[i] = in[j+int_sample_point_b];
                out[i++] -= f_offset;
                first = false;
                j += sps_x2;
            } else {
                out[i++] = in[j+int_sample_point_a];
                // fill first half of ui_buf
                for (idx = 0; idx < sps; idx++) {
                    ui_buf[idx] = in[j++];
                }
                ui_buf_new = true;

#ifdef P_DEBUG
                if (dbg_num_zeros < sps_x2)
                    printf(" odd-exit ");
#endif /* P_DEBUG */
            }
        } // ..for (int i = 0; i < noutput_items; i++)

#ifdef P_DEBUG
        if (dbg_num_zeros < sps_x2)
            printf(" exit work %d:%d\n", noutput_items, i);
#endif /* P_DEBUG */

        // Tell runtime system how many output items we produced.
        return noutput_items;
    } // ..work()

    //preamble_detector_impl::work_2ui(bool _first, int *_dbg_num_zeros, const float *in_)
    int
    preamble_detector_impl::work_2ui(bool _first, const float *in_)
    {
            int n;
            bool flipped = false;
            float min_val = 1000, max_val = -1000;
            int min_val_at = -1, max_val_at = -1;
            int zcu_at = -1, zcd_at = -1;
            dbg_num_zeros = 0;
            for (n = 0; n < sps_x2; n++) {
                if (in_[n] == 0)
                    dbg_num_zeros++;

                if (in_[n] < min_val) {
                    min_val = in_[n];
                    min_val_at  = n;
                }
                if (in_[n] > max_val) {
                    max_val = in_[n];
                    max_val_at  = n;
                }
                if (!_first || n > 0) {
                    if (in_[n-1] < mid_avg && in_[n] >= mid_avg)
                        zcu_at = n;
                    else if (in_[n-1] >= mid_avg && in_[n] < mid_avg)
                        zcd_at = n;
                }
            }

            if (dbg_num_zeros == sps_x2) {
                return 0;   // nothing to do (squelched)
            }

            /*if (_first)*/ {
                int diff_zcu = abs(zcu_at - prev_zcu_at);
                int diff_zcd = abs(zcd_at - prev_zcd_at);
#ifdef P_DEBUG
                printf("DIFF:%d,%d ", diff_zcu, diff_zcd); 
#endif /* P_DEBUG */
                if (abs(diff_zcu-sps) < s_tol && abs(diff_zcd-sps) < s_tol) {
                    int itmp;
                    float ftmp;
                    // this occurs when work() returns in middle of preamble
                    flipped = true;
#ifdef P_DEBUG
                    printf(" [43mFLIP");
                    printf("zcu@%d(%d) zcd@%d(%d)[0m ", zcu_at, prev_zcu_at, zcd_at, prev_zcd_at);
#endif /* P_DEBUG */
                    itmp = zcu_at;
                    zcu_at = zcd_at;
                    zcd_at = itmp;

                    itmp = prev_zcu_at;
                    prev_zcu_at = prev_zcd_at;
                    prev_zcd_at = itmp;

                    ftmp = zcu_sum;
                    zcu_sum = zcd_sum;
                    zcd_sum = ftmp;

                    itmp = zcu_sum_cnt;
                    zcu_sum_cnt = zcd_sum_cnt;
                    zcd_sum_cnt = itmp;
                }
            } // if first iteration

            mids[mid_idx++] = get_mid(min_val, max_val);
            if (mid_idx == NUM_MIDS)
                mid_idx = 0;
            int mi = mid_idx;
            float sum = 0;
            for (int c = 0; c < NUM_MIDS; c++) {
                sum += mids[mi++];
                if (mi == NUM_MIDS)
                    mi = 0;
            }
            mid_avg = sum / NUM_MIDS;

            int peaks_tol = abs( abs(min_val_at-max_val_at) - sps );
#ifdef P_DEBUG
            printf("pt:%d ", peaks_tol);
            if (peaks_tol < s_tol)
                printf("*");
            else
                printf(" ");
#endif /* P_DEBUG */

            if (zcd_at == 0 && prev_zcd_at >= (sps_x2-1)) {
#ifdef P_DEBUG
                printf(" [35mdFORCE-HI[0m ");
#endif /* P_DEBUG */
                zcd_at = sps_x2;
                prev_zcd_at = sps_x2-1;
                zcd_forced = true;
            } else {
                if (zcd_forced && zcd_at < 2 && zcd_at != -1) {
#ifdef P_DEBUG
                    printf("reset-zcd ");
#endif /* P_DEBUG */
                    zcd_sum = zcd_at;
                    zcd_sum_cnt = 1;
                }
                zcd_forced = false;
            }

            if (zcu_at == 0 && prev_zcu_at >= (sps_x2-1)) {
#ifdef P_DEBUG
                printf(" [35muFORCE-HI[0m ");
#endif /* P_DEBUG */
                zcu_at = sps_x2;
                prev_zcu_at = sps_x2-1;
                zcu_forced = true;
            } else {
                if (zcu_forced && zcu_at < 2 && zcu_at != -1) {
#ifdef P_DEBUG
                    printf("reset-zcu ");
#endif /* P_DEBUG */
                    zcu_sum = zcd_at;
                    zcu_sum_cnt = 1;
                }
                zcu_forced = false;
            }

            int zc_tol = sps;
            if (zcu_at != -1 && zcd_at != -1) {
                zc_tol = abs( abs(zcu_at-zcd_at) - sps );
#ifdef P_DEBUG
                if (zc_tol < s_tol)
                    printf("#");
                else
                    printf(" ");
#endif /* P_DEBUG */
            } else {
#ifdef P_DEBUG
                printf(" ");
#endif /* P_DEBUG */
                if (preamble_cnt > 0)
                    preamble_cnt--;
            }

            if (peaks_tol < s_tol && zc_tol < s_tol) {
                if (zcu_at == -1 || zcd_at == -1) {
                    /* prevent false preamble detection inside packet */
                    if (preamble_cnt > 0)
                        preamble_cnt--;
                } else
                    preamble_cnt++;

                if (preamble_cnt == 3) {
                    if (zcu_at == -1) {
                        zcu_sum = 0;
                        zcu_sum_cnt = 0;
                    } else {
                        zcu_sum = zcu_at;
                        zcu_sum_cnt = 1;
                    }
                    if (zcd_at == -1) {
                        zcd_sum = 0;
                        zcd_sum_cnt = 0;
                    } else {
                        zcd_sum = zcd_at;
                        zcd_sum_cnt = 1;
                    }
                }
            } else if (peaks_tol >= s_tol && zc_tol >= s_tol)
                preamble_cnt = 0;

            if (preamble_cnt > 3 && zcu_at != -1 && zcd_at != -1) {
                //if ( (prev_zcu_at < 1 && zcu_at >= (sps_x2-1)) || (prev_zcu_at > (sps_x2-1) && zcu_at < 1) )
                if (abs(prev_zcu_at - zcu_at) >= (sps_x2-1)) {
                    /* zero crossing is straddling edge */
                    zcu_sum_cnt = 1;
                    zcu_sum = zcu_at;
                } else {
                    zcu_sum_cnt++;
                    zcu_sum += zcu_at;
                }

                //if ( (prev_zcd_at < 1 && zcd_at >= (sps_x2-1)) || (prev_zcd_at > (sps_x2-1) && zcd_at < 1) )
                if (abs(prev_zcd_at - zcd_at) >= (sps_x2-1)) {
                    /* zero crossing is straddling edge */
                    zcd_sum_cnt = 1;
                    zcd_sum = zcd_at;
                } else {
                    zcd_sum_cnt++;
                    zcd_sum += zcd_at;
                }
            }

            if (preamble_cnt > 6) {
                float zcu_at_f = zcu_sum / zcu_sum_cnt;
                float zcd_at_f = zcd_sum / zcd_sum_cnt;
                float prev_spa = sample_point_a;
                float prev_spb = sample_point_b;
                if (zcu_at_f > zcd_at_f) {
                    // zcd_at is first in time
                    if (zcd_at_f < sps_half) {
                        sample_point_a = zcd_at_f + sps_half;
                        sample_point_b = zcu_at_f + sps_half;
#ifdef P_DEBUG
                        printf("SPa:%.2f,%.2f ", sample_point_a, sample_point_b);
#endif /* P_DEBUG */
                    } else {
                        sample_point_a = zcd_at_f - sps_half;
                        sample_point_b = zcu_at_f - sps_half;
#ifdef P_DEBUG
                        printf("SPc:%.2f,%.2f ", sample_point_a, sample_point_b);
#endif /* P_DEBUG */
                    }
                } else {
                    // zcu_at is first in time
                    if (zcu_at_f < sps_half) {
                        sample_point_a = zcu_at_f + sps_half;
                        sample_point_b = zcd_at_f + sps_half;
#ifdef P_DEBUG
                        printf("SPb:%.2f,%.2f ", sample_point_a, sample_point_b);
#endif /* P_DEBUG */
                    } else {
                        sample_point_a = zcu_at_f - sps_half;
                        sample_point_b = zcd_at_f - sps_half;
#ifdef P_DEBUG
                        printf("SPd:%.2f,%.2f ", sample_point_a, sample_point_b);
#endif /* P_DEBUG */
                    }
                }

#ifdef P_DEBUG
                float sdiff = fabs(sample_point_a - sample_point_b);
                if (sdiff < (sps_half-1)) {
                    printf("\n[41msdiff:%.3f (a%.3f b%.3f) zcu_at_f:%.3f zcd_at_f:%.3f\n", sdiff, sample_point_a, sample_point_b, zcu_at_f, zcd_at_f);
                    printf("zcu_sum:%.3f zcu_sum_cnt:%d\n", zcu_sum, zcu_sum_cnt);
                    printf("zcd_sum:%.3f zcd_sum_cnt:%d\n ", zcd_sum, zcd_sum_cnt);
                    printf("[0m\n");
                    return -1;
                }
                if (prev_spa != sample_point_a) {
                    printf("[36mnew spa:%.3f->%.3f[0m ", prev_spa, sample_point_a);
                }
                if (prev_spb != sample_point_b) {
                    printf("[36mnew spb:%.3f->%.3f[0m ", prev_spb, sample_point_b);
                }
#endif /* P_DEBUG */

                int_sample_point_a = round(sample_point_a);
                if (int_sample_point_a == sps_x2) {
#ifdef P_DEBUG
                    printf("[41mspa-wrap[0m ");
#endif /* P_DEBUG */
                    int_sample_point_a = 0;
                    int_sample_point_b = sps;
                }
                int_sample_point_b = round(sample_point_b);
                if (int_sample_point_b == sps_x2) {
#ifdef P_DEBUG
                    printf("[41mspb-wrap[0m ");
#endif /* P_DEBUG */
                    int_sample_point_a = 0;
                    int_sample_point_b = sps;
                }
                f_offset = mid_avg / 2;
            }
#ifdef P_DEBUG
            printf("min=% .3f@%2d max=% .3f@%2d | % .3f | zcu@%d(%d) zcd@%d(%d) ",
                min_val, min_val_at, max_val, max_val_at,
                mid_avg,
                zcu_at, prev_zcu_at, zcd_at, prev_zcd_at
            );
            if (preamble_cnt > 6)
                printf("pc:[32m%02d[0m ", preamble_cnt);
            else
                printf("pc:%02d ", preamble_cnt);

            if (preamble_cnt > 3) {
                printf("[zcavgs:%.2f %.2f]   ",
                    zcu_sum / zcu_sum_cnt,
                    zcd_sum / zcd_sum_cnt
                );
            }
            printf("[33m%d %d[0m ", int_sample_point_a, int_sample_point_b);
#endif /* P_DEBUG */

            if (!flipped) {
                if (zcu_at != -1)
                    prev_zcu_at = zcu_at;
                if (zcd_at != -1)
                    prev_zcd_at = zcd_at;
            }

#ifdef P_DEBUG
            printf("\n");
#endif /* P_DEBUG */
        return 0;
    }

    float
    preamble_detector_impl::get_mid(float a, float b)
    {
        if (a < 0 && b < 0)
            return (a + b) / 2;
        else if (a > 0 && b > 0)
            return (a + b) / 2;
        else
            return a + b;

#ifdef P_DEBUG
        //printf(" mid=% 01.3f ", mid);
#endif /* P_DEBUG */
    }


  } /* namespace ieee802154g */
} /* namespace gr */

