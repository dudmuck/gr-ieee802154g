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

class qa_preamble_detector (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        test = ieee802154g.preamble_detector(8)

        # typical preamble cycle, at 8 samples per symbol.
        num_cycles = 20
        data = num_cycles * [-0.098, -0.064, 0.064, 0.093, 0.081, 0.071, 0.069, 0.065, 0.070, 0.037, -0.087, -0.118, -0.105, -0.101, -0.099, -0.095]
        src = blocks.vector_source_f(data, False)
        snk = blocks.vector_sink_f()

        self.tb.connect(src, test, snk)
        self.tb.run ()

        dst_data = snk.data()
        # check data
        assert len(dst_data) == num_cycles * 2
        cnt = 0
        locked = False
        pos = False
        for n in dst_data:
            if not locked:
                if n > 0.08:    # found peak positive
                    locked = True
                    pos = True
                elif cnt > 24:
                    assert False, "failed to lock"
                    break
            else:
                if pos:
                    assert n < 0
                else:
                    assert n > 0
                pos = not pos
            cnt += 1


if __name__ == '__main__':
    gr_unittest.run(qa_preamble_detector, "qa_preamble_detector.xml")
