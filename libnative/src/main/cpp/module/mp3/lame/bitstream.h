

#ifndef LAME_BITSTREAM_H
#define LAME_BITSTREAM_H

int     getframebits(const lame_internal_flags * gfc);

int     format_bitstream(lame_internal_flags * gfc);

void    flush_bitstream(lame_internal_flags * gfc);
void    add_dummy_byte(lame_internal_flags * gfc, unsigned char val, unsigned int n);

int     copy_buffer(lame_internal_flags * gfc, unsigned char *buffer, int buffer_size,
                    int update_crc);
void    init_bit_stream_w(lame_internal_flags * gfc);
void    CRC_writeheader(lame_internal_flags const *gfc, char *buffer);
int     compute_flushbits(const lame_internal_flags * gfp, int *nbytes);

int     get_max_frame_buffer_size_by_constraint(SessionConfig_t const * cfg, int constraint);

#endif
