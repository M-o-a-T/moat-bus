#ifndef MOATBUS_MESSAGE
#define MOATBUS_MESSAGE

/*
Message structure for MoatBus

This interface mostly mimics message.py
*/

#include <sys/types.h>
#include "moat/common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum HDL_RES {
    RES_WORKING = 0,
    RES_FREE,
    RES_SUCCESS,
    RES_MISSING,
    RES_ERROR,
    RES_FATAL,
};

// one byte for now
typedef u_int8_t msglen_t;

struct _BusMessage {
    struct _BusMessage *next; // for chaining buffers

    // if all three are zero the data is in the message
    // otherwise the header is authoritative, write to the message
    int8_t src; // -4…127
    int8_t dst; // -4…127
    u_int8_t code; // 0…3/31/255
    u_int8_t prio; // 0…WIRES-1

    u_int8_t *data;  // buffer. Byte zero: usage counter, if data_max>0.
    msglen_t data_max; // buffer length, bytes. Zero: external allocation
    msglen_t data_off; // Offset for content (header is before this)

    msglen_t data_pos; // current read position: byte offset
    msglen_t data_end; // current write position: byte offset
    u_int8_t data_pos_off; // current read position: non-filled bits
    u_int8_t data_end_off; // current write position: non-filled bits
    u_int8_t hdr_len; // Length of header. 0: need to call add_header / read_header
    u_int8_t frames; // for debugging
};
#define MSG_MAXHDR (3+1)  // first byte is a shared-buffer counter
#define MSG_MINBUF 30

typedef struct _BusMessage *BusMessage;

// Allocate an empty message
BusMessage msg_alloc(msglen_t maxlen);
// init a message allocated elsewhere. Requires MSG_MAXHDR free bytes in front!
void msg_init(BusMessage msg, u_int8_t *data, msglen_t len);
// copy an existing message (but not the data!)
BusMessage msg_copy(BusMessage orig);

// Free a message
void msg_free(BusMessage msg);
// Increase max buffer size
void msg_resize(BusMessage msg, msglen_t maxlen);
// dump message
const unsigned char* msg_info(BusMessage msg);
const unsigned char* msg_info_long(BusMessage msg);

// Add header bytes to the message
void msg_add_header(BusMessage msg);
// Move header data from the message to data fields
void msg_read_header(BusMessage msg);

// Start address of the message (data only)
u_int8_t *msg_start(BusMessage msg);
// Length of the message, excluding header (bytes)
msglen_t msg_length(BusMessage msg);
// Length of the complete message (bits)
u_int16_t msg_bits(BusMessage msg);
// Length of already-processed/transmitted message (bits)
u_int16_t msg_sent_bits(BusMessage msg);

// copy the first @off bits to a new message
BusMessage msg_copy_bits(BusMessage msg, u_int8_t off);

// sender

// prepare a buffer to add content to be transmitted
void msg_start_send(BusMessage msg);
// add bytes
void msg_add_data(BusMessage msg, const u_int8_t *data, msglen_t len);
// add single byte
static inline void msg_add_byte(BusMessage msg, const u_int8_t ch) {
    u_int8_t c = (ch);
    msg_add_data(msg,&c,1);
}
static inline void msg_add_16(BusMessage msg, const u_int16_t ch) {
    u_int8_t c;
    c = ch >> 8;
    msg_add_data(msg,&c,1);
    c = ch;
    msg_add_data(msg,&c,1);
}
static inline void msg_add_32(BusMessage msg, const u_int32_t ch) {
    u_int8_t c;
    c = ch >> 24;
    msg_add_data(msg,&c,1);
    c = ch >> 16;
    msg_add_data(msg,&c,1);
    c = ch >> 8;
    msg_add_data(msg,&c,1);
    c = ch;
    msg_add_data(msg,&c,1);
}

// prepare a buffer for sending
void msg_start_extract(BusMessage msg);
// are there more data to extract?
bool msg_extract_more(BusMessage msg);
// extract a frame_bits wide chunk.
// at the end of the message, fill with zeroes if <8 bits are missing
//    otherwise align to 8-bit, return |(1<<frame_bits)
u_int16_t msg_extract_chunk(BusMessage msg, u_int8_t frame_bits);

// receiver

// prepare a buffer for receiving
void msg_start_add(BusMessage msg);
// received @frame_bits of data
void msg_add_chunk(BusMessage msg, u_int16_t data, u_int8_t frame_bits);
// copy a chunk from transmitted message
void msg_add_in(BusMessage msg, BusMessage orig, u_int16_t bits);

// remove @bits of data from the end, return contents
u_int16_t msg_drop(BusMessage msg, u_int8_t frame_bits);
// remove residual bits
void msg_align(BusMessage msg);

// deprecated, only present for fakebus/test_handler_crc.c
// add zero filler plus 1-bit "added more than 8 bits" flag + CRC
void msg_fill_crc(BusMessage msg, u_int8_t frame_bits, u_int16_t crc, u_int8_t crc_bits);

#ifdef __cplusplus
}
#endif

#endif
