/*
   BCM2835 Peripheral
*/

#ifndef __BCM2835Peri_h__
#define __BCM2835Peri_h__

#define BLOCK_SIZE (4*1024)

#define BCM2835_PERI_BASE    0x20000000
#define BCM2836_PERI_BASE    0x3f000000

#define GPIOCLK_OFFSET         0x101000
#define   GPCLKCTL           0x70/4
#define   GPCLKDIV           0x74/4
#define GPIO_OFFSET            0x200000
#define   GPFSEL             0x00/4
#define     FSELIN           0
#define     FSELOUT          1
#define     FSELALT0         4
#define     FSELALT1         5
#define     FSELALT2         6
#define     FSELALT3         7
#define     FSELALT4         3
#define     FSELALT5         2
#define   GPSET              0x1c/4
#define   GPCLR              0x28/4
#define   GPLEV              0x34/4
#define   GPEDS              0x40/4
#define   GPREN              0x4c/4
#define   GPFEN              0x58/4
#define   GPHEN              0x64/4
#define   GPLEN              0x70/4
#define   GPAREN             0x7c/4
#define   GPAFEN             0x88/4
#define   GPPUD              0x94/4
#define   GPPUDCLK           0x98/4
#define   GPPUDCLK0          0x98/4
#define   GPPUDCLK1          0x9c/4
#define PCM_OFFSET             0x203000
#define   PCMCS              0x00/4
#define     SHIFT_PCMCSSTBY      25
#define     PCMCSSTBY            (1 << SHIFT_PCMCSSTBY)
#define     SHIFT_PCMCSSYNC      24
#define     PCMCSSYNC            (1 << SHIFT_PCMCSSYNC)
#define     SHIFT_PCMCSRXSEX     23
#define     PCMCSRXSEX           (1 << SHIFT_PCMCSRXSEX)
#define     SHIFT_PCMCSRXF       22
#define     PCMCSRXF             (1 << SHIFT_PCMCSRXF)
#define     SHIFT_PCMCSTXE       21
#define     PCMCSTXE             (1 << SHIFT_PCMCSTXE)
#define     SHIFT_PCMCSRXD       20
#define     PCMCSTXD             (1 << SHIFT_PCMCSTXD)
#define     SHIFT_PCMCSTXD       19
#define     PCMCSRXD             (1 << SHIFT_PCMCSRXD)
#define     SHIFT_PCMCSRXR       18
#define     PCMCSRXR             (1 << SHIFT_PCMCSRXR)
#define     SHIFT_PCMCSTXW       17
#define     PCMCSTXW             (1 << SHIFT_PCMCSTXW)
#define     SHIFT_PCMCSRXERR     16
#define     PCMCSRXERR           (1 << SHIFT_PCMCSRXERR)
#define     SHIFT_PCMCSTXERR     15
#define     PCMCSTXERR           (1 << SHIFT_PCMCSTXERR)
#define     SHIFT_PCMCSRXSYNC    14
#define     PCMCSRXSYNC          (1 << SHIFT_PCMCSRXSYNC)
#define     SHIFT_PCMCSTXSYNC    13
#define     PCMCSTXSYNC          (1 << SHIFT_PCMCSTXSYNC)
#define     SHIFT_PCMCSDMAEN     9
#define     PCMCSDMAEN           (1 << SHIFT_PCMCSDMAEN)
#define     SHIFT_PCMCSRXTHR     7
#define     SHIFT_PCMCSTXTHR     5
#define     SHIFT_PCMCSRXCLR     4
#define     PCMCSRXCLR           (1 << SHIFT_PCMCSRXCLR)
#define     SHIFT_PCMCSTXCLR     3
#define     PCMCSTXCLR           (1 << SHIFT_PCMCSTXCLR)
#define     SHIFT_PCMCSTXON      2
#define     PCMCSTXON            (1 << SHIFT_PCMCSTXON)
#define     SHIFT_PCMCSRXON      1
#define     PCMCSRXON            (1 << SHIFT_PCMCSRXON)
#define     SHIFT_PCMCSEN        0
#define     PCMCSEN              (1 << SHIFT_PCMCSEN)
#define   PCMFIFO            0x04/4
#define   PCMMODE            0x08/4
#define     SHIFT_PCMMODECLKDIS  28
#define     PCMMODECLKDIS        (1 << SHIFT_PCMMODECLKDIS)
#define     SHIFT_PCMMODEFRXP    25
#define     PCMMODEFRXP          (1 << SHIFT_PCMMODEFRXP)
#define     SHIFT_PCMMODEFTXP    24
#define     PCMMODEFTXP          (1 << SHIFT_PCMMODEFTXP)
#define     SHIFT_PCMMODECLKM    23
#define     PCMMODECLKM          (1 << SHIFT_PCMMODECLKM)
#define     SHIFT_PCMMODECLKI    22
#define     PCMMODECLKI          (1 << SHIFT_PCMMODECLKI)
#define     SHIFT_PCMMODEFSM     21
#define     PCMMODEFSM           (1 << SHIFT_PCMMODEFSM)
#define     SHIFT_PCMMODEFSI     20
#define     PCMMODEFSI           (1 << SHIFT_PCMMODEFSI)
#define     SHIFT_PCMMODEFLEN    10
#define     SHIFT_PCMMODEFSLEN   0
#define   PCMRXC             0x0c/4
#define     SHIFT_PCMRXCCH1WEX   31
#define     PCMRXCCH1WEX         (1 << SHIFT_PCMRXCCH1WEX)
#define     SHIFT_PCMRXCCH1EN    30
#define     PCMRXCCH1EN          (1 << SHIFT_PCMRXCCH1EN)
#define     SHIFT_PCMRXCCH1POS   20
#define     SHIFT_PCMRXCCH1WID   16
#define     SHIFT_PCMRXCCH2WEX   15
#define     PCMRXCCH2WEX         (1 << SHIFT_PCMRXCCH2WEX)
#define     SHIFT_PCMRXCCH2EN    14
#define     PCMRXCCH2EN          (1 << SHIFT_PCMRXCCH2EN)
#define     SHIFT_PCMRXCCH2POS   4
#define     SHIFT_PCMRXCCH2WID   0
#define   PCMTXC             0x10/4
#define     SHIFT_PCMTXCCH1WEX   31
#define     PCMTXCCH1WEX         (1 << SHIFT_PCMTXCCH1WEX)
#define     SHIFT_PCMTXCCH1EN    30
#define     PCMTXCCH1EN          (1 << SHIFT_PCMTXCCH1EN)
#define     SHIFT_PCMTXCCH1POS   20
#define     SHIFT_PCMTXCCH1WID   16
#define     SHIFT_PCMTXCCH2WEX   15
#define     PCMTXCCH2WEX         (1 << SHIFT_PCMTXCCH2WEX)
#define     SHIFT_PCMTXCCH2EN    14
#define     PCMTXCCH2EN          (1 << SHIFT_PCMTXCCH2EN)
#define     SHIFT_PCMTXCCH2POS   4
#define     SHIFT_PCMTXCCH2WID   0
#define   PCMDREQ            0x14/4
#define   PCMINTEN           0x18/4
#define   PCMINTSTC          0x1c/4
#define   PCMGRAY            0x20/4

#endif // __BCM2835Peri_h__
