/***************************************************************************
　fvIO　DMA設定処理ソース                            作成者:シマフジ電機(株)
 ***************************************************************************/
#include "platform.h"
#include "r_port.h"
#include "r_mpc2.h"
#include "r_system.h"
#include "r_icu_init.h"

#include "fvIO_if.h"
#include "fvIO_rzt1_dma.h"

/***************************************************************************
 * [名称]    :dma_init_s
 * [機能]    :DMA設定( cpu mem → fvio fifo )
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t *sdata1            バッファ1
 *            uint8_t *sdata2            バッファ2
 *            uint8_t *dest
 *            uint32_t sz                DMA転送サイズ
 * [返値]    :なし
 * [備考]    :バッファ1,バッファ2はダブルバッファとして使用する。
 *            転送が完了した場合、DMAソースはバッファ1からバッファ2へ切り替わる
 ***************************************************************************/
void dma_init_s( uint32_t slot_id, uint8_t *src1, uint8_t *src2, uint8_t *dest, uint32_t sz )
{
    uint32_t ofst = 0x10 * slot_id;
    uint32_t ofst2= 0x01 * slot_id;

    DMA0.DMAC0_DCTRL_A.LONG         = 0x00000001;                                      //round robin

    (*(volatile uint32_t*)(&DMA0.N0SA_0.DMAC0_N0SA_0_N.LONG + ofst)) = (uint32_t)src1; //src Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.N1SA_0.DMAC0_N1SA_0_N.LONG + ofst)) = (uint32_t)src2; //src Next1 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N0DA_0.LONG + ofst)) = (uint32_t)dest;          //FIFO Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N1DA_0.LONG + ofst)) = (uint32_t)dest;          //FIFO Next1 Register Set

    (*(volatile uint32_t*)(&DMA0.DMAC0_N0TB_0.LONG + ofst)) = sz;                      //SIZE Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N1TB_0.LONG + ofst)) = sz;                      //SIZE Next1 Register Set

    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCFG_0.LONG + ofst))= 0x20200258 | slot_id;

    (*(volatile uint32_t*)(&DMA0.DMA0SEL0.LONG + ofst2))     = FVIO_IF_SLOT0_VECT_PAF+(slot_id<<1);

    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 0x8;                   //SWRST=1
    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_0.LONG + ofst)) = 1;                     //SETEN=1
}

/***************************************************************************
 * [名称]    :dma_init_r
 * [機能]    :DMA設定( fvio fifo →　cpu mem )
 * [引数]    :int32_t slot_id            スロットID
 *            uint8_t *src
 *            uint8_t *rdata1            バッファ1
 *            uint8_t *rdata2            バッファ2
 *            uint32_t sz                DMA転送サイズ
 * [返値]    :なし
 * [備考]    :バッファ1,バッファ2はダブルバッファとして使用する。
 *            転送が完了した場合、DMAデスティネーションはバッファ1からバッファ2へ切り替わる
 ***************************************************************************/
void dma_init_r( uint32_t slot_id, uint8_t *src, uint8_t *dest1, uint8_t *dest2, uint32_t sz )
{
    uint32_t ofst = 0x10 * slot_id;
    uint32_t ofst2= 0x01 * slot_id;

    DMA0.DMAC0_DCTRL_B.LONG         = 0x00000001;                                      //round robin

    (*(volatile uint32_t*)(&DMA0.N0SA_8.DMAC0_N0SA_8_N.LONG + ofst)) = (uint32_t)src;  //FIFO Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.N1SA_8.DMAC0_N1SA_8_N.LONG + ofst)) = (uint32_t)src;  //FIFO Next1 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N0DA_8.LONG + ofst)) = (uint32_t)dest1;         //src Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N1DA_8.LONG + ofst)) = (uint32_t)dest2;         //src Next1 Register Set

    (*(volatile uint32_t*)(&DMA0.DMAC0_N0TB_8.LONG + ofst)) = sz;                      //SIZE Next0 Register Set
    (*(volatile uint32_t*)(&DMA0.DMAC0_N1TB_8.LONG + ofst)) = sz;                      //SIZE Next1 Register Set

    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCFG_8.LONG + ofst))= 0x20100250 | (slot_id);

    (*(volatile uint32_t*)(&DMA0.DMA0SEL8.LONG + ofst2))     = FVIO_IF_SLOT0_VECT_PAE+(slot_id<<1);

    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 0x8;                   //SWRST=1
    (*(volatile uint32_t*)(&DMA0.DMAC0_CHCTRL_8.LONG + ofst)) = 1;                     //SETEN=1
}
