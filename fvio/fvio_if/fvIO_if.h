/***************************************************************************
fvIO I/Fヘッダ                                       作成者:シマフジ電機(株)
 ***************************************************************************/
#ifndef _FVIO_IF_H_
#define _FVIO_IF_H_

#include "platform.h"

//スロット総数
#define FVIO_SLOT_NUM             (8)

//app_if no.
#define FVIO_IF_SLOT0             (0)
#define FVIO_IF_SLOT1             (1)
#define FVIO_IF_SLOT2             (2)
#define FVIO_IF_SLOT3             (3)
#define FVIO_IF_SLOT4             (4)
#define FVIO_IF_SLOT5             (5)
#define FVIO_IF_SLOT6             (6)
#define FVIO_IF_SLOT7             (7)

//fvio reg.
#define FVIO_IF_SLOT0_REG         (0xB011C100)
#define FVIO_IF_SLOT1_REG         (0xb051c100)
#define FVIO_IF_SLOT2_REG         (0xb091c100)
#define FVIO_IF_SLOT3_REG         (0xb0d1c100)
#define FVIO_IF_SLOT4_REG         (0xb031c100)
#define FVIO_IF_SLOT5_REG         (0xb071c100)
#define FVIO_IF_SLOT6_REG         (0xb0b1c100)
#define FVIO_IF_SLOT7_REG         (0xb0f1c100)

//fvio FIFO addr
#define FVIO_IF_SLOT0_FIFO        (0xB0FF4000)
#define FVIO_IF_SLOT1_FIFO        (0xB0FF4800)
#define FVIO_IF_SLOT2_FIFO        (0xB0FF5000)
#define FVIO_IF_SLOT3_FIFO        (0xB0FF5800)
#define FVIO_IF_SLOT4_FIFO        (0xB0FF6000)
#define FVIO_IF_SLOT5_FIFO        (0xB0FF6800)
#define FVIO_IF_SLOT6_FIFO        (0xB0FF7000)
#define FVIO_IF_SLOT7_FIFO        (0xB0FF7800)

//fvio vector( cpu mem →fvio fifo )
#define FVIO_IF_SLOT0_VECT_PAF    (129)
#define FVIO_IF_SLOT1_VECT_PAF    (131)
#define FVIO_IF_SLOT2_VECT_PAF    (133)
#define FVIO_IF_SLOT3_VECT_PAF    (135)
#define FVIO_IF_SLOT4_VECT_PAF    (137)
#define FVIO_IF_SLOT5_VECT_PAF    (139)
#define FVIO_IF_SLOT6_VECT_PAF    (141)
#define FVIO_IF_SLOT7_VECT_PAF    (143)

//fvio vector( fvio fifo → cpu mem)
#define FVIO_IF_SLOT0_VECT_PAE    (130)
#define FVIO_IF_SLOT1_VECT_PAE    (132)
#define FVIO_IF_SLOT2_VECT_PAE    (134)
#define FVIO_IF_SLOT3_VECT_PAE    (136)
#define FVIO_IF_SLOT4_VECT_PAE    (138)
#define FVIO_IF_SLOT5_VECT_PAE    (140)
#define FVIO_IF_SLOT6_VECT_PAE    (142)
#define FVIO_IF_SLOT7_VECT_PAE    (144)

//fvioプラグイン情報(io_type)
#define FVIO_IF_INFO_TYPE_I       (0x01)
#define FVIO_IF_INFO_TYPE_O       (0x02)
#define FVIO_IF_INFO_TYPE_IO      (0x03)

//fvIO関数のi/f構造体
typedef struct fvio_opn_lst{
    int32_t fvio_id;                                  //fvio id
    uint32_t slot_sz;                                 //slot size
    int32_t (*assign)(int32_t, void**, void*);        //assign func
    int32_t (*unassign)(int32_t);                     //unassign func
    int32_t (*start)(int32_t, void*);                 //system start
    int32_t (*stop)(int32_t, void*);                  //stop
    int32_t (*write)(int32_t, uint32_t, void*);       //write func
    int32_t (*read)(int32_t, uint32_t, void*);        //read func
    struct fvio_opn_lst *next_list;                   //next pointer
}ST_FVIO_IF_LIST;

//fvioプラグイン情報
typedef struct {
    uint8_t  io_type;                                 //io type
    uint32_t in_sz;                                   //input size
    uint32_t out_sz;                                  //output size
}ST_FVIO_IF_INFO;

void    fvio_sys_init( void );
int32_t fvio_entry( ST_FVIO_IF_LIST *new_entry, uint32_t mode, int32_t *fvio_id );
int32_t fvio_release( int32_t fvio_id );
int32_t fvio_assign( int32_t fvio_id, int32_t slot_id, int32_t *plug_id, void *attr );
int32_t fvio_unassign( int32_t plug_id );
int32_t fvio_sys_start( int32_t *result, void *attr );
int32_t fvio_stop( int32_t slot_id, void *attr );
int32_t fvio_write( int32_t plug_id, uint32_t mode, void *attr );
int32_t fvio_read( int32_t plug_id, uint32_t mode, void *attr );

#endif
