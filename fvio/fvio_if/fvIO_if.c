/***************************************************************************
fvIO I/Fソース                                       作成者:シマフジ電機(株)
 ***************************************************************************/
#include "platform.h"
#include "r_port.h"
#include "r_system.h"
#include "r_icu_init.h"
#include "r_reset2.h"
#include "stdlib.h"

#include "r_ecl_rzt1_if.h"
#include "fvIO_if.h"

#define FVIO_ID_MASK        (0xFFFFFF00)                        //fvIO IDマスク

#define FVIO_IF_STS_INIT    (0)
#define FVIO_IF_STS_ASSIGN    (1)
#define FVIO_IF_STS_START    (2)

//i/f内部データ
typedef struct {
    uint32_t        Plug2Slot[FVIO_SLOT_NUM];                   //スロットに登録するプラグイン(fvio IDを登録)
    void            *FvIOConfig[FVIO_SLOT_NUM];                 //スロットに登録するconfigデータ
    ST_FVIO_IF_LIST *ListFvioSlt[FVIO_SLOT_NUM];                //スロットに登録したFVIOのリスト(スロット分登録可能、オープンリストからピックアップ)
    uint32_t        SlotSts[FVIO_SLOT_NUM];                     //スロットのスタート情報
}ST_FVIO_IF_DATA;

static ST_FVIO_IF_LIST    *ListFvioEntry;                       //FVIO登録リスト(メモリ領域に空きがある限り無制限に登録可能)
static ST_FVIO_IF_DATA fvio_inf;                                //FVIO I/F情報

/***************************************************************************
 * [名称]    :fvio_sys_init
 * [機能]    :fvIOシステム初期化処理
 * [引数]    :なし
 * [返値]    :なし
 * [備考]    :
 ***************************************************************************/
void fvio_sys_init( void )
{
    int32_t i;

    //モジュールストップ解除
    R_RST_WriteEnable();
    MSTP(DMAC0) = 0;            //DMA
    R_RST_WriteDisable();

    //FVIO I/F情報初期化
    for( i = 0 ; i < FVIO_SLOT_NUM ; i++ ){
        fvio_inf.Plug2Slot[i]   = 0;
        fvio_inf.FvIOConfig[i]  = NULL;
        fvio_inf.ListFvioSlt[i] = NULL;
        fvio_inf.SlotSts[i]     = FVIO_IF_STS_INIT;
    }

    //FVIO登録リスト初期化
    ListFvioEntry = NULL;
}

/***************************************************************************
 * [名称]    :fvio_entry
 * [機能]    :fvIOエントリ処理
 * [引数]    :ST_FVIO_LIST new_entry      新規追加するfvIO I/F関数リスト
 *            uint32_t mode               処理モード
 *            int32_t *fvio_id            fvIO ID
 * [返値]    :int32_t                     0=正常, それ以外=異常
 * [備考]    :
 ***************************************************************************/
int32_t fvio_entry( ST_FVIO_IF_LIST *new_entry, uint32_t mode, int32_t *fvio_id )
{
    uint32_t i;
    ST_FVIO_IF_LIST *new_list, *tmp_list;

    //登録情報のチェック
    if( ( new_entry->assign == NULL )         ||
        ( new_entry->unassign == NULL )       ||
        ( new_entry->write == NULL )          ||
        ( new_entry->read == NULL )           ||
        ( new_entry->start == NULL )          ||
        ( new_entry->stop == NULL )           ||
        ( new_entry->slot_sz > FVIO_SLOT_NUM )||
        ( new_entry->slot_sz == 0 )           ||
        ( new_entry->next_list != NULL ) ){
        return -1;        //関数エラー
    }

    //メモリ領域の確保
    new_list = (ST_FVIO_IF_LIST *)malloc(sizeof(ST_FVIO_IF_LIST));
    if(new_list == NULL){
        return -1;                                      //メモリ領域が取得できない
    }

    *new_list = *new_entry;                             //新規リストにパラメータを設定

    //リスト登録
    if( ListFvioEntry == NULL ){                        //リストが空の場合
        ListFvioEntry = new_list;
        *fvio_id = ListFvioEntry->fvio_id;
    }else{
        //リストの終端検索
        tmp_list = ListFvioEntry;                       //登録リストを取得

        //登録IDを検索
        while(1){
            //idチェック
            if( tmp_list->fvio_id == new_entry->fvio_id ){
                free(new_list);                         //新リスト削除
                return -1;                              //既存リストに一致するIDが存在
            }

            //リスト終端?
            if( tmp_list->next_list == NULL){
                tmp_list->next_list = new_list;         //リストに新規登録
                break;
            }else{
                tmp_list = tmp_list->next_list;         //次のリストへ
            }
        }
        //新規追加のdev_idを返す
        *fvio_id = tmp_list->next_list->fvio_id;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_release
 * [機能]    :fvIOリリース処理
 * [引数]    :int32_t fvio_id              fvio ID
 * [返値]    :int32_t                      0=正常、0以外=エラー
 * [備考]    :
 ***************************************************************************/
int32_t fvio_release( int32_t fvio_id )
{
    uint8_t i;
    ST_FVIO_IF_LIST *tmp_list, *tmp_list2;

    //オープン中のfvIOが存在するかチェック
    if(ListFvioEntry==NULL){
        return -1;
    }

    //プラグインがunassignできているか確認
    for( i = 0 ; i < FVIO_SLOT_NUM ; i++ ){
        if( fvio_inf.Plug2Slot[i] == fvio_id ){                          //fvio IDチェック
            return -1;                                                   //fvio IDが一致するassign中のスロットあり
        }
    }

    //先頭リストを削除
    if( ListFvioEntry->fvio_id == fvio_id ){
        tmp_list2 = ListFvioEntry->next_list;                           //next_listを退避
        free(ListFvioEntry);                                            //リストを削除
        ListFvioEntry = tmp_list2;                                      //削除したリストの代わりにnext_listを設定
    //先頭以外を削除
    }else{
        tmp_list = ListFvioEntry;
        while(1){
            if( tmp_list->next_list == NULL ){
                return -1;
            }else if( tmp_list->next_list->fvio_id == fvio_id ){
                tmp_list2 = tmp_list->next_list->next_list;             //next_listを退避
                free(tmp_list->next_list);                              //リストを削除
                tmp_list->next_list = tmp_list2;                        //削除したリストの代わりにnext_listを設定
                break;
            }else{
                tmp_list = tmp_list->next_list;
            }
        }
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_assign
 * [機能]    :fvIOアサイン処理
 * [引数]    :int32_t fvio_id             fvIO ID
 *            int32_t slot_id             スロットID(番号)
 *            int32_t *plug_id            プラグインID
 *            void *attr                  実装依存の引数
 * [返値]    :int32_t                     0以上の値=プラグインID, 負数=エラー
 * [備考]    :
 ***************************************************************************/
int32_t fvio_assign( int32_t fvio_id, int32_t slot_id, int32_t *plug_id, void *attr )
{
    int32_t ret;
    int8_t i;
    ST_FVIO_IF_LIST *tmp_list;

    //スロットidチェック
    if( ( slot_id >= FVIO_SLOT_NUM ) ||
        ( slot_id < 0 )    ){
        return -1;
    }

    //ステータス情報をチェック
    if( fvio_inf.SlotSts[slot_id] != FVIO_IF_STS_INIT ){
        return -1;                                                       //アサイン済み
    }

    //オープン中のfvIOをチェック
    if(ListFvioEntry==NULL){
        return -1;
    }

    //登録した関数をコール
    tmp_list = ListFvioEntry;
    while( 1 ){
        if( tmp_list->fvio_id == fvio_id ){

            //スロットidチェック
            if( ( slot_id >= FVIO_SLOT_NUM ) ||                          //slot_idの範囲
                ( ( slot_id + tmp_list->slot_sz ) > FVIO_SLOT_NUM  ) ){  //slot_szの範囲
                return -1;                                               //パラメータエラー
            }

            //アサイン済みかチェック
            for( i = 0 ; i < tmp_list->slot_sz ; i++ ){
                if( fvio_inf.Plug2Slot[slot_id+i] == fvio_id ){
                    return -1;                                           //assign済
                }
            }

            ret = tmp_list->assign( slot_id, &fvio_inf.FvIOConfig[slot_id], attr ) ;
            break;
        }

        tmp_list = tmp_list->next_list;

        //終端チェック
        if( tmp_list == NULL){
            return -1;                                                   //不正fvio ID
        }
    }

    //結果確認
    if( ret == 0 ){                                                      //正常
        for( i = 0 ; i < tmp_list->slot_sz ; i++ ){
            fvio_inf.ListFvioSlt[slot_id+i] = tmp_list;                  //スロットリストに登録
            fvio_inf.Plug2Slot[slot_id+i]   = fvio_id;                   //fvio IDを登録
            fvio_inf.SlotSts[slot_id+i]     = FVIO_IF_STS_ASSIGN;        //ステータス更新
        }
        *plug_id = ( fvio_id & FVIO_ID_MASK) | slot_id;                  //plug_IDを返す
        return 0;
    }else{
        return -1;                                                       //アサイン処理エラー
    }
}

/***************************************************************************
 * [名称]    :fvio_unassign
 * [機能]    :fvIOアンアサイン処理
 * [引数]    :int32_t plug_id            プラグインID
 * [返値]    :int32_t                    0=正常、0以外=エラー
 * [備考]    :
 ***************************************************************************/
int32_t fvio_unassign( int32_t plug_id )
{
    int32_t ret, i;
    int32_t slot_id;
    uint32_t sz;

    slot_id = plug_id & 0xFF;

    //スロットidチェック
    if( ( slot_id >= FVIO_SLOT_NUM ) ||
        ( slot_id < 0 )                 ||
        ( fvio_inf.Plug2Slot[slot_id] & FVIO_ID_MASK ) != ( plug_id & FVIO_ID_MASK ) ){
        return -1;
    }

    //ステータス情報をチェック
    if( fvio_inf.SlotSts[slot_id] != FVIO_IF_STS_ASSIGN ){
        return -1;                                                       //アサインなし
    }

    //オープン中のfvIOをチェック
    if(ListFvioEntry==NULL){
        return -1;
    }

    //登録した関数をコール
    ret = fvio_inf.ListFvioSlt[slot_id]->unassign( slot_id );
    if( ret == 0 ){
        sz = fvio_inf.ListFvioSlt[slot_id]->slot_sz;

        for( i = 0 ; i < sz ; i++ ){
            fvio_inf.ListFvioSlt[slot_id+i]  = NULL;
            fvio_inf.Plug2Slot[slot_id+i]    = 0 ;
            fvio_inf.FvIOConfig[slot_id+i]   = NULL;
            fvio_inf.SlotSts[slot_id+i]      = FVIO_IF_STS_INIT;
        }

    }

    return ret;
}

/***************************************************************************
 * [名称]    :fvio_sys_start
 * [機能]    :fvIOシステムスタート
 * [引数]    :int32_t *result            各スロットの処理結果 0=正常、 0以外エラー
 *            void *attr                 実装依存の引数
 * [返値]    :int32_t                    0=正常、0以外=エラー
 * [備考]    :スタートは全スロット一括で行う。
 ***************************************************************************/
int32_t fvio_sys_start( int32_t *result, void *attr )
{
    int32_t ret, i, cre_num=0;
    void *config[FVIO_SLOT_NUM];

    for( i =  0 ; i < FVIO_SLOT_NUM ; i++ ){
        //ステータス情報をチェック
        if( fvio_inf.SlotSts[i] == FVIO_IF_STS_START ){
            return -1;
        }

        //アサインしたプラグインを検索
        if( fvio_inf.FvIOConfig[i] != NULL ){
            //プラグインデータを配列の先頭に詰めていく
            config[cre_num++] = fvio_inf.FvIOConfig[i];
        }

        //結果を初期化
        if( result != NULL ){
            result[i] = fvio_inf.SlotSts[i];
        }
    }

    //アサイン数のチェック
    if ( cre_num == 0 ){
        return -1;
    }

    //fvIOロード
    ret = R_ECL_ConfigureMulti( (const void *)config, cre_num );
    if( ret != R_ECL_SUCCESS ){
        return -1;
    }

    //各スロットのプラグインを開始
    for( i = 0 ; i < FVIO_SLOT_NUM ; i++ ){
        //アサインしたプラグインを検索
        if( fvio_inf.FvIOConfig[i] != NULL ){
            if( fvio_inf.ListFvioSlt[i]->start( i, attr ) == 0 ){
                fvio_inf.SlotSts[i] = FVIO_IF_STS_START;
            }else{
                return -1;        //開始に失敗
            }
        }

        //結果を更新
        if( result != NULL ){
            result[i] = fvio_inf.SlotSts[i];
        }
    }

    return 0 ;
}

/***************************************************************************
 * [名称]    :fvio_stop
 * [機能]    :fvIOストップ処理
 * [引数]    :int32_t plug_id            プラグインID
 *            void *attr                 実装依存の引数
 * [返値]    :int32_t                    0=正常、0以外=エラー
 * [備考]    :ストップはスロット個別で行う。
 ***************************************************************************/
int32_t fvio_stop( int32_t plug_id, void *attr )
{
    int32_t i, ret = 0, stop_flg = 0;
    int32_t slot_id;

    slot_id = plug_id & 0xFF;

    //スロットidチェック
    if( ( slot_id >= FVIO_SLOT_NUM ) ||
        ( slot_id < 0 )              ||
        ( fvio_inf.Plug2Slot[slot_id] & FVIO_ID_MASK ) != ( plug_id & FVIO_ID_MASK ) ){
        return -1;
    }

    //ステータス情報のチェック
    if( fvio_inf.SlotSts[slot_id] != FVIO_IF_STS_START ){
        return -1;
    }

    //登録した関数をコール
    if( fvio_inf.ListFvioSlt[slot_id]->stop( slot_id, attr ) == 0 ){
        fvio_inf.SlotSts[slot_id] = FVIO_IF_STS_ASSIGN;
    }else{
        return -1;
    }

    return 0;
}

/***************************************************************************
 * [名称]    :fvio_write
 * [機能]    :fvIOライト処理
 * [引数]    :int32_t plug_id            プラグインID
 *            uint32_t mode              動作モード
 *            void *attr                 実装依存の引数
 * [返値]    :int32_t                    0=正常、0以外=エラー
 * [備考]    :
 ***************************************************************************/
int32_t fvio_write( int32_t plug_id, uint32_t mode, void *attr )
{
    int32_t slot_id = plug_id & 0xff;

    //スロットidチェック
    if( ( slot_id >= FVIO_SLOT_NUM ) ||
        ( slot_id < 0 )              ||
        ( fvio_inf.Plug2Slot[slot_id] & FVIO_ID_MASK ) != ( plug_id & FVIO_ID_MASK ) ){
        return -1;
    }

    //ステータス情報のチェック
    if( fvio_inf.SlotSts[slot_id] != FVIO_IF_STS_START ){
        return -1;
    }

    //登録した関数をコール
    return fvio_inf.ListFvioSlt[slot_id]->write( slot_id, mode, attr );
}

/***************************************************************************
 * [名称]    :fvio_read
 * [機能]    :fvIOリード処理
 * [引数]    :int32_t plug_id            プラグインID
 *            uint32_t mode              動作モード
 *            void *attr                 実装依存の引数
 * [返値]    :int32_t                    0=正常、0以外=エラー
 * [備考]    :
 ***************************************************************************/
int32_t fvio_read( int32_t plug_id, uint32_t mode, void *attr )
{
    int32_t slot_id = plug_id & 0xff;

    //スロットidチェック
    if( ( slot_id >= FVIO_SLOT_NUM ) ||
        ( slot_id < 0 )              ||
        ( fvio_inf.Plug2Slot[slot_id] & FVIO_ID_MASK ) != ( plug_id & FVIO_ID_MASK ) ){
        return -1;
    }

    //ステータス情報のチェック
    if( fvio_inf.SlotSts[slot_id] != FVIO_IF_STS_START ){
        return -1;
    }

    //登録した関数をコール
    return fvio_inf.ListFvioSlt[slot_id]->read( slot_id, mode, attr );
}


