


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "tables.h"
#include "quantize_pvt.h"
#include "lame_global_flags.h"
#include "gain_analysis.h"
#include "VbrTag.h"
#include "bitstream.h"
#include "tables.h"





#define MAX_LENGTH      32



#ifdef DEBUG
static int hogege;
#endif



static int
calcFrameLength(SessionConfig_t const *const cfg, int kbps, int pad)
{
  return 8 * ((cfg->version + 1) * 72000 * kbps / cfg->samplerate_out + pad);
}



int
getframebits(const lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t const *const eov = &gfc->ov_enc;
    int     bit_rate;

    
    if (eov->bitrate_index)
        bit_rate = bitrate_table[cfg->version][eov->bitrate_index];
    else
        bit_rate = cfg->avg_bitrate;
    
    assert(8 <= bit_rate && bit_rate <= 640);

    
    
    return calcFrameLength(cfg, bit_rate, eov->padding);
}

int
get_max_frame_buffer_size_by_constraint(SessionConfig_t const * cfg, int constraint)
{
    int     maxmp3buf = 0;
    if (cfg->avg_bitrate > 320) {
        
        if (constraint == MDB_STRICT_ISO) {
            maxmp3buf = calcFrameLength(cfg, cfg->avg_bitrate, 0);
        }
        else {
            
            maxmp3buf = 7680 * (cfg->version + 1);
        }
    }
    else {
        int     max_kbps;
        if (cfg->samplerate_out < 16000) {
            max_kbps = bitrate_table[cfg->version][8]; 
        }
        else {
            max_kbps = bitrate_table[cfg->version][14];
        }
        switch (constraint) 
        {
        default:
        case MDB_DEFAULT:
            
            
            maxmp3buf = 8 * 1440;
            break;
        case MDB_STRICT_ISO:
            maxmp3buf = calcFrameLength(cfg, max_kbps, 0);
            break;
        case MDB_MAXIMUM:
            maxmp3buf = 7680 * (cfg->version + 1);
            break;
        }
    }
    return maxmp3buf;
}


static void
putheader_bits(lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    Bit_stream_struc *bs = &gfc->bs;
#ifdef DEBUG
    hogege += cfg->sideinfo_len * 8;
#endif
    memcpy(&bs->buf[bs->buf_byte_idx], esv->header[esv->w_ptr].buf, cfg->sideinfo_len);
    bs->buf_byte_idx += cfg->sideinfo_len;
    bs->totbit += cfg->sideinfo_len * 8;
    esv->w_ptr = (esv->w_ptr + 1) & (MAX_HEADER_BUF - 1);
}





inline static void
putbits2(lame_internal_flags * gfc, int val, int j)
{
    EncStateVar_t const *const esv = &gfc->sv_enc;
    Bit_stream_struc *bs;
    bs = &gfc->bs;

    assert(j < MAX_LENGTH - 2);

    while (j > 0) {
        int     k;
        if (bs->buf_bit_idx == 0) {
            bs->buf_bit_idx = 8;
            bs->buf_byte_idx++;
            assert(bs->buf_byte_idx < BUFFER_SIZE);
            assert(esv->header[esv->w_ptr].write_timing >= bs->totbit);
            if (esv->header[esv->w_ptr].write_timing == bs->totbit) {
                putheader_bits(gfc);
            }
            bs->buf[bs->buf_byte_idx] = 0;
        }

        k = Min(j, bs->buf_bit_idx);
        j -= k;

        bs->buf_bit_idx -= k;

        assert(j < MAX_LENGTH); 
        assert(bs->buf_bit_idx < MAX_LENGTH);

        bs->buf[bs->buf_byte_idx] |= ((val >> j) << bs->buf_bit_idx);
        bs->totbit += k;
    }
}


inline static void
putbits_noheaders(lame_internal_flags * gfc, int val, int j)
{
    Bit_stream_struc *bs;
    bs = &gfc->bs;

    assert(j < MAX_LENGTH - 2);

    while (j > 0) {
        int     k;
        if (bs->buf_bit_idx == 0) {
            bs->buf_bit_idx = 8;
            bs->buf_byte_idx++;
            assert(bs->buf_byte_idx < BUFFER_SIZE);
            bs->buf[bs->buf_byte_idx] = 0;
        }

        k = Min(j, bs->buf_bit_idx);
        j -= k;

        bs->buf_bit_idx -= k;

        assert(j < MAX_LENGTH); 
        assert(bs->buf_bit_idx < MAX_LENGTH);

        bs->buf[bs->buf_byte_idx] |= ((val >> j) << bs->buf_bit_idx);
        bs->totbit += k;
    }
}




inline static void
drain_into_ancillary(lame_internal_flags * gfc, int remainingBits)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     i;
    assert(remainingBits >= 0);

    if (remainingBits >= 8) {
        putbits2(gfc, 0x4c, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x41, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x4d, 8);
        remainingBits -= 8;
    }
    if (remainingBits >= 8) {
        putbits2(gfc, 0x45, 8);
        remainingBits -= 8;
    }

    if (remainingBits >= 32) {
        const char *const version = get_lame_short_version();
        if (remainingBits >= 32)
            for (i = 0; i < (int) strlen(version) && remainingBits >= 8; ++i) {
                remainingBits -= 8;
                putbits2(gfc, version[i], 8);
            }
    }

    for (; remainingBits >= 1; remainingBits -= 1) {
        putbits2(gfc, esv->ancillary_flag, 1);
        esv->ancillary_flag ^= !cfg->disable_reservoir;
    }

    assert(remainingBits == 0);

}


inline static void
writeheader(lame_internal_flags * gfc, int val, int j)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     ptr = esv->header[esv->h_ptr].ptr;

    while (j > 0) {
        int const k = Min(j, 8 - (ptr & 7));
        j -= k;
        assert(j < MAX_LENGTH); 
        esv->header[esv->h_ptr].buf[ptr >> 3]
            |= ((val >> j)) << (8 - (ptr & 7) - k);
        ptr += k;
    }
    esv->header[esv->h_ptr].ptr = ptr;
}


static int
CRC_update(int value, int crc)
{
    int     i;
    value <<= 8;
    for (i = 0; i < 8; i++) {
        value <<= 1;
        crc <<= 1;

        if (((crc ^ value) & 0x10000))
            crc ^= CRC16_POLYNOMIAL;
    }
    return crc;
}


void
CRC_writeheader(lame_internal_flags const *gfc, char *header)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     crc = 0xffff;    
    int     i;

    crc = CRC_update(((unsigned char *) header)[2], crc);
    crc = CRC_update(((unsigned char *) header)[3], crc);
    for (i = 6; i < cfg->sideinfo_len; i++) {
        crc = CRC_update(((unsigned char *) header)[i], crc);
    }

    header[4] = crc >> 8;
    header[5] = crc & 255;
}

inline static void
encodeSideInfo2(lame_internal_flags * gfc, int bitsPerFrame)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t const *const eov = &gfc->ov_enc;
    EncStateVar_t *const esv = &gfc->sv_enc;
    III_side_info_t *l3_side;
    int     gr, ch;

    l3_side = &gfc->l3_side;
    esv->header[esv->h_ptr].ptr = 0;
    memset(esv->header[esv->h_ptr].buf, 0, cfg->sideinfo_len);
    if (cfg->samplerate_out < 16000)
        writeheader(gfc, 0xffe, 12);
    else
        writeheader(gfc, 0xfff, 12);
    writeheader(gfc, (cfg->version), 1);
    writeheader(gfc, 4 - 3, 2);
    writeheader(gfc, (!cfg->error_protection), 1);
    writeheader(gfc, (eov->bitrate_index), 4);
    writeheader(gfc, (cfg->samplerate_index), 2);
    writeheader(gfc, (eov->padding), 1);
    writeheader(gfc, (cfg->extension), 1);
    writeheader(gfc, (cfg->mode), 2);
    writeheader(gfc, (eov->mode_ext), 2);
    writeheader(gfc, (cfg->copyright), 1);
    writeheader(gfc, (cfg->original), 1);
    writeheader(gfc, (cfg->emphasis), 2);
    if (cfg->error_protection) {
        writeheader(gfc, 0, 16); 
    }

    if (cfg->version == 1) {
        
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 9);

        if (cfg->channels_out == 2)
            writeheader(gfc, l3_side->private_bits, 3);
        else
            writeheader(gfc, l3_side->private_bits, 5);

        for (ch = 0; ch < cfg->channels_out; ch++) {
            int     band;
            for (band = 0; band < 4; band++) {
                writeheader(gfc, l3_side->scfsi[ch][band], 1);
            }
        }

        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gr_info *const gi = &l3_side->tt[gr][ch];
                writeheader(gfc, gi->part2_3_length + gi->part2_length, 12);
                writeheader(gfc, gi->big_values / 2, 9);
                writeheader(gfc, gi->global_gain, 8);
                writeheader(gfc, gi->scalefac_compress, 4);

                if (gi->block_type != NORM_TYPE) {
                    writeheader(gfc, 1, 1); 
                    writeheader(gfc, gi->block_type, 2);
                    writeheader(gfc, gi->mixed_block_flag, 1);

                    if (gi->table_select[0] == 14)
                        gi->table_select[0] = 16;
                    writeheader(gfc, gi->table_select[0], 5);
                    if (gi->table_select[1] == 14)
                        gi->table_select[1] = 16;
                    writeheader(gfc, gi->table_select[1], 5);

                    writeheader(gfc, gi->subblock_gain[0], 3);
                    writeheader(gfc, gi->subblock_gain[1], 3);
                    writeheader(gfc, gi->subblock_gain[2], 3);
                }
                else {
                    writeheader(gfc, 0, 1); 
                    if (gi->table_select[0] == 14)
                        gi->table_select[0] = 16;
                    writeheader(gfc, gi->table_select[0], 5);
                    if (gi->table_select[1] == 14)
                        gi->table_select[1] = 16;
                    writeheader(gfc, gi->table_select[1], 5);
                    if (gi->table_select[2] == 14)
                        gi->table_select[2] = 16;
                    writeheader(gfc, gi->table_select[2], 5);

                    assert(0 <= gi->region0_count && gi->region0_count < 16);
                    assert(0 <= gi->region1_count && gi->region1_count < 8);
                    writeheader(gfc, gi->region0_count, 4);
                    writeheader(gfc, gi->region1_count, 3);
                }
                writeheader(gfc, gi->preflag, 1);
                writeheader(gfc, gi->scalefac_scale, 1);
                writeheader(gfc, gi->count1table_select, 1);
            }
        }
    }
    else {
        
        assert(l3_side->main_data_begin >= 0);
        writeheader(gfc, (l3_side->main_data_begin), 8);
        writeheader(gfc, l3_side->private_bits, cfg->channels_out);

        gr = 0;
        for (ch = 0; ch < cfg->channels_out; ch++) {
            gr_info *const gi = &l3_side->tt[gr][ch];
            writeheader(gfc, gi->part2_3_length + gi->part2_length, 12);
            writeheader(gfc, gi->big_values / 2, 9);
            writeheader(gfc, gi->global_gain, 8);
            writeheader(gfc, gi->scalefac_compress, 9);

            if (gi->block_type != NORM_TYPE) {
                writeheader(gfc, 1, 1); 
                writeheader(gfc, gi->block_type, 2);
                writeheader(gfc, gi->mixed_block_flag, 1);

                if (gi->table_select[0] == 14)
                    gi->table_select[0] = 16;
                writeheader(gfc, gi->table_select[0], 5);
                if (gi->table_select[1] == 14)
                    gi->table_select[1] = 16;
                writeheader(gfc, gi->table_select[1], 5);

                writeheader(gfc, gi->subblock_gain[0], 3);
                writeheader(gfc, gi->subblock_gain[1], 3);
                writeheader(gfc, gi->subblock_gain[2], 3);
            }
            else {
                writeheader(gfc, 0, 1); 
                if (gi->table_select[0] == 14)
                    gi->table_select[0] = 16;
                writeheader(gfc, gi->table_select[0], 5);
                if (gi->table_select[1] == 14)
                    gi->table_select[1] = 16;
                writeheader(gfc, gi->table_select[1], 5);
                if (gi->table_select[2] == 14)
                    gi->table_select[2] = 16;
                writeheader(gfc, gi->table_select[2], 5);

                assert(0 <= gi->region0_count && gi->region0_count < 16);
                assert(0 <= gi->region1_count && gi->region1_count < 8);
                writeheader(gfc, gi->region0_count, 4);
                writeheader(gfc, gi->region1_count, 3);
            }

            writeheader(gfc, gi->scalefac_scale, 1);
            writeheader(gfc, gi->count1table_select, 1);
        }
    }

    if (cfg->error_protection) {
        
        CRC_writeheader(gfc, esv->header[esv->h_ptr].buf);
    }

    {
        int const old = esv->h_ptr;
        assert(esv->header[old].ptr == cfg->sideinfo_len * 8);

        esv->h_ptr = (old + 1) & (MAX_HEADER_BUF - 1);
        esv->header[esv->h_ptr].write_timing = esv->header[old].write_timing + bitsPerFrame;

        if (esv->h_ptr == esv->w_ptr) {
            
            ERRORF(gfc, "Error: MAX_HEADER_BUF too small in bitstream.c \n");
        }

    }
}


inline static int
huffman_coder_count1(lame_internal_flags * gfc, gr_info const *gi)
{
    
    struct huffcodetab const *const h = &ht[gi->count1table_select + 32];
    int     i, bits = 0;
#ifdef DEBUG
    int     gegebo = gfc->bs.totbit;
#endif

    int const *ix = &gi->l3_enc[gi->big_values];
    FLOAT const *xr = &gi->xr[gi->big_values];
    assert(gi->count1table_select < 2);

    for (i = (gi->count1 - gi->big_values) / 4; i > 0; --i) {
        int     huffbits = 0;
        int     p = 0, v;

        v = ix[0];
        if (v) {
            p += 8;
            if (xr[0] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[1];
        if (v) {
            p += 4;
            huffbits *= 2;
            if (xr[1] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[2];
        if (v) {
            p += 2;
            huffbits *= 2;
            if (xr[2] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        v = ix[3];
        if (v) {
            p++;
            huffbits *= 2;
            if (xr[3] < 0.0f)
                huffbits++;
            assert(v <= 1);
        }

        ix += 4;
        xr += 4;
        putbits2(gfc, huffbits + h->table[p], h->hlen[p]);
        bits += h->hlen[p];
    }
#ifdef DEBUG
    DEBUGF(gfc, "count1: real: %ld counted:%d (bigv %d count1len %d)\n",
           gfc->bs.totbit - gegebo, gi->count1bits, gi->big_values, gi->count1);
#endif
    return bits;
}




inline static int
Huffmancode(lame_internal_flags * const gfc, const unsigned int tableindex,
            int start, int end, gr_info const *gi)
{
    struct huffcodetab const *const h = &ht[tableindex];
    unsigned int const linbits = h->xlen;
    int     i, bits = 0;

    assert(tableindex < 32u);
    if (!tableindex)
        return bits;

    for (i = start; i < end; i += 2) {
        int16_t  cbits = 0;
        uint16_t xbits = 0;
        unsigned int xlen = h->xlen;
        unsigned int ext = 0;
        unsigned int x1 = gi->l3_enc[i];
        unsigned int x2 = gi->l3_enc[i + 1];

        assert(gi->l3_enc[i] >= 0);
        assert(gi->l3_enc[i+1] >= 0);

        if (x1 != 0u) {
            if (gi->xr[i] < 0.0f)
                ext++;
            cbits--;
        }

        if (tableindex > 15u) {
            
            if (x1 >= 15u) {
                uint16_t const linbits_x1 = x1 - 15u;
                assert(linbits_x1 <= h->linmax);
                ext |= linbits_x1 << 1u;
                xbits = linbits;
                x1 = 15u;
            }

            if (x2 >= 15u) {
                uint16_t const linbits_x2 = x2 - 15u;
                assert(linbits_x2 <= h->linmax);
                ext <<= linbits;
                ext |= linbits_x2;
                xbits += linbits;
                x2 = 15u;
            }
            xlen = 16;
        }

        if (x2 != 0u) {
            ext <<= 1;
            if (gi->xr[i + 1] < 0.0f)
                ext++;
            cbits--;
        }

        assert((x1 | x2) < 16u);

        x1 = x1 * xlen + x2;
        xbits -= cbits;
        cbits += h->hlen[x1];

        assert(cbits <= MAX_LENGTH);
        assert(xbits <= MAX_LENGTH);

        putbits2(gfc, h->table[x1], cbits);
        putbits2(gfc, (int)ext, xbits);
        bits += cbits + xbits;
    }
    return bits;
}


static int
ShortHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    int     bits;
    int     region1Start;

    region1Start = 3 * gfc->scalefac_band.s[3];
    if (region1Start > gi->big_values)
        region1Start = gi->big_values;

    
    bits = Huffmancode(gfc, gi->table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->table_select[1], region1Start, gi->big_values, gi);
    return bits;
}

static int
LongHuffmancodebits(lame_internal_flags * gfc, gr_info const *gi)
{
    unsigned int i;
    int     bigvalues, bits;
    int     region1Start, region2Start;

    bigvalues = gi->big_values;
    assert(0 <= bigvalues && bigvalues <= 576);

    assert(gi->region0_count >= -1);
    assert(gi->region1_count >= -1);
    i = gi->region0_count + 1;
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region1Start = gfc->scalefac_band.l[i];
    i += gi->region1_count + 1;
    assert((size_t) i < dimension_of(gfc->scalefac_band.l));
    region2Start = gfc->scalefac_band.l[i];

    if (region1Start > bigvalues)
        region1Start = bigvalues;

    if (region2Start > bigvalues)
        region2Start = bigvalues;

    bits = Huffmancode(gfc, gi->table_select[0], 0, region1Start, gi);
    bits += Huffmancode(gfc, gi->table_select[1], region1Start, region2Start, gi);
    bits += Huffmancode(gfc, gi->table_select[2], region2Start, bigvalues, gi);
    return bits;
}

inline static int
writeMainData(lame_internal_flags * const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    III_side_info_t const *const l3_side = &gfc->l3_side;
    int     gr, ch, sfb, data_bits, tot_bits = 0;

    if (cfg->version == 1) {
        
        for (gr = 0; gr < 2; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gr_info const *const gi = &l3_side->tt[gr][ch];
                int const slen1 = slen1_tab[gi->scalefac_compress];
                int const slen2 = slen2_tab[gi->scalefac_compress];
                data_bits = 0;
#ifdef DEBUG
                hogege = gfc->bs.totbit;
#endif
                for (sfb = 0; sfb < gi->sfbdivide; sfb++) {
                    if (gi->scalefac[sfb] == -1)
                        continue; 
                    putbits2(gfc, gi->scalefac[sfb], slen1);
                    data_bits += slen1;
                }
                for (; sfb < gi->sfbmax; sfb++) {
                    if (gi->scalefac[sfb] == -1)
                        continue; 
                    putbits2(gfc, gi->scalefac[sfb], slen2);
                    data_bits += slen2;
                }
                assert(data_bits == gi->part2_length);

                if (gi->block_type == SHORT_TYPE) {
                    data_bits += ShortHuffmancodebits(gfc, gi);
                }
                else {
                    data_bits += LongHuffmancodebits(gfc, gi);
                }
                data_bits += huffman_coder_count1(gfc, gi);
#ifdef DEBUG
                DEBUGF(gfc, "<%ld> ", gfc->bs.totbit - hogege);
#endif
                
                assert(data_bits == gi->part2_3_length + gi->part2_length);
                tot_bits += data_bits;
            }           
        }               
    }
    else {
        
        gr = 0;
        for (ch = 0; ch < cfg->channels_out; ch++) {
            gr_info const *const gi = &l3_side->tt[gr][ch];
            int     i, sfb_partition, scale_bits = 0;
            assert(gi->sfb_partition_table);
            data_bits = 0;
#ifdef DEBUG
            hogege = gfc->bs.totbit;
#endif
            sfb = 0;
            sfb_partition = 0;

            if (gi->block_type == SHORT_TYPE) {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->sfb_partition_table[sfb_partition] / 3;
                    int const slen = gi->slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 0], 0), slen);
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 1], 0), slen);
                        putbits2(gfc, Max(gi->scalefac[sfb * 3 + 2], 0), slen);
                        scale_bits += 3 * slen;
                    }
                }
                data_bits += ShortHuffmancodebits(gfc, gi);
            }
            else {
                for (; sfb_partition < 4; sfb_partition++) {
                    int const sfbs = gi->sfb_partition_table[sfb_partition];
                    int const slen = gi->slen[sfb_partition];
                    for (i = 0; i < sfbs; i++, sfb++) {
                        putbits2(gfc, Max(gi->scalefac[sfb], 0), slen);
                        scale_bits += slen;
                    }
                }
                data_bits += LongHuffmancodebits(gfc, gi);
            }
            data_bits += huffman_coder_count1(gfc, gi);
#ifdef DEBUG
            DEBUGF(gfc, "<%ld> ", gfc->bs.totbit - hogege);
#endif
            
            assert(data_bits == gi->part2_3_length);
            assert(scale_bits == gi->part2_length);
            tot_bits += scale_bits + data_bits;
        }               
    }                   
    return tot_bits;
}                       




int
compute_flushbits(const lame_internal_flags * gfc, int *total_bytes_output)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t const *const esv = &gfc->sv_enc;
    int     flushbits, remaining_headers;
    int     bitsPerFrame;
    int     last_ptr, first_ptr;
    first_ptr = esv->w_ptr; 
    last_ptr = esv->h_ptr - 1; 
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;

    
    flushbits = esv->header[last_ptr].write_timing - gfc->bs.totbit;
    *total_bytes_output = flushbits;

    if (flushbits >= 0) {
        
        
        remaining_headers = 1 + last_ptr - first_ptr;
        if (last_ptr < first_ptr)
            remaining_headers = 1 + last_ptr - first_ptr + MAX_HEADER_BUF;
        flushbits -= remaining_headers * 8 * cfg->sideinfo_len;
    }


    
    bitsPerFrame = getframebits(gfc);
    flushbits += bitsPerFrame;
    *total_bytes_output += bitsPerFrame;
    
    if (*total_bytes_output % 8)
        *total_bytes_output = 1 + (*total_bytes_output / 8);
    else
        *total_bytes_output = (*total_bytes_output / 8);
    *total_bytes_output += gfc->bs.buf_byte_idx + 1;


    if (flushbits < 0) {
#if 0
        
        DEBUGF(gfc, "last header write_timing = %i \n", esv->header[last_ptr].write_timing);
        DEBUGF(gfc, "first header write_timing = %i \n", esv->header[first_ptr].write_timing);
        DEBUGF(gfc, "bs.totbit:                 %i \n", gfc->bs.totbit);
        DEBUGF(gfc, "first_ptr, last_ptr        %i %i \n", first_ptr, last_ptr);
        DEBUGF(gfc, "remaining_headers =        %i \n", remaining_headers);
        DEBUGF(gfc, "bitsperframe:              %i \n", bitsPerFrame);
        DEBUGF(gfc, "sidelen:                   %i \n", cfg->sideinfo_len);
#endif
        ERRORF(gfc, "strange error flushing buffer ... \n");
    }
    return flushbits;
}


void
flush_bitstream(lame_internal_flags * gfc)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    III_side_info_t *l3_side;
    int     nbytes;
    int     flushbits;
    int     last_ptr = esv->h_ptr - 1; 
    if (last_ptr == -1)
        last_ptr = MAX_HEADER_BUF - 1;
    l3_side = &gfc->l3_side;


    if ((flushbits = compute_flushbits(gfc, &nbytes)) < 0)
        return;
    drain_into_ancillary(gfc, flushbits);

    
    assert(esv->header[last_ptr].write_timing + getframebits(gfc)
           == gfc->bs.totbit);

    
    esv->ResvSize = 0;
    l3_side->main_data_begin = 0;
}




void
add_dummy_byte(lame_internal_flags * gfc, unsigned char val, unsigned int n)
{
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     i;

    while (n-- > 0u) {
        putbits_noheaders(gfc, val, 8);

        for (i = 0; i < MAX_HEADER_BUF; ++i)
            esv->header[i].write_timing += 8;
    }
}



int
format_bitstream(lame_internal_flags * gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     bits, nbytes;
    III_side_info_t *l3_side;
    int     bitsPerFrame;
    l3_side = &gfc->l3_side;

    bitsPerFrame = getframebits(gfc);
    drain_into_ancillary(gfc, l3_side->resvDrain_pre);

    encodeSideInfo2(gfc, bitsPerFrame);
    bits = 8 * cfg->sideinfo_len;
    bits += writeMainData(gfc);
    drain_into_ancillary(gfc, l3_side->resvDrain_post);
    bits += l3_side->resvDrain_post;

    l3_side->main_data_begin += (bitsPerFrame - bits) / 8;

    
    if (compute_flushbits(gfc, &nbytes) != esv->ResvSize) {
        ERRORF(gfc, "Internal buffer inconsistency. flushbits <> ResvSize");
    }


    
    if ((l3_side->main_data_begin * 8) != esv->ResvSize) {
        ERRORF(gfc, "bit reservoir error: \n"
               "l3_side->main_data_begin: %i \n"
               "Resvoir size:             %i \n"
               "resv drain (post)         %i \n"
               "resv drain (pre)          %i \n"
               "header and sideinfo:      %i \n"
               "data bits:                %i \n"
               "total bits:               %i (remainder: %i) \n"
               "bitsperframe:             %i \n",
               8 * l3_side->main_data_begin,
               esv->ResvSize,
               l3_side->resvDrain_post,
               l3_side->resvDrain_pre,
               8 * cfg->sideinfo_len,
               bits - l3_side->resvDrain_post - 8 * cfg->sideinfo_len,
               bits, bits % 8, bitsPerFrame);

        ERRORF(gfc, "This is a fatal error.  It has several possible causes:");
        ERRORF(gfc, "90%%  LAME compiled with buggy version of gcc using advanced optimizations");
        ERRORF(gfc, " 9%%  Your system is overclocked");
        ERRORF(gfc, " 1%%  bug in LAME encoding library");

        esv->ResvSize = l3_side->main_data_begin * 8;
    };
    assert(gfc->bs.totbit % 8 == 0);

    if (gfc->bs.totbit > 1000000000) {
        
        int     i;
        for (i = 0; i < MAX_HEADER_BUF; ++i)
            esv->header[i].write_timing -= gfc->bs.totbit;
        gfc->bs.totbit = 0;
    }


    return 0;
}


static int
do_gain_analysis(lame_internal_flags * gfc, unsigned char* buffer, int minimum)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    RpgStateVar_t const *const rsv = &gfc->sv_rpg;
    RpgResult_t *const rov = &gfc->ov_rpg;
#ifdef DECODE_ON_THE_FLY
    if (cfg->decode_on_the_fly) { 
        sample_t pcm_buf[2][1152];
        int     mp3_in = minimum;
        int     samples_out = -1;

        
        while (samples_out != 0) {

            samples_out = hip_decode1_unclipped(gfc->hip, buffer, mp3_in, pcm_buf[0], pcm_buf[1]);
            

            
            mp3_in = 0;

            if (samples_out == -1) {
                
                samples_out = 0;
            }
            if (samples_out > 0) {
                

                
                assert(samples_out <= 1152);

                if (cfg->findPeakSample) {
                    int     i;
                    
                    for (i = 0; i < samples_out; i++) {
                        if (pcm_buf[0][i] > rov->PeakSample)
                            rov->PeakSample = pcm_buf[0][i];
                        else if (-pcm_buf[0][i] > rov->PeakSample)
                            rov->PeakSample = -pcm_buf[0][i];
                    }
                    if (cfg->channels_out > 1)
                        for (i = 0; i < samples_out; i++) {
                            if (pcm_buf[1][i] > rov->PeakSample)
                                rov->PeakSample = pcm_buf[1][i];
                            else if (-pcm_buf[1][i] > rov->PeakSample)
                                rov->PeakSample = -pcm_buf[1][i];
                        }
                }

                if (cfg->findReplayGain)
                    if (AnalyzeSamples
                        (rsv->rgdata, pcm_buf[0], pcm_buf[1], samples_out,
                         cfg->channels_out) == GAIN_ANALYSIS_ERROR)
                        return -6;

            }       
        }           
    }               
#endif
    return minimum;
}

static int
do_copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int size)
{
    Bit_stream_struc *const bs = &gfc->bs;
    int const minimum = bs->buf_byte_idx + 1;
    if (minimum <= 0)
        return 0;
    if (size != 0 && minimum > size)
        return -1;      
    memcpy(buffer, bs->buf, minimum);
    bs->buf_byte_idx = -1;
    bs->buf_bit_idx = 0;
    return minimum;
}


int
copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int size, int mp3data)
{
    int const minimum = do_copy_buffer(gfc, buffer, size);
    if (minimum > 0 && mp3data) {
        UpdateMusicCRC(&gfc->nMusicCRC, buffer, minimum);

        
        gfc->VBR_seek_table.nBytesWritten += minimum;

        return do_gain_analysis(gfc, buffer, minimum);
    }                   
    return minimum;
}


void
init_bit_stream_w(lame_internal_flags * gfc)
{
    EncStateVar_t *const esv = &gfc->sv_enc;

    esv->h_ptr = esv->w_ptr = 0;
    esv->header[esv->h_ptr].write_timing = 0;

    gfc->bs.buf = (unsigned char *) malloc(BUFFER_SIZE);
    gfc->bs.buf_size = BUFFER_SIZE;
    gfc->bs.buf_byte_idx = -1;
    gfc->bs.buf_bit_idx = 0;
    gfc->bs.totbit = 0;
}


