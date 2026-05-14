#include "dma.h"
#include "macchiato_log.h"

static int dma_read(MacchiatoState *dev, void *buf, hwaddr addr, hwaddr size)
{
    MemTxResult err;

    if ((err = pci_dma_read(&dev->pci_dev, addr, buf, size)) != MEMTX_OK) {
        macchiato_log("failed dma read: sz=%#" PRIx64 " src_addr=%#" PRIx64
                      "\n",
                      size, addr);
        return 1;
    }

    return 0;
}

int macchiato_dma_read(MacchiatoState *dev, hwaddr addr, hwaddr size)
{

    if (size >= MACCHIATO_DEV_BUF_SZ) {
        macchiato_log("failed dma read: not enough space\n");
        return 1;
    }

    return dma_read(dev, dev->buffer, addr, size);
}

MemTxResult macchiato_dma_write(MacchiatoState *dev, hwaddr dest, void *src,
                                size_t size)
{
    MemTxResult err;

    err = pci_dma_write(&dev->pci_dev, dest, src, size);

    if (err != MEMTX_OK) {
        macchiato_log("failed dma write: sz=%#" PRIx64 " dst_addr=%#" PRIx64
                      "\n",
                      size, dest);
    }
    return 0;
}
