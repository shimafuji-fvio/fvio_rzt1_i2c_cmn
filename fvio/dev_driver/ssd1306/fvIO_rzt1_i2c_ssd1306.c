/***************************************************************************
　fvIO汎用プラグイン・SSD1306専用処理ソース          作成者:シマフジ電機(株)
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
#include "fvIO_rzt1_i2c_ssd1306.h"

static void    fvio_i2c_ssd1306_init( int32_t slot_id );
static int32_t fvio_i2c_ssd1306_get_inf( ST_FVIO_IF_INFO *attr );
static int32_t fvio_i2c_ssd1306_trg_stop( int32_t slot_id );
static int32_t fvio_i2c_ssd1306_wr_ctrl_reg( int32_t slot_id, ST_FVIO_I2C_SSD1306_CREG *attr );
static int32_t fvio_i2c_ssd1306_wr_dma( int32_t slot_id, ST_FVIO_I2C_SSD1306_DMA *attr );
static int32_t fvio_i2c_ssd1306_set_cnf( int32_t slot_id, ST_FVIO_I2C_SSD1306_CNF *attr );
static int32_t fvio_i2c_ssd1306_get_cnf( int32_t slot_id, ST_FVIO_I2C_SSD1306_CNF *attr );
static int32_t fvio_i2c_ssd1306_set_int( int32_t slot_id, ST_FVIO_I2C_SSD1306_INT *attr );
static int32_t fvio_i2c_ssd1306_get_bsy( int32_t slot_id );
static int32_t fvio_i2c_ssd1306_dma_restart( int32_t slot_id );
static void    fvio_i2c_ssd1306_isr_pae_func( int32_t slot_id );
static void    fvio_i2c_ssd1306_isr_paf_func( int32_t slot_id );

typedef struct {
    ST_FVIO_I2C_SSD1306_CNF    TblfvIOCnf[FVIO_SLOT_NUM];        //config設定
    ST_FVIO_I2C_SSD1306_INT    TblfvIOInt[FVIO_SLOT_NUM];        //interrupt
    uint8_t                    TblPlugBusy[FVIO_SLOT_NUM];       //busyフラグ(DMA実行中)
}ST_FVIO_SSD1306_INFO;

ST_FVIO_SSD1306_INFO ssd1306_inf;

ST_FVIO_IF_LIST fvio_i2c_ssd1306_entry = {
        0x00100201,
        1,
        fvio_i2c_ssd1306_assign,
        fvio_i2c_ssd1306_unassign,
        fvio_i2c_ssd1306_start,
        fvio_i2c_ssd1306_stop,
        fvio_i2c_ssd1306_write,
        fvio_i2c_ssd1306_read,
        NULL,
};

/***************************************************************************
 * [名称]    :fvio_ssd1306_assign
 * [機能]    :ssd1306アサイン処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t slot_sz       スロットサイズ
 *            void **config          プラグインデータ
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_assign( int32_t slot_id, void **config, void *attr )
{
    int32_t ret, i, cre_id = 0;

    //ポート初期化(hi-z)
    fvio_i2c_cmn_init_port_hiz(slot_id);

    //プラグインデータの登録
    *config = (void *)fvio_i2c_cmn_config_tbl[slot_id];

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_ssd1306_assign
 * [機能]    :ssd1306アンアサイン処理
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_unassign( int32_t slot_id )
{
    fvio_i2c_cmn_init_port_hiz(slot_id);
    return 0;
}

/***************************************************************************
 * [名称]    :fvio_ssd1306_start
 * [機能]    :ssd1306スタート処理
 * [引数]    :int32_t slot_id        スロットID
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_start( int32_t slot_id, void *attr )
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
    fvio_i2c_ssd1306_init( slot_id );

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_ssd1306_stop
 * [機能]    :ssd1306ストップ処理
 * [引数]    :int32_t slot_id        スロットID
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_stop( int32_t slot_id, void *attr )
{
	fvio_i2c_ssd1306_trg_stop( slot_id );

    fvio_i2c_cmn_init_int(slot_id, 0);
    fvio_i2c_cmn_isr_paf[slot_id] = NULL ;    //割り込みは使用しない
    fvio_i2c_cmn_isr_pae[slot_id] = NULL ;    //割り込みは使用しない

    return R_ECL_Stop((0x01<<slot_id));
}

/***************************************************************************
 * [名称]    :fvio_ssd1306_write
 * [機能]    :ssd1306ライト処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t mode          writeモード
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_write( int32_t slot_id, uint32_t mode, void *attr )
{
    //stop
    if( mode == FVIO_I2C_SSD1306_MODE_STOP ){
        return fvio_i2c_ssd1306_trg_stop( slot_id );
    //ctrl reg
    }else if( mode == FVIO_I2C_SSD1306_MODE_CREG ){
        return fvio_i2c_ssd1306_wr_ctrl_reg( slot_id, attr );
    //dma
    }else if( mode == FVIO_I2C_SSD1306_MODE_DMA ){
        return fvio_i2c_ssd1306_wr_dma( slot_id, attr );
    //cnf
    }else if( mode == FVIO_I2C_SSD1306_MODE_CNF ){
        return fvio_i2c_ssd1306_set_cnf( slot_id, attr );
    //int
    }else if( mode == FVIO_I2C_SSD1306_MODE_INT ){
        return fvio_i2c_ssd1306_set_int( slot_id, attr );
    //dma restart
    }else if( mode == FVIO_I2C_SSD1306_MODE_RDMA ){
        return fvio_i2c_ssd1306_dma_restart( slot_id );
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_ssd1306_read
 * [機能]    :ssd1306リード処理
 * [引数]    :int32_t slot_id        スロットID
 *            uint32_t mode          readモード
 *            void *attr             実装依存の引数
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_i2c_ssd1306_read( int32_t slot_id, uint32_t mode, void *attr )
{
    //inf
    if( mode == FVIO_I2C_SSD1306_MODE_INFO ){
        return fvio_i2c_ssd1306_get_inf( attr );
    //cnf
    }else if( mode == FVIO_I2C_SSD1306_MODE_CNF ){
        return fvio_i2c_ssd1306_get_cnf( slot_id, attr );
    //busy
    }else if( mode == FVIO_I2C_SSD1306_MODE_BSY ){
        return fvio_i2c_ssd1306_get_bsy( slot_id );
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_init
 * [機能]    :ssd1306初期化処理
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
static void fvio_i2c_ssd1306_init( int32_t slot_id )
{
    int32_t i;

    //CONFIG設定初期化
    ssd1306_inf.TblfvIOCnf[slot_id].cwait = 40;
    ssd1306_inf.TblfvIOCnf[slot_id].lwait = 0;
    ssd1306_inf.TblPlugBusy[slot_id]      = FVIO_I2C_DMA_STOP;

    //割り込み初期化
    fvio_i2c_cmn_isr_paf[slot_id] = fvio_i2c_ssd1306_isr_paf_func;
    fvio_i2c_cmn_isr_pae[slot_id] = fvio_i2c_ssd1306_isr_pae_func;

    fvio_i2c_cmn_init_int(slot_id, 1);
}

/***************************************************************************
 * [名称]    :ssd1306_get_inf
 * [機能]    :ssd1306プラグイン情報取得
 * [引数]    :ST_FVIO_IF_INFO *attr  fvioプラグイン情報
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_get_inf( ST_FVIO_IF_INFO *attr )
{
    attr->io_type = FVIO_I2C_SSD1306_INFO_TYPE;
    attr->in_sz   = FVIO_I2C_SSD1306_INFO_INSZ;
    attr->out_sz  = FVIO_I2C_SSD1306_INFO_OUTSZ;
    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_trg_stop
 * [機能]    :ssd1306 fvIOシーケンス停止
 * [引数]    :int32_t slot_id        スロットID
 * [返値]    :int32_t                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_trg_stop( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id ;
    uint8_t  dummy[16];

    fvio_i2c_cmn_stop(slot_id);                                           //TRG→STOP
    fvio_i2c_cmn_wait(slot_id);                                           //wait(lwait=maxなら約1[s])

    if(ssd1306_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 2;    //DMA中断（CLREN=1)
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 2;    //DMA中断（CLREN=1)

        while( (*(volatile uint32_t*)(&DMA0.DMAC0_CHSTAT_0.LONG + ofst)) & 0x4 );
        while( (*(volatile uint32_t*)(&DMA0.DMAC0_CHSTAT_8.LONG + ofst)) & 0x4 );
        fvio_i2c_cmn_getfifo( slot_id, dummy, 16);                        //FIFO空読み
        ssd1306_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_STOP;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_wr_cmd
 * [機能]    :ssd1306 コントロールレジスタライト
 * [引数]    :int32_t slot_id                        スロットID
 *            ST_FVIO_SSD1306_CREG *attr             コントロールレジスタのr/w用構造体
 * [返値]    :int32_t                                0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_wr_ctrl_reg( int32_t slot_id, ST_FVIO_I2C_SSD1306_CREG *attr )
{
    ST_FVIO_I2C_CMN_CMD para;

    //busyチェック
    if( ssd1306_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        return -1;
    }

    para.trg       = FVIO_CMN_REG_TRG_TRG;
    para.slen      = attr->sz;
    para.rlen      = 0;
    para.sdata     = attr->sdata;
    para.dma_num   = 0;
    para.cwait     = ssd1306_inf.TblfvIOCnf[slot_id].cwait;
    para.lwait     = ssd1306_inf.TblfvIOCnf[slot_id].lwait;

    if( fvio_i2c_cmn_cmd( slot_id, FVIO_I2C_CMN_CMD_W1, &para ) != 0 ){
        return -1;
    }

    fvio_i2c_cmn_wait(slot_id);

    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_wr_dma
 * [機能]    :ssd1306　DMAライト
 * [引数]    :int32_t slot_id             スロットID
 *            ST_FVIO_SSD1306_DMA *attr   DMA通信のr/w用構造体
 * [返値]    :int32_t                     0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_wr_dma( int32_t slot_id, ST_FVIO_I2C_SSD1306_DMA *attr )
{
    ST_FVIO_I2C_CMN_CMD para;

    //busyチェック
    if( ssd1306_inf.TblPlugBusy[slot_id] != FVIO_I2C_DMA_STOP ){
        return -1;
    }

    para.trg       = FVIO_CMN_REG_TRG_TRG | attr->trg;
    para.slen      = attr->sz;
    para.rlen      = 0;
    para.data1     = attr->data1;
    para.data2     = attr->data2;
    para.dma_num   = attr->num;
    para.cwait     = ssd1306_inf.TblfvIOCnf[slot_id].cwait;
    para.lwait     = ssd1306_inf.TblfvIOCnf[slot_id].lwait;

    if( attr->trg & (FVIO_CMN_REG_TRG_REP | FVIO_CMN_REG_TRG_SYNC) ){
        ssd1306_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_LOOP_PAF;
    }else{
        ssd1306_inf.TblPlugBusy[slot_id] = FVIO_I2C_DMA_BUSY_PAF;
    }

    if( fvio_i2c_cmn_cmd( slot_id, FVIO_I2C_CMN_CMD_W2, &para ) != 0 ){
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_set_cnf
 * [機能]    :ssd1306 cnfデータの設定
 * [引数]    :int32_t slot_id             スロットID
 *            ST_FVIO_SSD1306_CNF *attr   fvIO設定のr/w用構造体
 * [返値]    :int32_t                     0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_set_cnf( int32_t slot_id, ST_FVIO_I2C_SSD1306_CNF *attr )
{
    ssd1306_inf.TblfvIOCnf[slot_id].cwait  = attr->cwait;
    ssd1306_inf.TblfvIOCnf[slot_id].lwait  = attr->lwait;
    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_get_cnf
 * [機能]    :ssd1306 cnfデータの取得
 * [引数]    :int32_t slot_id             スロットID
 *            ST_FVIO_SSD1306_CNF *attr   fvIO設定のr/w用構造体
 * [返値]    :int32_t                     0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_get_cnf( int32_t slot_id, ST_FVIO_I2C_SSD1306_CNF *attr )
{
    attr->cwait = ssd1306_inf.TblfvIOCnf[slot_id].cwait;
    attr->lwait = ssd1306_inf.TblfvIOCnf[slot_id].lwait;
    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_set_int
 * [機能]    :ssd1306 割り込み関数の設定
 * [引数]    :int32_t slot_id             スロットID
 *            ST_FVIO_SSD1306_INT *attr   ユーザー定義割り込みのr/w用構造体
 * [返値]    :int32_t                     0=正常、0以外=異常
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_set_int( int32_t slot_id, ST_FVIO_I2C_SSD1306_INT *attr )
{
    ssd1306_inf.TblfvIOInt[slot_id].paf_callback = attr->paf_callback;
    ssd1306_inf.TblfvIOInt[slot_id].pae_callback = attr->pae_callback;
    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_get_bsy
 * [機能]    :ssd1306 busyステータスの取得
 * [引数]    :int32_t slot_id            スロットID
 * [返値]    :int32_t                    FVIO_LOOP=連続実行中、FVIO_BUSY=単発実行中、FVIO_STOP=停止
 * [備考]    :
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_get_bsy( int32_t slot_id )
{
    return ssd1306_inf.TblPlugBusy[slot_id];
}

/***************************************************************************
 * [名称]    :ssd1306_dma_restart_paf
 * [機能]    :ssd1306 dmaの再トリガ(paf)
 * [引数]    :int32_t slot_id           スロットID
 * [返値]    :
 * [備考]    :リピート or syncでトリガ実行した場合にDMAに対し、再トリガをかける
 ***************************************************************************/
static int32_t fvio_i2c_ssd1306_dma_restart( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    if( ( ssd1306_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_LOOP_PAF ) == FVIO_I2C_DMA_LOOP_PAF ){
        //次回のDMAを発行(PAF)
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 0x8;    //SWRST=1
        (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 1;        //SETEN=1
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :ssd1306_isr_pae_func
 * [機能]    :ssd1306 FIFO_PAE割り込み
 * [引数]    :int32_t slot_id           スロットID
 * [返値]    :なし
 * [備考]    :fvIO_rzt1_i2c_cmn.cの各割り込み処理(fvio_i2c_cmn_isr_paen)からコールされる
 ***************************************************************************/
static void fvio_i2c_ssd1306_isr_pae_func( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    //コールバック
    if( ssd1306_inf.TblfvIOInt[slot_id].pae_callback != NULL ){
        ssd1306_inf.TblfvIOInt[slot_id].pae_callback(slot_id);
    }

    //busyフラグクリア( LOOPの場合はクリアしない )
    if( ssd1306_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_BUSY_PAE ){
        ssd1306_inf.TblPlugBusy[slot_id] &= ~FVIO_I2C_DMA_BUSY_PAE;
    }
}

/***************************************************************************
 * [名称]    :ssd1306_isr_paf_func
 * [機能]    :ssd1306 FIFO_PAF割り込み
 * [引数]    :int32_t slot_id           スロットID
 * [返値]    :なし
 * [備考]    :fvIO_rzt1_i2c_cmn.cの各割り込み処理(fvio_i2c_cmn_isr_pafn)からコールされる
 ***************************************************************************/
static void fvio_i2c_ssd1306_isr_paf_func( int32_t slot_id )
{
    uint32_t ofst = 0x10 * slot_id;

    //コールバック
    if( ssd1306_inf.TblfvIOInt[slot_id].paf_callback != NULL ){
        ssd1306_inf.TblfvIOInt[slot_id].paf_callback(slot_id);
    }

    //busyフラグクリア( LOOPの場合はクリアしない )
    if( ssd1306_inf.TblPlugBusy[slot_id] & FVIO_I2C_DMA_BUSY_PAF ){
        ssd1306_inf.TblPlugBusy[slot_id] &= ~FVIO_I2C_DMA_BUSY_PAF;
    }
}

