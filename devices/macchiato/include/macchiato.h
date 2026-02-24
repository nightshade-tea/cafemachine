#ifndef MACCHIATO_H
#define MACCHIATO_H

/* device attributes */
#define MACCHIATO_HW_NAME "macchiato"
#define MACCHIATO_VENDOR_ID 0x1ced
#define MACCHIATO_DEVICE_ID 0x4acc

/* mmio settings */

enum macchiato_bar_id {
    MACCHIATO_BAR_ID_CTL,
    MACCHIATO_BAR_ID_TRANSFER,
    MACCHIATO_BAR_ID_MSIX,
    MACCHIATO_BAR_ID_CNT
};

/* BAR 0 -> device control registers */
#define MACCHIATO_MMIO_BAR0_NAME "macchiato-mmio-ctl"
#define MACCHIATO_MMIO_BAR0_SIZE 4096
#define MACCHIATO_MMIO_BAR0_MIN_ACCESS_SIZE 8
#define MACCHIATO_MMIO_BAR0_MAX_ACCESS_SIZE 8

enum macchiato_reg {
    MACCHIATO_REG_CMD,
    MACCHIATO_REG_STATUS,
    MACCHIATO_REG_LOWER_IRQ,
    /* ... */
    MACCHIATO_REG_CNT
};

/* BAR 1 -> direct data transfers */
#define MACCHIATO_MMIO_BAR1_NAME "macchiato-mmio-transfer"
#define MACCHIATO_MMIO_BAR1_SIZE (64 * 1024 * 1024)

/* BAR 2 -> msix */
#define MACCHIATO_MMIO_BAR2_NAME "macchiato-msix"
#define MACCHIATO_MMIO_BAR2_SIZE 4096

/* interrupt vector values */
enum macchiato_irq { MACCHIATO_IRQ_SYNC, MACCHIATO_IRQ_CNT };

/* device operations (values for MACCHIATO_CMD register) */
enum macchiato_op {
    /* register rules, scan file(s) ... */
    MACCHIATO_OP_CNT
};

#endif /* MACCHIATO_H */
