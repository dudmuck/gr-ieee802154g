gr-ieee802154g
==============

MR-FSK implementation for GNU-RADIO

Build Process
==================
you must have the gnu-radio build environment setup as described http://gnuradio.org/redmine/projects/gnuradio/wiki/BuildGuide

git clone <this repository>

cd gr-ieee802154g

mkdir build

cd build

cmake ../

make

make test

sudo make install

sudo ldconfig


see example grc files in examples directory, for use with gnuradio-companion.
