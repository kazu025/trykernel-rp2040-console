#ifndef SYSLIB_H
#define SYSLIB_H
/* 
 *** Try Kernel
 *      共通ライブラリ関数定義
 */

/* 32bitレジスタからの入力 */
static inline UW in_w(UW adr)
{
    return *(_UW*)adr;
}

/* 32bitレジスタへの出力 */
static inline void out_w(UW adr, UW data)
{
    *(_UW*)adr = data;
}

/* 32bitレジスタへの出力(ビットクリア) */
#define OP_CLR      0x3000
static inline void clr_w(UW adr, UW data)
{
    *(_UW*)(adr + OP_CLR) = data;
}

/* 32bitレジスタへの出力(ビットセット) */
#define OP_SET       0x2000
static inline void set_w(UW adr, UW data)
{
    *(_UW*)(adr + OP_SET) = data;
}

/* 32bitレジスタへの出力(ビット排他的論理和) */
#define OP_XOR      0x1000
static inline void xset_w(UW adr, UW data)
{
    *(_UW*)(adr + OP_XOR) = data;
}

/* PRIMASKレジスタ制御インライン関数 */
static inline void set_primask( INT pm )
{
    __asm__ volatile("msr primask, %0":: "r"(pm));
}

static inline UW get_primask( void )
{
    UW  pm;
    __asm__ volatile("mrs %0, primask": "=r"(pm));
    return pm;
}

/* IPSRレジスタの取得 */
static inline UW get_ipsr(void)
{
    UW ipsr;
    __asm__ volatile("mrs %0, ipsr" : "=r"(ipsr));
    return ipsr;
}

/*  割り込みコンテキスト判定 */
/*
 * IPSR:「今どの例外/割り込みを処理しているのか」を示すレジスタ
 * 0:通常 2:NMI 3:HardFault 11:SVCall 14:PendSV 15:SysTick 16:外部IRQ0 ... 36:外部IRQ20
 */
static inline BOOL is_interrupt_context(void)
{
    return(get_ipsr() != 0U);
}
/* 割込み禁止マクロ */
#define	DI(intsts)	(intsts=get_primask(), set_primask(1))

/* 割込み許可マクロ */
#define	EI(intsts)	(set_primask(intsts))

#endif  /* STYLIB_H */