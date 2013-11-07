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
# based off of: gr-blocks/python/blocks/qa_block_gateway.py
import numpy
import pmt

from gnuradio import gr, gr_unittest, blocks
import ieee802154g_swig as ieee802154g

class tag_sink(gr.sync_block):
    def __init__(self):
        gr.sync_block.__init__(
            self,
            name = "tag sink",
            in_sig = [numpy.byte],
            out_sig = None
        )
        self.tx_on_at = -1
        self.tx_off_at = -1

    def work(self, input_items, output_items):
        num_input_items = len(input_items[0])
        nread = self.nitems_read(0)
        tags = self.get_tags_in_range(0, nread, nread+num_input_items);
        for tag in tags:
            if pmt.symbol_to_string(tag.key) == 'pa_ramp':
                tx_on = pmt.to_long(tag.value)
                if tx_on == 1:
                    self.tx_on_at = tag.offset
                else:
                    self.tx_off_at = tag.offset
                

        return num_input_items

class qa_mrfsk_source (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        # CRC-32 test from section 5.2.1.9 in 802.15.4g-2012
        pkt_src = ieee802154g.mrfsk_source(
            1,      #num_iterations
            4,      #preamble_size
            False,  #fec_en
            False,  #dw
            False,  #crc_type_16
            2,      #payload_type (0x400056)
            7,      #psdu_len  (ignored with payload_type=2)
            0       #delay_bytes
        )
        dst = blocks.vector_sink_b()
        tsink = tag_sink()
        expected_results = (0x55, 0x55, 0x55, 0x55, 0x90, 0x4e, 0x00, 0x07, 0x40, 0x00, 0x56, 0x5d, 0x29, 0xfa, 0x28)

        self.tb.connect(pkt_src, dst)
        self.tb.connect(pkt_src, tsink)
        self.tb.run ()
        result = dst.data()
        result_txon = result[tsink.tx_on_at:tsink.tx_off_at-1];
        # check data
        self.assertEqual(expected_results, result_txon)

    def test_002_t (self):
        # CRC-16 test from section 5.2.1.9 in 802.15.4-2011
        pkt_src = ieee802154g.mrfsk_source(
            1,      #num_iterations
            1,      #preamble_size
            False,  #fec_en
            False,  #dw
            True,   #crc_type_16
            2,      #payload_type (0x400056)
            5,      #psdu_len  (ignored with payload_type=2)
            0       #delay_bytes
        )
        dst = blocks.vector_sink_b()
        tsink = tag_sink()
        expected_results = (0x55, 0x90, 0x4e, 0x10, 0x05, 0x40, 0x00, 0x56, 0x27, 0x9e)

        self.tb.connect(pkt_src, dst)
        self.tb.connect(pkt_src, tsink)
        self.tb.run ()
        result = dst.data()
        result_txon = result[tsink.tx_on_at:tsink.tx_off_at-1];
        self.assertEqual(expected_results, result_txon)

    def test_003_t (self):
        # Annex O.3 example 2: whitening enabled
        pkt_src = ieee802154g.mrfsk_source(
            1,      #num_iterations
            4,      #preamble_size
            False,  #fec_en
            True,   #dw
            False,   #crc_type_16
            2,      #payload_type (0x400056)
            7,      #psdu_len  (ignored with payload_type=2)
            0       #delay_bytes
        )
        dst = blocks.vector_sink_b()
        tsink = tag_sink()
        # Annex O.3.5
        expected_results = (0x55, 0x55, 0x55, 0x55, 0x90, 0x4e, 0x08, 0x07, 0x4f, 0x70, 0xe5, 0x32, 0x6a, 0x62, 0x60)

        self.tb.connect(pkt_src, dst)
        self.tb.connect(pkt_src, tsink)
        self.tb.run ()
        result = dst.data()
        result_txon = result[tsink.tx_on_at:tsink.tx_off_at-1];
        self.assertEqual(expected_results, result_txon)

    def test_004_t (self):
        # Annex O example 4, NRNSC with 0x6f4e SFD
        pkt_src = ieee802154g.mrfsk_source(
            1,      #num_iterations
            4,      #preamble_size
            True,  #fec_en
            False,   #dw
            False,   #crc_type_16
            2,      #payload_type (0x400056)
            7,      #psdu_len  (ignored with payload_type=2)
            0       #delay_bytes
        )
        dst = blocks.vector_sink_b()
        tsink = tag_sink()
        expected_results = ( 0x55, 0x55, 0x55, 0x55, 0x6f, 0x4e,
            0xbf, 0x7f, 0x3f, 0xff, 0xfc, 0xfd, 0xfc, 0xf2, 0x37, 0xaa,
            0xbc, 0xb7, 0x5e, 0x13, 0xa4, 0x5d, 0xb2, 0xf0, 0xb4, 0x3c)

        self.tb.connect(pkt_src, dst)
        self.tb.connect(pkt_src, tsink)
        self.tb.run ()
        result = dst.data()
        #for i in range(tsink.tx_on_at, tsink.tx_off_at):
        #    print "%d: %02x" % (i, result[i])
        result_txon = result[tsink.tx_on_at:tsink.tx_off_at-1];
        #print result_txon
        #print expected_results 
        self.assertEqual(expected_results, result_txon)

if __name__ == '__main__':
    gr_unittest.run(qa_mrfsk_source, "qa_mrfsk_source.xml")
