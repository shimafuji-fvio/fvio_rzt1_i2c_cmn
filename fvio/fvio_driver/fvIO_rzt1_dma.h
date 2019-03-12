/***************************************************************************
　fvIO　DMA設定処理ヘッダ                            作成者:シマフジ電機(株)
 ***************************************************************************/

#ifndef _FVIO_DMA_H_
#define _FVIO_DMA_H_

void dma_init_s( uint32_t slot_id, uint8_t *src1, uint8_t *src2, uint8_t *dest, uint32_t sz );
void dma_init_r( uint32_t slot_id, uint8_t *src, uint8_t *dest1, uint8_t *dest2, uint32_t sz );

#endif
