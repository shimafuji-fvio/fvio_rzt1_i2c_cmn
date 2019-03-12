/***************************************************************************
メインサンプル                                       作成者:シマフジ電機(株)
 ***************************************************************************/
#include "platform.h"
#include "iodefine.h"
#include "r_system.h"
#include "r_icu_init.h"
#include "r_mpc2.h"
#include "r_port.h"
#include "r_ecm.h"

#include "r_ecl_rzt1_if.h"
#include "r_reset2.h"
#include "fvIO_cmn_if.h"
#include "fvIO_if.h"
#include "string.h"

#include "fvIO_rzt1_i2c_adxl345.h"
#include "fvIO_rzt1_i2c_ssd1306.h"
#include "utility.h"

int main(void);

void user_adxl_isr( int32_t slot_id );
void user_ssd_isr( int32_t slot_id );
void init_val( void );
void debug_port_init( void );

#define SLOT_NO_SSD     (1)
#define SLOT_NO_ADXL    (0)

uint8_t oled_startup_cmd[][6] = {
      //slave adr,          data,data,data,data,size
        FVIO_SSD1306_SLVADR,0x00,0xAE,0x00,0x00,2,        //display off
        FVIO_SSD1306_SLVADR,0x00,0xA8,0x3F,0x00,3,        //Set Multiplex Ratio
        FVIO_SSD1306_SLVADR,0x00,0xD3,0x00,0x00,3,        //display offset
        FVIO_SSD1306_SLVADR,0x00,0x40,0x00,0x00,2,        //set display start line
        FVIO_SSD1306_SLVADR,0x00,0xA0,0x00,0x00,2,        //Set Segment Re-map
        FVIO_SSD1306_SLVADR,0x00,0xC0,0x00,0x00,2,        //Set COM Output Scan Direction
        FVIO_SSD1306_SLVADR,0x00,0xDA,0x11,0x00,3,        //Set COM Pins Hardware Configuration
        FVIO_SSD1306_SLVADR,0x00,0x81,0x7F,0x00,3,        //Set Contrast Control
        FVIO_SSD1306_SLVADR,0x00,0xA4,0x00,0x00,2,        //Entire Display ON
        FVIO_SSD1306_SLVADR,0x00,0xA6,0x00,0x00,2,        //Set Normal/Inverse Display
        FVIO_SSD1306_SLVADR,0x00,0xD5,0x80,0x00,3,        //Set Display Clock Divide Ratio/Oscillator Frequency
        FVIO_SSD1306_SLVADR,0x00,0x20,0x01,0x00,3,        //Set Memory Addressing Mode
        FVIO_SSD1306_SLVADR,0x00,0x21,0x00,0x7F,4,        //Set Column Address
        FVIO_SSD1306_SLVADR,0x00,0x22,0x00,0x07,4,        //Set Page Address
        FVIO_SSD1306_SLVADR,0x00,0x8D,0x14,0x00,3,        //Charge Pump Setting
        FVIO_SSD1306_SLVADR,0x00,0xAF,0x00,0x00,2,        //display on
        FVIO_SSD1306_SLVADR,0xff,0xff,0xff,0xff,0,        //end(size=0)
};

ST_FVIO_SSD1306_PACKET oled_data[2][128*2] __attribute__ ((section(".uncached_section")));    //OLED出力値
uint8_t acc_data[2][128*6] __attribute__ ((section(".uncached_section")));                    //acc取得値

int32_t wend=0;
int32_t rend=0;
int8_t r_p=0;                                //ACCから入力中のバッファを示す
int8_t w_p=0;                                //OLEDへ出力中のバッファを示す

int32_t plug_id_ssd, plug_id_adxl;

/***************************************************************************
 * [名称]    :icu_init
 * [機能]    :割り込み初期化
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void icu_init(void)
{
    /* Initialize VIC (dummy writing to HVA0 register) */
    HVA0_DUMMY_WRITE();

    /* Enable IRQ interrupt (Clear CPSR.I bit to 0) */
    asm("cpsie i");   // Clear CPSR.I bit to 0
    asm("isb");       // Ensuring Context-changing
}

/***************************************************************************
 * [名称]    :user_adxl_isr
 * [機能]    :adxl355とのDMA完了割り込み(FIFO_PAE)
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void user_adxl_isr( int32_t slot_id )
{
    //DMA再実行
    if( slot_id == SLOT_NO_ADXL ){
        fvio_write( plug_id_adxl, FVIO_ADXL345_MODE_RDMA, NULL );
        r_p = (~r_p)&1;
        rend=1;
    }
}

/***************************************************************************
 * [名称]    :user_ssd_isr
 * [機能]    :ssd1306とのDMA完了割り込み(FIFO_PAF)
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void user_ssd_isr( int32_t slot_id )
{
    if( slot_id == SLOT_NO_SSD ){
        w_p = (~w_p)&1;
        wend=1;
    }
}

/*******************************************************************************
 * [名称]    :main
 * [機能]    :メイン処理
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
*******************************************************************************/
int main (void)
{
    int32_t i;
    int32_t dev_id_ssd, dev_id_adxl;
    uint8_t oled_col[8];
    uint8_t sdata[8], rddata[8];
    int32_t result[FVIO_SLOT_NUM]={0,0,0,0,0,0,0,0};
    int8_t r_wkp, w_wkp;

    ST_FVIO_ADXL345_CNF     adxl_cnf;
    ST_FVIO_ADXL345_CREG    adxl_creg;
    ST_FVIO_ADXL345_DMA     adxl_dma;
    ST_FVIO_ADXL345_INT     adxl_int;

    ST_FVIO_SSD1306_CNF     ssd_cnf;
    ST_FVIO_SSD1306_CREG    ssd_creg;
    ST_FVIO_SSD1306_DMA     ssd_dma;
    ST_FVIO_SSD1306_INT     ssd_int;

    ST_FVIO_SSD1306_CNF     ssd;
    ST_FVIO_ADXL345_CREG    adxl;

    //ダミーの待ち時間
    for (i = 0; i < 10000000 ; i++ )
    {
        asm("nop");
    }
    
    icu_init();

    //OLED出力データ初期化
    init_val();

    //fvio初期化
    fvio_sys_init();

    //fvioエントリ
    if( fvio_entry( &fvio_i2c_adxl345_entry, 0, &dev_id_adxl ) < 0 ){
        goto end;
    }

    if( fvio_entry( &fvio_i2c_ssd1306_entry, 0, &dev_id_ssd ) < 0 ){
        goto end;
    }

    //fvIOアサイン
    if( fvio_assign(dev_id_adxl, SLOT_NO_ADXL, &plug_id_adxl, NULL ) < 0 ){
        goto end;
    }

    if( fvio_assign(dev_id_ssd, SLOT_NO_SSD, &plug_id_ssd, NULL ) < 0 ){
        goto end;
    }

    //fvIOスタート
    if( fvio_sys_start(result, NULL) != 0 ){
        goto end;
    }

    //fvIOプラグイン設定
    ssd_cnf.lwait=0;
    ssd_cnf.cwait=13;
    fvio_write(plug_id_ssd, FVIO_SSD1306_MODE_CNF, &ssd_cnf);

    adxl_cnf.lwait=0;
    adxl_cnf.cwait=20;
    fvio_write(plug_id_adxl, FVIO_ADXL345_MODE_CNF, &adxl_cnf);

    //SSD1306のスタートアップ設定
    for( i = 0 ; oled_startup_cmd[i][5] != 0 ; i++ ){
        ssd_creg.trg = 0;
        sdata[0]   = oled_startup_cmd[i][0];
        sdata[1]   = oled_startup_cmd[i][1];
        sdata[2]   = oled_startup_cmd[i][2];
        sdata[3]   = oled_startup_cmd[i][3];
        sdata[4]   = oled_startup_cmd[i][4];
        ssd_creg.sdata  = sdata;
        ssd_creg.sz     = oled_startup_cmd[i][5];
        fvio_write( plug_id_ssd, FVIO_SSD1306_MODE_CREG, &ssd_creg );
    }

    //ADXL345のスタートアップ設定
    //データレート
    adxl_creg.trg   = 0;
    sdata[0]        = FVIO_ADXL345_SLVADR;
    sdata[1]        = 0x2c;
    sdata[2]        = 0xf;
    adxl_creg.sdata = sdata;
    adxl_creg.sz    = 2;
    fvio_write( plug_id_adxl, FVIO_ADXL345_MODE_CREG, &adxl_creg );

    //wake up
    adxl_creg.trg   = 0;
    sdata[0]        = FVIO_ADXL345_SLVADR;
    sdata[1]        = 0x2d;
    sdata[2]        = 0x8;        //8Hz
    adxl_creg.sdata = sdata;
    adxl_creg.sz    = 2;
    fvio_write( plug_id_adxl, FVIO_ADXL345_MODE_CREG, &adxl_creg );

    //割り込みハンドラ設定
    adxl_int.pae_callback = user_adxl_isr;
    adxl_int.paf_callback = NULL;
    fvio_write( plug_id_adxl, FVIO_ADXL345_MODE_INT, &adxl_int );

    ssd_int.pae_callback = NULL;
    ssd_int.paf_callback = user_ssd_isr;
    fvio_write( plug_id_ssd, FVIO_SSD1306_MODE_INT, &ssd_int );

    //DMA転送開始(SSD1306)
    ssd_dma.trg   = FVIO_CMN_REG_TRG_REP;
    ssd_dma.data1 = (uint8_t*)oled_data[0];
    ssd_dma.data2 = (uint8_t*)oled_data[1];
    ssd_dma.sz    = 5;
    ssd_dma.num   = 128*2;
    fvio_write( plug_id_ssd, FVIO_SSD1306_MODE_DMA, &ssd_dma );

    //DMA転送開始(AXDL345)
    adxl_dma.trg   = FVIO_CMN_REG_TRG_REP;
    adxl_dma.data1 = acc_data[0];
    adxl_dma.data2 = acc_data[1];
    adxl_dma.sz    = 5;
    adxl_dma.num   = 128;
    fvio_write( plug_id_adxl, FVIO_ADXL345_MODE_DMA, &adxl_dma );

    while(1){
        //受信完了
        if( wend == 1 && rend == 1 ){
            wend = 0;
            rend = 0;
            r_wkp = (~r_p)&1;
            w_wkp = (~w_p)&1;

            //DMA再実行
            fvio_write( plug_id_ssd, FVIO_SSD1306_MODE_RDMA, NULL );

            for( i = 0 ; i < 128 ; i++ ){
                //データ変換(1列分)
                conv_adxl2ssd( &acc_data[r_wkp][(i*6)], oled_col, 1 );

                //画面プラス領域
                oled_data[w_wkp][i*2].data[0] = oled_col[0];
                oled_data[w_wkp][i*2].data[1] = oled_col[1];
                oled_data[w_wkp][i*2].data[2] = oled_col[2];
                oled_data[w_wkp][i*2].data[3] = oled_col[3];

                //画面マイナス領域
                oled_data[w_wkp][i*2+1].data[0] = oled_col[4];
                oled_data[w_wkp][i*2+1].data[1] = oled_col[5];
                oled_data[w_wkp][i*2+1].data[2] = oled_col[6];
                oled_data[w_wkp][i*2+1].data[3] = oled_col[7];

            }
        }
    }
end:
    return 0;
}
/*******************************************************************************
 End of function main
*******************************************************************************/

/***************************************************************************
 * [名称]    :init_val
 * [機能]    :グローバル変数初期化
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void init_val( void )
{
    int32_t i, j;

    //OLED出力データ
    for( i = 0 ; i < 2 ; i++ ){
        for( j = 0 ; j < 256 ; j++ ){
            oled_data[i][j].slv_addr = FVIO_SSD1306_SLVADR;  //スレーブアドレス
            oled_data[i][j].cntrl    = 0x40;                 //コントロール
            oled_data[i][j].data[0]  = 0;                    //データ0
            oled_data[i][j].data[1]  = 0;                    //データ1
            oled_data[i][j].data[2]  = 0;                    //データ2
            oled_data[i][j].data[3]  = 0;                    //データ3
        }
    }
}

/* End of File */
