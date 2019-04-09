/***************************************************************************
fvIO汎用プラグイン処理ソース                         作成者:シマフジ電機(株)
 ***************************************************************************/

#include "platform.h"
#include "r_port.h"
#include "r_mpc2.h"
#include "r_system.h"
#include "r_icu_init.h"

#include "r_ecl_rzt1_if.h"
#include "fvIO_if.h"
#include "fvIO_cmn_if.h"
#include "fvIO_rzt1_i2c_cmn.h"
#include "fvIO_rzt1_dma.h"


//センサ処理用割り込み処理ポインタ
void (*fvio_i2c_cmn_isr_pae[FVIO_SLOT_NUM])(int32_t) = {NULL};
void (*fvio_i2c_cmn_isr_paf[FVIO_SLOT_NUM])(int32_t) = {NULL};

//fvIO I/F レジスタアクセス用構造体
ST_FVIO_CMN_REGIF fvio_if[FVIO_SLOT_NUM] = {
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT0_REG, (uint8_t *)FVIO_IF_SLOT0_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT1_REG, (uint8_t *)FVIO_IF_SLOT1_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT2_REG, (uint8_t *)FVIO_IF_SLOT2_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT3_REG, (uint8_t *)FVIO_IF_SLOT3_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT4_REG, (uint8_t *)FVIO_IF_SLOT4_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT5_REG, (uint8_t *)FVIO_IF_SLOT5_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT6_REG, (uint8_t *)FVIO_IF_SLOT6_FIFO},
        {(struct st_fvio_cmn_out *)FVIO_IF_SLOT7_REG, (uint8_t *)FVIO_IF_SLOT7_FIFO},
};

//割り込み処理(paf)登録用配列
uint32_t fvio_isr_paf_adr[] = {
        (uint32_t)fvio_i2c_cmn_isr_paf0, (uint32_t)fvio_i2c_cmn_isr_paf1,
        (uint32_t)fvio_i2c_cmn_isr_paf2, (uint32_t)fvio_i2c_cmn_isr_paf3,
        (uint32_t)fvio_i2c_cmn_isr_paf4, (uint32_t)fvio_i2c_cmn_isr_paf5,
        (uint32_t)fvio_i2c_cmn_isr_paf6, (uint32_t)fvio_i2c_cmn_isr_paf7,
};

//割り込み処理(pae)登録用配列
uint32_t fvio_isr_pae_adr[] = {
        (uint32_t)fvio_i2c_cmn_isr_pae0, (uint32_t)fvio_i2c_cmn_isr_pae1,
        (uint32_t)fvio_i2c_cmn_isr_pae2, (uint32_t)fvio_i2c_cmn_isr_pae3,
        (uint32_t)fvio_i2c_cmn_isr_pae4, (uint32_t)fvio_i2c_cmn_isr_pae5,
        (uint32_t)fvio_i2c_cmn_isr_pae6, (uint32_t)fvio_i2c_cmn_isr_pae7,
};

extern uint32_t g_fvIO_rzt1_comn_i2c_0_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_1_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_2_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_3_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_4_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_5_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_6_config[];
extern uint32_t g_fvIO_rzt1_comn_i2c_7_config[];

//プラグインデータ(configデータ)
const void *fvio_i2c_cmn_config_tbl[FVIO_SLOT_NUM] = {
    g_fvIO_rzt1_comn_i2c_0_config,
    g_fvIO_rzt1_comn_i2c_1_config,
    g_fvIO_rzt1_comn_i2c_2_config,
    g_fvIO_rzt1_comn_i2c_3_config,
    g_fvIO_rzt1_comn_i2c_4_config,
    g_fvIO_rzt1_comn_i2c_5_config,
    g_fvIO_rzt1_comn_i2c_6_config,
    g_fvIO_rzt1_comn_i2c_7_config,
};

/***************************************************************************
 * [名称]    :fvio_init_port_hiz
 * [機能]    :fvIOポート初期化(ハイインピーダンス設定)
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_init_port_hiz( int32_t slot_id )
{
    switch( slot_id ){
        case 0x00:
            PORTD.PDR.BIT.B4 = PORT_DIRECTION_HIZ;
            PORTD.PDR.BIT.B3 = PORT_DIRECTION_HIZ;
            break;
        case 0x01:
            PORT5.PDR.BIT.B2 = PORT_DIRECTION_HIZ;
            PORT5.PDR.BIT.B1 = PORT_DIRECTION_HIZ;
            break;
        case 0x02:
            PORT1.PDR.BIT.B6 = PORT_DIRECTION_HIZ;
            PORT1.PDR.BIT.B5 = PORT_DIRECTION_HIZ;
            break;
        case 0x03:
            PORT1.PDR.BIT.B2 = PORT_DIRECTION_HIZ;
            PORT1.PDR.BIT.B1 = PORT_DIRECTION_HIZ;
            break;
        case 0x04:
            PORTR.PDR.BIT.B1 = PORT_DIRECTION_HIZ;
            PORTT.PDR.BIT.B0 = PORT_DIRECTION_HIZ;
            break;
        case 0x05:
            PORT9.PDR.BIT.B0 = PORT_DIRECTION_HIZ;
            PORTA.PDR.BIT.B7 = PORT_DIRECTION_HIZ;
            break;
        case 0x06:
            PORTA.PDR.BIT.B4 = PORT_DIRECTION_HIZ;
            PORTA.PDR.BIT.B3 = PORT_DIRECTION_HIZ;
            break;
        case 0x07:
            PORT7.PDR.BIT.B3 = PORT_DIRECTION_HIZ;
            PORT7.PDR.BIT.B2 = PORT_DIRECTION_HIZ;
            break;
    }
}

/***************************************************************************
 * [名称]    :fvio_init_port
 * [機能]    :fvIOポート初期化
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_init_port( int32_t slot_id )
{
    switch( slot_id ){
        case 0x00:
            R_MPC_WriteEnable();
            MPC.PD4PFS.BIT.PSEL = 0x2B;
            MPC.PD3PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORTD.PMR.BIT.B4 = PORT_MODE_PERIPHERAL;
            PORTD.PMR.BIT.B3 = PORT_MODE_PERIPHERAL;
            break;
        case 0x01:
            R_MPC_WriteEnable();
            MPC.P52PFS.BIT.PSEL = 0x2B;
            MPC.P51PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORT5.PMR.BIT.B2 = PORT_MODE_PERIPHERAL;
            PORT5.PMR.BIT.B1 = PORT_MODE_PERIPHERAL;
            break;
        case 0x02:
            R_MPC_WriteEnable();
            MPC.P16PFS.BIT.PSEL = 0x2B;
            MPC.P15PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORT1.PMR.BIT.B6 = PORT_MODE_PERIPHERAL;
            PORT1.PMR.BIT.B5 = PORT_MODE_PERIPHERAL;
            break;
        case 0x03:
            R_MPC_WriteEnable();
            MPC.P12PFS.BIT.PSEL = 0x2B;
            MPC.P11PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORT1.PMR.BIT.B2 = PORT_MODE_PERIPHERAL;
            PORT1.PMR.BIT.B1 = PORT_MODE_PERIPHERAL;
            break;
        case 0x04:
            R_MPC_WriteEnable();
            MPC.PR1PFS.BIT.PSEL = 0x2B;
            MPC.PT0PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORTR.PMR.BIT.B1 = PORT_MODE_PERIPHERAL;
            PORTT.PMR.BIT.B0 = PORT_MODE_PERIPHERAL;
            break;
        case 0x05:
            R_MPC_WriteEnable();
            MPC.P90PFS.BIT.PSEL = 0x2B;
            MPC.PA7PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORT9.PMR.BIT.B0 = PORT_MODE_PERIPHERAL;
            PORTA.PMR.BIT.B7 = PORT_MODE_PERIPHERAL;
            break;
        case 0x06:
            R_MPC_WriteEnable();
            MPC.PA4PFS.BIT.PSEL = 0x2B;
            MPC.PA3PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORTA.PMR.BIT.B4 = PORT_MODE_PERIPHERAL;
            PORTA.PMR.BIT.B3 = PORT_MODE_PERIPHERAL;
            break;
        case 0x07:
            R_MPC_WriteEnable();
            MPC.P73PFS.BIT.PSEL = 0x2B;
            MPC.P72PFS.BIT.PSEL = 0x2B;
            R_MPC_WriteDisable();
            PORT7.PMR.BIT.B3 = PORT_MODE_PERIPHERAL;
            PORT7.PMR.BIT.B2 = PORT_MODE_PERIPHERAL;
            break;
    }
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_cmd
 * [機能]    :fvIO汎用コマンド処理
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t cmd                コマンド番号
 *            ST_FVIO_I2C_CMN_CMD *attr  実装依存の引数
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_cmn_cmd( int32_t slot_id, uint8_t cmd, ST_FVIO_I2C_CMN_CMD *attr )
{
    uint8_t i;
    uint8_t *input, inc;

    //コマンド番号チェック
    if( cmd >= FVIO_I2C_CMN_CMD_NUM ){
        return -1;
    //スロットIDチェック
    }else if( slot_id >= FVIO_SLOT_NUM ){
        return -1;
    //送信データ数チェック
    }else if( attr->slen > 7 ){
        return -1;
    //送信データ数チェック(リード)
    }else if( ( attr->slen > 6 ) && ( cmd <= FVIO_I2C_CMN_CMD_R3 ) ){
        return -1;
    //受信データ数チェック
    }else if( attr->rlen > 7 ){
        return -1;
    }

    fvio_if[slot_id].reg->TRG    = 0;                            //停止
    fvio_if[slot_id].reg->CMD    = cmd;                          //コマンド
    fvio_if[slot_id].reg->CWAIT  = attr->cwait;                  //クロック周期
    fvio_if[slot_id].reg->LWAIT  = attr->lwait & 0xFFFFFF;       //トリガ受付無効期間
    fvio_if[slot_id].reg->SLEN   = attr->slen;                   //送信サイズ
    fvio_if[slot_id].reg->RLEN   = attr->rlen;                   //受信サイズ

    //通常動作, DMA動作(コマンドR2)
    if( attr->dma_num == 0 || ( (attr->dma_num > 0) && ( cmd == FVIO_I2C_CMN_CMD_R2) ) ){
        //fifo入力
        if( ( cmd == FVIO_I2C_CMN_CMD_R3 ) || ( cmd == FVIO_I2C_CMN_CMD_W2 ) ){
            input = fvio_if[slot_id].fifo;
            inc   = 0;
        //レジスタ入力
        }else{
            input = &fvio_if[slot_id].reg->IREG0;
            inc   = 1;
        }

        //slave addr(write)入力
        *input = attr->sdata[0] & 0xfe;
        input += inc;

        //data入力
        for( i = 0 ; i < (attr->slen) ; i++ ){
            *input = attr->sdata[i+1];
            input += inc;
        }

        //slave addr(read)入力
        if( ( cmd == FVIO_I2C_CMN_CMD_R1 ) || ( cmd == FVIO_I2C_CMN_CMD_R2 ) || ( cmd == FVIO_I2C_CMN_CMD_R3 ) ){
            *input = (attr->sdata[0] | 1 ) ;
        }

        if( attr->dma_num > 0 ){
            dma_init_r( slot_id, fvio_if[slot_id].fifo, attr->data1, attr->data2, (attr->rlen+1)*attr->dma_num );
        }
    //DMA動作(コマンドR3)
    }else if( cmd == FVIO_I2C_CMN_CMD_R3 ){
        dma_init_s( slot_id, attr->data1, attr->data2, fvio_if[slot_id].fifo, (attr->slen+2)*attr->dma_num );
        dma_init_r( slot_id, fvio_if[slot_id].fifo, attr->data1, attr->data2, (attr->rlen+1)*attr->dma_num );
    //DMA動作(コマンドW2)
    }else if( cmd == FVIO_I2C_CMN_CMD_W2 ){
        dma_init_s( slot_id, attr->data1, attr->data2, fvio_if[slot_id].fifo, (attr->slen+1)*attr->dma_num );
    //それ以外
    }else {
        return -1;
    }

    //開始
    fvio_if[slot_id].reg->TRG    = FVIO_CMN_REG_TRG_TRG | attr->trg;

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_stop
 * [機能]    :fvIO汎用ストップ処理
 * [引数]    :int32_t slot_id            スロットID
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_cmn_stop( int32_t slot_id )
{
    fvio_if[slot_id].reg->TRG = FVIO_CMN_REG_TRG_FCLR;
    return 0;
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_wait
 * [機能]    :fvIO汎用ウェイト処理
 * [引数]    :int32_t slot_id            スロットID
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :リピート,sync,dmaを使用する処理では使用しないこと
 ***************************************************************************/
void fvio_i2c_cmn_wait( int32_t slot_id )
{
    uint8_t i;

    while( i < 20 ){
        if( fvio_if[slot_id].reg->STAT & FVIO_CMN_REG_STAT_TRDY ){
            i++;
        }else {
            i=0;
        }
    }
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_getreg
 * [機能]    :fvIO データ取得(IREG)
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t *rdata             取得データ
 *            uint8_t sz                 取得サイズ
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :リピート,sync,dmaを使用する処理では使用しないこと
 ***************************************************************************/
int32_t fvio_i2c_cmn_getreg( int32_t slot_id, uint8_t *rdata, uint8_t sz)
{
    int32_t i;
    uint8_t *output;

    output = &fvio_if[slot_id].reg->OREG0;

    for( i = sz ; i >= 0 ; i-- ){
        rdata[sz-i] = *(output + i);
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_getfifo
 * [機能]    :fvIO データ取得(FIFO)
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t *rdata             取得データ
 *            uint8_t sz                 取得サイズ
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :リピート,sync,dmaを使用する処理では使用しないこと
 ***************************************************************************/
int32_t fvio_i2c_cmn_getfifo( int32_t slot_id, uint8_t *rdata, uint8_t sz)
{
    int32_t i;

    for( i = 0 ; i <= sz ; i++ ){
        rdata[i] = (*fvio_if[slot_id].fifo);
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_init_int
 * [機能]    :DMA設定( fvio fifo → cpu mem )
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t *rdata1            バッファ1
 *            uint8_t *rdata2            バッファ2
 *            uint32_t sz                DMA転送サイズ
 * [返値]    :なし
 * [備考]    :バッファ1,バッファ2はダブルバッファとして使用する。
 *            転送が完了した場合、DMAデスティネーションはバッファ1からバッファ2へ切り替わる
 ***************************************************************************/
void fvio_i2c_cmn_init_int(int32_t slot_id, uint32_t mode)
{
    int32_t paf_vect = FVIO_IF_SLOT0_VECT_PAF+(slot_id<<1);
    int32_t pae_vect = FVIO_IF_SLOT0_VECT_PAE+(slot_id<<1);

    asm("cpsid i");
    asm("isb");
    R_ICU_Disable(paf_vect);
    R_ICU_Disable(pae_vect);

    if( mode == 1 ){
        R_ICU_Regist( paf_vect, ICU_TYPE_EDGE, FVIO_ICU_PRI, fvio_isr_paf_adr[slot_id]);
        R_ICU_Regist( pae_vect, ICU_TYPE_EDGE, FVIO_ICU_PRI, fvio_isr_pae_adr[slot_id]);
        R_ICU_Enable(paf_vect);
        R_ICU_Enable(pae_vect);
    }
    asm("cpsie i");
    asm("isb");
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf0
 * [機能]    :スロット0 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf0(void)
{
    VIC.PIC4.BIT.PIC129 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT0] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT0])(FVIO_IF_SLOT0);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf1
 * [機能]    :スロット1 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf1(void)
{
    VIC.PIC4.BIT.PIC131 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT1] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT1])(FVIO_IF_SLOT1);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf2
 * [機能]    :スロット2 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf2(void)
{
    VIC.PIC4.BIT.PIC133 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT2] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT2])(FVIO_IF_SLOT2);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf3
 * [機能]    :スロット3 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf3(void)
{
    VIC.PIC4.BIT.PIC135 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT3] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT3])(FVIO_IF_SLOT3);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf4
 * [機能]    :スロット4 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf4(void)
{
    VIC.PIC4.BIT.PIC137 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT4] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT4])(FVIO_IF_SLOT4);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf5
 * [機能]    :スロット5 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf5(void)
{
    VIC.PIC4.BIT.PIC139 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT5] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT5])(FVIO_IF_SLOT5);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf6
 * [機能]    :スロット6 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf6(void)
{
    VIC.PIC4.BIT.PIC141 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT6] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT6])(FVIO_IF_SLOT6);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_paf7
 * [機能]    :スロット7 fifo paf割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_paf7(void)
{
    VIC.PIC4.BIT.PIC143 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT7] != NULL ){
        (*fvio_i2c_cmn_isr_paf[FVIO_IF_SLOT7])(FVIO_IF_SLOT7);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae0
 * [機能]    :スロット0 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae0(void)
{
    VIC.PIC4.BIT.PIC130 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT0] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT0])(FVIO_IF_SLOT0);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae1
 * [機能]    :スロット1 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae1(void)
{
    VIC.PIC4.BIT.PIC132 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT1] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT1])(FVIO_IF_SLOT1);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae2
 * [機能]    :スロット2 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae2(void)
{
    VIC.PIC4.BIT.PIC134 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT2] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT2])(FVIO_IF_SLOT2);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae3
 * [機能]    :スロット3 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae3(void)
{
    VIC.PIC4.BIT.PIC136 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT3] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT3])(FVIO_IF_SLOT3);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae4
 * [機能]    :スロット4 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae4(void)
{
    VIC.PIC4.BIT.PIC138 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT4] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT4])(FVIO_IF_SLOT4);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae5
 * [機能]    :スロット5 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae5(void)
{
    VIC.PIC4.BIT.PIC140 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT5] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT5])(FVIO_IF_SLOT5);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae6
 * [機能]    :スロット6 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae6(void)
{
    VIC.PIC4.BIT.PIC142 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT6] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT6])(FVIO_IF_SLOT6);
    }
    HVA0_DUMMY_WRITE();
}

/***************************************************************************
 * [名称]    :fvio_i2c_cmn_isr_pae7
 * [機能]    :スロット7 fifo pae割り込み
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_i2c_cmn_isr_pae7(void)
{
    VIC.PIC4.BIT.PIC144 = ICU_PIC_EDGE_CLEAR;
    if( fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT7] != NULL ){
        (*fvio_i2c_cmn_isr_pae[FVIO_IF_SLOT7])(FVIO_IF_SLOT7);
    }
    HVA0_DUMMY_WRITE();
}
