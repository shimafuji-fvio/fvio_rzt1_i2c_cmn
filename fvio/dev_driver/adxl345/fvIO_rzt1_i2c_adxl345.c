/***************************************************************************
 fvIO汎用プラグイン・ADXL354専用処理ソース           作成者:シマフジ電機(株)
 ***************************************************************************/

#include "platform.h"
#include "r_port.h"
#include "r_ecl_rzt1_if.h"
#include "r_mpc2.h"
#include "r_system.h"
#include "r_icu_init.h"
#include "string.h"

#include "r_ecl_rzt1_if.h"
#include "fvIO_if.h"
#include "fvIO_cmn_if.h"
#include "fvIO_rzt1_i2c_cmn.h"
#include "fvIO_rzt1_i2c_adxl345.h"

static void    adxl345_init( int32_t slot_id );
static int32_t adxl345_get_inf( ST_FVIO_IF_INFO *attr );
static int32_t adxl345_trg_stop( int32_t slot_id );
static int32_t adxl345_wr_ctrl_reg( int32_t slot_id, ST_FVIO_ADXL345_CREG *attr );
static int32_t adxl345_rd_ctrl_reg( int32_t slot_id, ST_FVIO_ADXL345_CREG *attr );
static int32_t adxl345_wr_dma( int32_t slot_id, ST_FVIO_ADXL345_DMA *attr );
static int32_t adxl345_set_cnf( int32_t slot_id, ST_FVIO_ADXL345_CNF *attr );
static int32_t adxl345_get_cnf( int32_t slot_id, ST_FVIO_ADXL345_CNF *attr );
static int32_t adxl345_set_int( int32_t slot_id, ST_FVIO_ADXL345_INT *attr );
static int32_t adxl345_get_bsy( int32_t slot_id );
static int32_t adxl345_dma_restart( int32_t slot_id );
static void    adxl345_isr_pae_func( int32_t slot_id );
static void    adxl345_isr_paf_func( int32_t slot_id );

typedef struct {
    ST_FVIO_ADXL345_CNF        TblfvIOCnf[FVIO_SLOT_NUM];        //config設定
    ST_FVIO_ADXL345_INT        TblfvIOInt[FVIO_SLOT_NUM];        //interrupt
    uint8_t                    TblPlugBusy[FVIO_SLOT_NUM];       //busyフラグ(DMA実行中)
}ST_FVIO_SSD1306_INFO;

ST_FVIO_SSD1306_INFO adxl345_inf;

ST_FVIO_IF_LIST fvio_i2c_adxl345_entry = {
        0x00100101,
        1,
        fvio_adxl345_assign,
        fvio_adxl345_unassign,
        fvio_adxl345_start,
        fvio_adxl345_stop,
        fvio_adxl345_write,
        fvio_adxl345_read,
        NULL,
};

/***************************************************************************
 * [名称]    :fvio_adxl345_assign
 * [機能]    :adxl345アサイン処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t slot_sz       スロットサイズ
 *            void **config          プラグインデータ
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_assign( int32_t slot_id, void **config, void *attr )
{
    int32_t ret, i, cre_id = 0;

    //ポート初期化(hi-z)
    fvio_i2c_cmn_init_port_hiz(slot_id);

    //プラグインデータの登録
    *config = (void *)fvio_i2c_cmn_config_tbl[slot_id];

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_adxl345_unassign
 * [機能]    :adxl345アンアサイン処理
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_unassign( int32_t slot_id )
{
    fvio_i2c_cmn_init_port_hiz(slot_id);
    return 0;
}

/***************************************************************************
 * [名称]    :fvio_adxl345_start
 * [機能]    :adxl345スタート処理
 * [引数]    :int32_t slot_id        スロットID
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_start( int32_t slot_id, void *attr )
{
    int32_t ret;

    //ポート初期化
    fvio_i2c_cmn_init_port(slot_id);

    //fvIOスタート
    ret = R_ECL_Start( 1<<slot_id, FVIO_I2_CMN_FREQ);
    if( ret != R_ECL_SUCCESS ){
        return ret;
    }

    //内部処理初期化
    adxl345_init( slot_id );

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_adxl345_stop
 * [機能]    :adxl345ストップ処理
 * [引数]    :int32_t slot_id        スロットID
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_stop( int32_t slot_id, void *attr )
{
    adxl345_trg_stop( slot_id );
    fvio_i2c_cmn_isr_paf[slot_id] = NULL ;    //割り込みは使用しない
    fvio_i2c_cmn_isr_pae[slot_id] = NULL ;    //割り込みは使用しない
    return R_ECL_Stop((0x01<<slot_id));
}

/***************************************************************************
 * [名称]    :fvio_adxl345_write
 * [機能]    :adxl345ライト処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t mode          writeモード
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_write( int32_t slot_id, uint32_t mode, void *attr )
{
    //stop
    if( mode == FVIO_ADXL345_MODE_STOP ){
        return adxl345_trg_stop( slot_id );
    //CTRL reg
    }else if( mode == FVIO_ADXL345_MODE_CREG ){
        return adxl345_wr_ctrl_reg( slot_id, attr );
    //DMA DATA reg
    }else if( mode == FVIO_ADXL345_MODE_DMA ){
        return adxl345_wr_dma( slot_id, attr );
    //cnf
    }else if( mode == FVIO_ADXL345_MODE_CNF ){
        return adxl345_set_cnf( slot_id, attr );
    //int
    }else if( mode == FVIO_ADXL345_MODE_INT ){
        return adxl345_set_int( slot_id, attr );
    //dma restart
    }else if( mode == FVIO_ADXL345_MODE_RDMA ){
        return adxl345_dma_restart( slot_id );
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_adxl345_read
 * [機能]    :adxl345リード処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t mode          readモード
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_adxl345_read( int32_t slot_id, uint32_t mode, void *attr )
{
    //inf
    if( mode == FVIO_ADXL345_MODE_INFO ){
        return adxl345_get_inf( attr );
    //CTRL reg
    }else if( mode == FVIO_ADXL345_MODE_CREG ){
        return adxl345_rd_ctrl_reg( slot_id, attr );
    //cnf
    }else if( mode == FVIO_ADXL345_MODE_CNF ){
        return adxl345_get_cnf( slot_id, attr );
    }else if( mode == FVIO_ADXL345_MODE_BSY ){
        return adxl345_get_bsy( slot_id );
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_init
 * [機能]    :adxl345初期化処理
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
static void adxl345_init( int32_t slot_id )
{
    //CONFIG設定初期化
    adxl345_inf.TblfvIOCnf[slot_id].cwait = 60;
    adxl345_inf.TblfvIOCnf[slot_id].lwait = 0;
    adxl345_inf.TblPlugBusy[slot_id]      = FVIO_I2C_DMA_STOP;

    //割り込み初期化
    fvio_i2c_cmn_isr_paf[slot_id] = adxl345_isr_paf_func;
    fvio_i2c_cmn_isr_pae[slot_id] = adxl345_isr_pae_func;

    fvio_i2c_cmn_init_int(slot_id, 1);
}

/***************************************************************************
 * [名称]    :adxl345_get_inf
 * [機能]    :adxl345プラグイン情報取得
 * [引数]    :ST_FVIO_IF_INFO *attr  fvioプラグイン情報
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_get_inf( ST_FVIO_IF_INFO *attr )
{
    attr->io_type = FVIO_ADXL345_INFO_TYPE;
    attr->in_sz   = FVIO_ADXL345_INFO_INSZ;
    attr->out_sz  = FVIO_ADXL345_INFO_OUTSZ;
    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_trg_stop
 * [機能]    :adxl345 fvIOシーケンス停止
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_trg_stop( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id ;
    uint8_t  dummy[16];

    fvio_i2c_cmn_stop(slot_id);                                           //TRG→STOP
    fvio_i2c_cmn_wait(slot_id);                                           //wait(lwait=maxなら約1[s])

    if(adxl345_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 2;    //DMA中断
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 2;    //DMA中断

        while( (*(volatile uint32_t*)(&DMA0.DMAC0_CHSTAT_0.LONG + ofst)) & 0x4 );
        while( (*(volatile uint32_t*)(&DMA0.DMAC0_CHSTAT_8.LONG + ofst)) & 0x4 );

        fvio_i2c_cmn_getfifo( slot_id, dummy, 16);                        //FIFO空読み
        adxl345_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_STOP;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_wr_ctrl_reg
 * [機能]    :adxl345 コントロールレジスタライト
 * [引数]    :int32_t slot_id            スロットID
 *            ST_FVIO_ADXL345_CREG *attr コントロールレジスタのr/w用構造体
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_wr_ctrl_reg( int32_t slot_id, ST_FVIO_ADXL345_CREG *attr )
{
    ST_FVIO_I2C_CMN_CMD para;

    //busyチェック
    if( adxl345_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        return -1;
    }

    para.trg       = FVIO_CMN_REG_TRG_TRG;
    para.slen      = attr->sz;
    para.rlen      = 0;
    para.sdata     = attr->sdata;
    para.dma_num   = 0;
    para.cwait     = adxl345_inf.TblfvIOCnf[slot_id].cwait;
    para.lwait     = adxl345_inf.TblfvIOCnf[slot_id].lwait;

    if( fvio_i2c_cmn_cmd( slot_id, FVIO_I2C_CMN_CMD_W1, &para ) != 0 ){
        return -1;
    }

    fvio_i2c_cmn_wait(slot_id);

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_rd_ctrl_reg
 * [機能]    :adxl345 コントロールレジスタリード
 * [引数]    :int32_t slot_id            スロットID
 *            ST_FVIO_ADXL345_CREG *attr コントロールレジスタのr/w用構造体
 * [返値]      :int32_t                  0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_rd_ctrl_reg( int32_t slot_id, ST_FVIO_ADXL345_CREG *attr )
{
    ST_FVIO_I2C_CMN_CMD para;

    //busyチェック
    if( adxl345_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        return -1;
    }

    para.trg       = FVIO_CMN_REG_TRG_TRG;
    para.slen      = 1;
    para.rlen      = attr->sz;
    para.sdata     = attr->sdata;
    para.dma_num   = 0;
    para.cwait     = adxl345_inf.TblfvIOCnf[slot_id].cwait;
    para.lwait     = adxl345_inf.TblfvIOCnf[slot_id].lwait;

    if( fvio_i2c_cmn_cmd( slot_id, FVIO_I2C_CMN_CMD_R1, &para ) != 0 ){
        return -1;
    }

    fvio_i2c_cmn_wait(slot_id);
    fvio_i2c_cmn_getreg( slot_id, attr->rdata, attr->sz);

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_wr_dma
 * [機能]    :adxl345 DMAライト
 * [引数]    :int32_t slot_id            スロットID
 *            ST_FVIO_ADXL345_DMA *attr  DMAのr/w用構造体
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_wr_dma( int32_t slot_id, ST_FVIO_ADXL345_DMA *attr )
{
    ST_FVIO_I2C_CMN_CMD para;
    uint8_t send_data[8];

    //busyチェック
    if( adxl345_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        return -1;
    }

    send_data[0]   = FVIO_ADXL345_SLVADR;
    send_data[1]   = FVIO_ADXL345_REG_DATAX0;

    para.trg       = FVIO_CMN_REG_TRG_TRG | attr->trg;
    para.slen      = 1;
    para.rlen      = attr->sz;
    para.sdata     = send_data;
    para.data1     = attr->data1;
    para.data2     = attr->data2;
    para.dma_num   = attr->num;
    para.cwait     = adxl345_inf.TblfvIOCnf[slot_id].cwait;
    para.lwait     = adxl345_inf.TblfvIOCnf[slot_id].lwait;

    if( attr->trg & (FVIO_CMN_REG_TRG_REP | FVIO_CMN_REG_TRG_SYNC) ){
        adxl345_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_LOOP_PAE;
    }else{
        adxl345_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_BUSY_PAE;
    }

    if( fvio_i2c_cmn_cmd( slot_id, FVIO_I2C_CMN_CMD_R2, &para ) != 0 ){
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_set_cnf
 * [機能]    :adxl345 cnfデータの設定
 * [引数]    :int32_t slot_id            スロットID
 *            ST_FVIO_ADXL345_CNF *attr  fvIO設定のr/w用構造体
 * [返値]    :int32_t                    0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_set_cnf( int32_t slot_id, ST_FVIO_ADXL345_CNF *attr )
{
    adxl345_inf.TblfvIOCnf[slot_id].cwait  = attr->cwait;
    adxl345_inf.TblfvIOCnf[slot_id].lwait  = attr->lwait;
    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_get_cnf
 * [機能]    :adxl345 cnfデータの取得
 * [引数]    :int32_t slot_id             スロットID
 *            ST_FVIO_ADXL345_CNF *attr   fvIO設定のr/w用構造体
 * [返値]    :int32_t                     0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_get_cnf( int32_t slot_id, ST_FVIO_ADXL345_CNF *attr )
{
    attr->cwait = adxl345_inf.TblfvIOCnf[slot_id].cwait;
    attr->lwait = adxl345_inf.TblfvIOCnf[slot_id].lwait;
    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_set_int
 * [機能]    :adxl345 割り込み関数の設定
 * [引数]    :int32_t slot_id              スロットID
 *            ST_FVIO_ADXL345_INT　*attr   ユーザー定義割り込み処理のr/w用構造体
 * [返値]    :int32_t                      0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_set_int( int32_t slot_id, ST_FVIO_ADXL345_INT *attr )
{
    adxl345_inf.TblfvIOInt[slot_id].paf_callback = attr->paf_callback;
    adxl345_inf.TblfvIOInt[slot_id].pae_callback = attr->pae_callback;
    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_get_bsy
 * [機能]    :adxl345 busyステータスの取得
 * [引数]    :int32_t slot_id              スロットID
 * [返値]    :int32_t                      FVIO_LOOP=連続実行中、FVIO_BUSY=単発実行中、FVIO_STOP=停止
 * [備考]    :
 ***************************************************************************/
static int32_t adxl345_get_bsy( int32_t slot_id )
{
    return adxl345_inf.TblPlugBusy[slot_id];
}

/***************************************************************************
 * [名称]    :adxl345_dma_restart
 * [機能]    :adxl345 dmaの再トリガ(pae)
 * [引数]    :int32_t slot_id              スロットID
 * [返値]    :
 * [備考]    :リピート or syncでトリガ実行した場合にDMAに対し、再トリガをかける
 ***************************************************************************/
static int32_t adxl345_dma_restart( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    if( ( adxl345_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_LOOP_PAE ) == FVIO_I2C_DMA_LOOP_PAE ){
        //次回のDMAを発行(PAE)
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 0x8;    //SWRST=1
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 1;      //SETEN=1
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :adxl345_isr_pae_func
 * [機能]    :adxl345 FIFO_PAE割り込み
 * [引数]    :int32_t slot_id              スロットID
 * [返値]    :なし
 * [備考]    :fvIO_rzt1_i2c_cmn.cの各割り込み処理(fvio_i2c_cmn_isr_paen)からコールされる
 ***************************************************************************/
static void adxl345_isr_pae_func( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    //コールバック
    if( adxl345_inf.TblfvIOInt[slot_id].pae_callback != NULL ){
        adxl345_inf.TblfvIOInt[slot_id].pae_callback(slot_id);
    }

    //busyフラグクリア( LOOPの場合はクリアしない )
    if( adxl345_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_BUSY_PAE ){
        adxl345_inf.TblPlugBusy[slot_id] &= ~FVIO_I2C_DMA_BUSY_PAE;
    }
}

/***************************************************************************
 * [名称]    :adxl345_isr_paf_func
 * [機能]    :adxl345 FIFO_PAF割り込み
 * [引数]    :int32_t slot_id              スロットID
 * [返値]    :なし
 * [備考]    :fvIO_rzt1_i2c_cmn.cの各割り込み処理(fvio_i2c_cmn_isr_pafn)からコールされる
 ***************************************************************************/
static void adxl345_isr_paf_func( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    //コールバック
    if( adxl345_inf.TblfvIOInt[slot_id].paf_callback != NULL ){
        adxl345_inf.TblfvIOInt[slot_id].paf_callback(slot_id);
    }

    //busyフラグクリア( LOOPの場合はクリアしない )
    if( adxl345_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_BUSY_PAF ){
        adxl345_inf.TblPlugBusy[slot_id] &= ~FVIO_I2C_DMA_BUSY_PAF;
    }
}

