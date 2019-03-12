/***************************************************************************
fvIO汎用プラグイン共通ヘッダ                         作成者:シマフジ電機(株)
 ***************************************************************************/
#ifndef __FVIO_CMN_IF_H
#define __FVIO_CMN_IF_H

#include "platform.h"

//汎用fvIOレジスタ
struct st_fvio_cmn_out{
      uint8_t                     /* 0x0011C100 */ TRG;
      uint8_t                     /* 0x0011C101 */ CMD;
      uint8_t                     /* 0x0011C102 */ SLEN;
      uint8_t                     /* 0x0011C103 */ RLEN;
      uint8_t                     /* 0x0011C104 */ CWAIT;
      uint8_t                     /* 0x0011C105 */ OPT0;
      uint8_t                     /* 0x0011C106 */ OPT1;
      uint8_t                     /* 0x0011C107 */ OPT2;
      uint32_t                    /* 0x0011C108 */ LWAIT;
      uint8_t  dummy332[       4];
      uint8_t                     /* 0x0011C110 */ IREG0;
      uint8_t                     /* 0x0011C111 */ IREG1;
      uint8_t                     /* 0x0011C112 */ IREG2;
      uint8_t                     /* 0x0011C113 */ IREG3;
      uint8_t                     /* 0x0011C114 */ IREG4;
      uint8_t                     /* 0x0011C115 */ IREG5;
      uint8_t                     /* 0x0011C116 */ IREG6;
      uint8_t                     /* 0x0011C117 */ IREG7;
      uint8_t                     /* 0x0011C118 */ IREG8;
      uint8_t                     /* 0x0011C119 */ IREG9;
      uint8_t                     /* 0x0011C11A */ IREGA;
      uint8_t                     /* 0x0011C11B */ IREGB;
      uint8_t                     /* 0x0011C11C */ IREGC;
      uint8_t                     /* 0x0011C11D */ IREGD;
      uint8_t                     /* 0x0011C11E */ IREGE;
      uint8_t                     /* 0x0011C11F */ IREGF;
      uint8_t  dummy000[   0x1e0];
      uint8_t  dummy416[      32];
      uint8_t  dummy003[   0x1e0];
      uint8_t  dummy352[      32];
      uint8_t  dummy001[   0x1e0];
      uint8_t                     /* 0x0011C700 */ VER;
      uint8_t                     /* 0x0011C701 */ STAT;
      uint8_t                     /* 0x0011C702 */ RSLT;
      uint8_t  dummy451[      13];
      uint8_t                     /* 0x0011C710 */ OREG0;
      uint8_t                     /* 0x0011C711 */ OREG1;
      uint8_t                     /* 0x0011C712 */ OREG2;
      uint8_t                     /* 0x0011C713 */ OREG3;
      uint8_t                     /* 0x0011C714 */ OREG4;
      uint8_t                     /* 0x0011C715 */ OREG5;
      uint8_t                     /* 0x0011C716 */ OREG6;
      uint8_t                     /* 0x0011C717 */ OREG7;
      uint8_t                     /* 0x0011C718 */ OREG8;
      uint8_t                     /* 0x0011C719 */ OREG9;
      uint8_t                     /* 0x0011C71A */ OREGA;
      uint8_t                     /* 0x0011C71B */ OREGB;
      uint8_t                     /* 0x0011C71C */ OREGC;
      uint8_t                     /* 0x0011C71D */ OREGD;
      uint8_t                     /* 0x0011C71E */ OREGE;
      uint8_t                     /* 0x0011C71F */ OREGF;
      uint8_t  dummy004[   0x5e0];
      uint8_t  dummy384[      32];
      uint8_t  dummy002[   0x1e0];
      uint8_t  dummy480[      32];
};

//fvIO インタフェースアクセス用構造体
typedef struct {
    struct st_fvio_cmn_out *reg;   //fvIO レジスタ構造体
    unsigned char *fifo;           //FIFOレジスタ
} ST_FVIO_CMN_REGIF;

//TRG reg
#define FVIO_CMN_REG_TRG_FCLR        (0x80)
#define FVIO_CMN_REG_TRG_SYNC        (0x08)
#define FVIO_CMN_REG_TRG_INT         (0x04)
#define FVIO_CMN_REG_TRG_REP         (0x02)
#define FVIO_CMN_REG_TRG_TRG         (0x01)

//STAT reg
#define FVIO_CMN_REG_STAT_FERR       (0x80)
#define FVIO_CMN_REG_STAT_TRDY       (0x01)

#endif

