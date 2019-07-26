/***************************************************************************
　fvIO汎用プラグイン・SSD1306専用処理ヘッダ          作成者:シマフジ電機(株)
 ***************************************************************************/

#ifndef _FVIO_RZT1_I2C_SSD1306_H_
#define _FVIO_RZT1_I2C_SSD1306_H_

//I2Cスレーブ
#define FVIO_I2C_SSD1306_SLVADR       (0x78)

//r/wモード
#define FVIO_I2C_SSD1306_MODE_INFO    (0)        //get info
#define FVIO_I2C_SSD1306_MODE_STOP    (1)        //stop
#define FVIO_I2C_SSD1306_MODE_CREG    (2)        //ctrl reg
#define FVIO_I2C_SSD1306_MODE_DMA     (3)        //dma access
#define FVIO_I2C_SSD1306_MODE_CNF     (4)        //config
#define FVIO_I2C_SSD1306_MODE_INT     (5)        //interrupt
#define FVIO_I2C_SSD1306_MODE_BSY     (6)        //busy
#define FVIO_I2C_SSD1306_MODE_RDMA    (7)        //dma restart

//info
#define FVIO_I2C_SSD1306_INFO_TYPE    (FVIO_IF_INFO_TYPE_O)
#define FVIO_I2C_SSD1306_INFO_INSZ    (0)
#define FVIO_I2C_SSD1306_INFO_OUTSZ   (6*128*2)  //1画面分の通信量(DMA込み)

//コントロールレジスタのr/w用構造体
typedef struct {
    uint8_t trg;                                 //トリガ
    uint8_t *sdata;                              //送信データ
    uint8_t *rdata;                              //受信データ
    uint8_t sz;                                  //1通信当たりの長さ
}ST_FVIO_I2C_SSD1306_CREG;

//DMA通信のr/w用構造体
typedef struct {
    uint8_t  trg;                                //トリガ
    uint8_t  *data1;                             //dest adr(rbuf1)
    uint8_t  *data2;                             //dest adr(rbuf2)
    uint8_t  sz;                                 //1通信当たりの長さ
    uint32_t num;                                //1回の通信回数(DMA1回=sz*num)
}ST_FVIO_I2C_SSD1306_DMA;

//fvIO設定のr/w用構造体
typedef struct {
    uint8_t  cwait;                              //分周設定
    uint32_t lwait;                              //トリガ無効期間
}ST_FVIO_I2C_SSD1306_CNF;

//ユーザー定義割り込みのr/w用構造体
typedef struct {
    void (*paf_callback)(int32_t);               //FIFO PAF割り込み関数
    void (*pae_callback)(int32_t);               //FIFO PAE割り込み関数
}ST_FVIO_I2C_SSD1306_INT;

//通信パケット(DMAのwrite用)
typedef struct {
    uint8_t slv_addr;
    uint8_t cntrl;
    uint8_t data[4];
}ST_FVIO_I2C_SSD1306_PACKET;

int32_t fvio_i2c_ssd1306_assign( int32_t slot_id, void **config, void *attr );
int32_t fvio_i2c_ssd1306_unassign( int32_t slot_id );
int32_t fvio_i2c_ssd1306_start( int32_t slot_id, void *attr );
int32_t fvio_i2c_ssd1306_stop( int32_t slot_id, void *attr );
int32_t fvio_i2c_ssd1306_write( int32_t slot_id, uint32_t mode, void *attr );
int32_t fvio_i2c_ssd1306_read( int32_t slot_id, uint32_t mode, void *attr );

extern ST_FVIO_IF_LIST fvio_i2c_ssd1306_entry;

#endif
