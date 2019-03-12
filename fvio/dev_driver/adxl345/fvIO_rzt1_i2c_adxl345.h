/***************************************************************************
　fvIO汎用プラグイン・ADXL354専用処理ヘッダ          作成者:シマフジ電機(株)
 ***************************************************************************/

#ifndef _FVIO_ADXL345_H_
#define _FVIO_ADXL345_H_

//i2cスレーブ
#define FVIO_ADXL345_SLVADR           (0xA6)

//r/wモード
#define FVIO_ADXL345_MODE_INFO        (0)    //get info
#define FVIO_ADXL345_MODE_STOP        (1)    //stop
#define FVIO_ADXL345_MODE_CREG        (2)    //ctrl reg
#define FVIO_ADXL345_MODE_DMA         (3)    //dma access
#define FVIO_ADXL345_MODE_CNF         (4)    //config
#define FVIO_ADXL345_MODE_INT         (5)    //interrupt
#define FVIO_ADXL345_MODE_BSY         (6)    //busy
#define FVIO_ADXL345_MODE_RDMA        (7)    //dma restart

//info
#define FVIO_ADXL345_INFO_TYPE        (FVIO_IF_INFO_TYPE_IO)
#define FVIO_ADXL345_INFO_INSZ        (8)    //1パケットの最大サイズ
#define FVIO_ADXL345_INFO_OUTSZ       (8)    //1パケットの最大サイズ

//データレジスタ
#define FVIO_ADXL345_REG_DATAX0       (0x32)
#define FVIO_ADXL345_REG_DATAX1       (0x33)
#define FVIO_ADXL345_REG_DATAY0       (0x34)
#define FVIO_ADXL345_REG_DATAY1       (0x35)
#define FVIO_ADXL345_REG_DATAZ0       (0x36)
#define FVIO_ADXL345_REG_DATAZ1       (0x37)

//コントロールレジスタのr/w用構造体
typedef struct {
    uint8_t trg;                             //トリガ
    uint8_t *sdata;                          //送信データ
    uint8_t *rdata;                          //受信データ
    uint8_t sz;                              //1通信当たりの長さ
}ST_FVIO_ADXL345_CREG;

//DMA通信のr/w用構造体
typedef struct {
    uint8_t  trg;                            //トリガ
    uint8_t  *data1;                         //dest adr(rbuf1)
    uint8_t  *data2;                         //dest adr(rbuf2)
    uint8_t  sz;                             //1通信当たりの長さ
    uint32_t num;                            //1回の通信回数(DMA1回=sz*num)
}ST_FVIO_ADXL345_DMA;

//fvIO設定のr/w用構造体
typedef struct {
    uint8_t  cwait;                          //分周設定
    uint32_t lwait;                          //トリガ無効期間
}ST_FVIO_ADXL345_CNF;

//ユーザー定義割り込み処理のr/w用構造体
typedef struct {
    void (*paf_callback)(int32_t);           //FIFO PAF割り込み関数
    void (*pae_callback)(int32_t);           //FIFO PAE割り込み関数
}ST_FVIO_ADXL345_INT;

int32_t fvio_adxl345_assign( int32_t slot_id, void **config, void *attr );
int32_t fvio_adxl345_unassign( int32_t slot_id );
int32_t fvio_adxl345_start( int32_t slot_id, void *attr );
int32_t fvio_adxl345_stop( int32_t slot_id, void *attr );
int32_t fvio_adxl345_write( int32_t slot_id, uint32_t mode, void *attr );
int32_t fvio_adxl345_read( int32_t slot_id, uint32_t mode, void *attr );

extern ST_FVIO_IF_LIST fvio_i2c_adxl345_entry;

#endif
