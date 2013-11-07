/* -*- c++ -*- */

#define IEEE802154G_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "ieee802154g_swig_doc.i"

%{
#include "ieee802154g/mrfsk_source.h"
#include "ieee802154g/pa_ramp.h"
#include "ieee802154g/framer_sink_mrfsk.h"
#include "ieee802154g/framer_sink_mrfsk_nrnsc.h"
#include "ieee802154g/preamble_detector.h"
%}


%include "ieee802154g/mrfsk_source.h"
GR_SWIG_BLOCK_MAGIC2(ieee802154g, mrfsk_source);
%include "ieee802154g/pa_ramp.h"
GR_SWIG_BLOCK_MAGIC2(ieee802154g, pa_ramp);
%include "ieee802154g/framer_sink_mrfsk.h"
GR_SWIG_BLOCK_MAGIC2(ieee802154g, framer_sink_mrfsk);
%include "ieee802154g/framer_sink_mrfsk_nrnsc.h"
GR_SWIG_BLOCK_MAGIC2(ieee802154g, framer_sink_mrfsk_nrnsc);
%include "ieee802154g/preamble_detector.h"
GR_SWIG_BLOCK_MAGIC2(ieee802154g, preamble_detector);
