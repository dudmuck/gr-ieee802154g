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
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define aMaxPHYPacketSize   2047        // section 9.2, table 70

#define PHR_LENGTH  2   // two octets
typedef union { // as defined in section 18.1.1.3
    struct {
        uint16_t frame_length   : 11; // 10->0
        uint16_t DW             :  1; // 11
        uint16_t FCS            :  1; // 12         1=16bitCRC, 0=32bitCRC
        uint16_t reserved       :  2; // 14,13
        uint16_t MS             :  1; // 15         always 0 for non-mode-switch
    } bits;
    uint16_t word;
} MRFSK_PHR_t;

#define INITIAL_CRC16       0x0000

#define INITIAL_CRC32   0xffffffff

unsigned int digital_update_crc32(unsigned int crc, const unsigned char *data, size_t len);
uint16_t crc_msb_first(uint16_t crc, uint8_t const *p, int len);
void interleave_u32(uint32_t *u32);
void interleave(uint8_t *buf);
uint8_t get_pn9_byte(uint16_t *);
uint8_t reverse_octet(uint8_t);

#ifdef __cplusplus
}
#endif

