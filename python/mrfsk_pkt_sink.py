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

import numpy
from gnuradio import gr, digital
import gnuradio.gr.gr_threading as _threading
import ieee802154g

DEFAULT_MSGQ_LIMIT = 2

####class mrfsk_pkt_sink(gr.sync_block):###
class mrfsk_pkt_sink(gr.hier_block2):
    """
    docstring for block mrfsk_pkt_sink
    """
    def __init__(self):
#        gr.sync_block.__init__(self,
#            name="mrfsk_pkt_sink",
#            in_sig=[numpy.uint8],
#            out_sig=None)
        gr.hier_block2.__init__(
            self,
            "mrfsk_pkt_sink",
            gr.io_signature(1, 1, gr.sizeof_char),  # Input signature
            gr.io_signature(0, 0, 0)                # Output signature
        )

        msgq = gr.msg_queue(DEFAULT_MSGQ_LIMIT) # holds packets from the PHY
        ## last nibble of preamble, and 0x904e for SFD uncoded packet
        # last byte of preamble, and 0x904e for SFD uncoded packet
        # add more preamble to this if excessive false triggering
        correlator       = digital.correlate_access_code_bb('0101010101011001000001001110', 1)
        correlator_nrnsc = digital.correlate_access_code_bb('0101010101010110111101001110', 2)
        framer_sink = ieee802154g.framer_sink_mrfsk(msgq)
        framer_sink_nrnsc = ieee802154g.framer_sink_mrfsk_nrnsc(msgq)
        #connect
        self.connect(self, correlator, framer_sink)
        self.connect(self, correlator_nrnsc, framer_sink_nrnsc)
        #start thread
        _packet_decoder_thread(msgq)


    def work(self, input_items, output_items):
        in0 = input_items[0]
        # <+signal processing here+>
        return len(input_items[0])


class _packet_decoder_thread(_threading.Thread):

    def __init__(self, msgq):
        _threading.Thread.__init__(self)
        self.setDaemon(1)
        self._msgq = msgq
        self.keep_running = True

        self.pkts_good = 0
        self.pkts_bad = 0

        self.start()

    def run(self):
        while self.keep_running:
            msg = self._msgq.delete_head()
            phr = int(msg.arg1())
            fec = int(msg.type())
            if fec != 0:
                print "FEC ",
            print "PHR:%04x " % phr,
            if phr & 0x0800:
                print "dw ",
            crc_ok = int(msg.arg2())
            if crc_ok == 1:
                self.pkts_good += 1
                if phr & 0x1000:
                    print "CRC16-ok    ",
                else:
                    print "crc32-ok    ",
            else:
                self.pkts_bad += 1
                if phr & 0x1000:
                    print "[41mCRC16-fail    ",
                else:
                    print "[41mcrc32-fail    ",

            s = msg.to_string()
            print ' '.join(x.encode('hex') for x in s)
            if crc_ok == 0:
                print "[0m",
            print "good:%d, bad:%d" % (self.pkts_good, self.pkts_bad)

