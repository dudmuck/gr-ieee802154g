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

from gnuradio import gr, gr_unittest, digital, blocks
import ieee802154g_swig as ieee802154g

def hex_list_to_binary_list(s):
    r = []
    for e in s:
        for i in range(8):
            t = (e >> (7-i)) & 0x1
            r.append(t)
    return r;

class qa_framer_sink_mrfsk (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        # CRC-32 test from section 5.2.1.9 in 802.15.4g-2012
        pad = (0xff,) * 8
        src_data = pad + (0x55, 0x55, 0x55, 0x55, 0x90, 0x4e, 0x00, 0x07, 0x40, 0x00, 0x56, 0x5d, 0x29, 0xfa, 0x28) + pad
        src_data_list = hex_list_to_binary_list(src_data)
        expected_str = '\x40\x00\x56\x5d\x29\xfa\x28'

        rcvd_pktq = gr.msg_queue()

        src = blocks.vector_source_b(src_data_list)
        correlator = digital.correlate_access_code_bb('0101010101011001000001001110', 1)
        framer_sink = ieee802154g.framer_sink_mrfsk(rcvd_pktq)
        vsnk = blocks.vector_sink_b()

        self.tb.connect(src, correlator, framer_sink)
        self.tb.connect(correlator, vsnk)
        self.tb.run ()

        # check data
        self.assertEquals(1, rcvd_pktq.count())
        result_msg = rcvd_pktq.delete_head()
        fec = int(result_msg.type())
        self.assertEquals(0, fec)
        phr = int(result_msg.arg1())
        self.assertEquals(0x0007, phr)
        crc_ok = int(result_msg.arg2())
        self.assertEquals(1, crc_ok)
        self.assertEquals(expected_str, result_msg.to_string())

    def test_002_t (self):
        # CRC-16 test from section 5.2.1.9 in 802.15.4-2011
        pad = (0xff,) * 8
        src_data = pad + (0x55, 0x55, 0x90, 0x4e, 0x10, 0x05, 0x40, 0x00, 0x56, 0x27, 0x9e) + pad
        src_data_list = hex_list_to_binary_list(src_data)
        expected_str = '\x40\x00\x56\x27\x9e'

        rcvd_pktq = gr.msg_queue()

        src = blocks.vector_source_b(src_data_list)
        correlator = digital.correlate_access_code_bb('0101010101011001000001001110', 1)
        framer_sink = ieee802154g.framer_sink_mrfsk(rcvd_pktq)
        vsnk = blocks.vector_sink_b()

        self.tb.connect(src, correlator, framer_sink)
        self.tb.connect(correlator, vsnk)
        self.tb.run ()

        # check data
        self.assertEquals(1, rcvd_pktq.count())
        result_msg = rcvd_pktq.delete_head()
        fec = int(result_msg.type())
        self.assertEquals(0, fec)
        phr = int(result_msg.arg1())
        self.assertEquals(0x1005, phr)
        crc_ok = int(result_msg.arg2())
        self.assertEquals(1, crc_ok)
        self.assertEquals(expected_str, result_msg.to_string())

    def test_003_t (self):
        # Annex O.3 example 2: whitening enabled
        pad = (0xff,) * 8
        src_data = pad + (0x55, 0x55, 0x55, 0x90, 0x4e, 0x08, 0x07, 0x4f, 0x70, 0xe5, 0x32, 0x6a, 0x62, 0x60) + pad
        src_data_list = hex_list_to_binary_list(src_data)
        expected_str = '\x40\x00\x56\x5d\x29\xfa\x28'

        rcvd_pktq = gr.msg_queue()

        src = blocks.vector_source_b(src_data_list)
        correlator = digital.correlate_access_code_bb('0101010101011001000001001110', 1)
        framer_sink = ieee802154g.framer_sink_mrfsk(rcvd_pktq)
        vsnk = blocks.vector_sink_b()

        self.tb.connect(src, correlator, framer_sink)
        self.tb.connect(correlator, vsnk)
        self.tb.run ()

        # check data
        self.assertEquals(1, rcvd_pktq.count())
        result_msg = rcvd_pktq.delete_head()
        fec = int(result_msg.type())
        self.assertEquals(0, fec)
        phr = int(result_msg.arg1())
        self.assertEquals(0x0807, phr)
        crc_ok = int(result_msg.arg2())
        self.assertEquals(1, crc_ok)
        self.assertEquals(expected_str, result_msg.to_string())
        #result_str = result_msg.to_string()
        #for x in result_str:
        #    print x.encode('hex')

if __name__ == '__main__':
    gr_unittest.run(qa_framer_sink_mrfsk, "qa_framer_sink_mrfsk.xml")
