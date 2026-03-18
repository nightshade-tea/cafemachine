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

    if (size >= MACCHIATO_INTERNAL_BUFFER) {
        macchiato_log("failed dma read: not enough space\n");
        return 1;
    }

    return dma_read(dev, dev->buffer, addr, size);
}

MemTxResult macchiato_dma_write(MacchiatoState *dev)
{
    MemTxResult err;

    // TODO
    /*
    err = pci_dma_write(&dev->pci_dev, dev->reg[MACCHIATO_DMA_DST],
    dev->dma_buf, dev->reg[MACCHIATO_DMA_SZ]);

    if (err != MEMTX_OK)
      macchiato_log("failed dma write: sz=%#" PRIx64 " dst_addr=%#" PRIx64 "\n",
               dev->reg[MACCHIATO_DMA_SZ], dev->reg[MACCHIATO_DMA_DST]);
               */

    return 0;
}
