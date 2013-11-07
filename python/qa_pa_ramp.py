#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2013 wroberts92780@gmail.com
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gr_unittest, blocks
import ieee802154g_swig as ieee802154g
import pmt
import numpy

class tag_source(gr.sync_block):
    def __init__(self, rd):
        gr.sync_block.__init__(
            self,
            name = "tag source",
            in_sig = None,
            out_sig = [numpy.complex64],
        )
        self.rd_at = rd

    def work(self, input_items, output_items):
        num_output_items = len(output_items[0])

        output_items[0][:] = 1.0+0j

        count = 0
        key = pmt.string_to_symbol("pa_ramp")
        value = pmt.from_long(1)    # send enable
        self.add_item_tag(0, count, key, value)

        count = self.rd_at
        value = pmt.from_long(0)    # send disable
        self.add_item_tag(0, count, key, value)

        return num_output_items


class qa_pa_ramp (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        # check ramp up
        steps = 4
        src = tag_source(steps+4)
        head = blocks.head(gr.sizeof_gr_complex, 12)
        dst = blocks.vector_sink_c()
        op = ieee802154g.pa_ramp(0.7, steps)
        self.tb.connect(src, head, op, dst)
        self.tb.run ()
        # check data
        result_data = dst.data()
        """print "result_data:",
        print result_data"""
        # full power should be reached at last step
        # check ramp up
        self.assertAlmostEqual(0.7, result_data[3].real)
        # check ramp down
        self.assertAlmostEqual(0.0, result_data[11].real)


if __name__ == '__main__':
    gr_unittest.run(qa_pa_ramp, "qa_pa_ramp.xml")
