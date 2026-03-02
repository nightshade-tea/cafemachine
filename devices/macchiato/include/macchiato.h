#ifndef MACCHIATO_H
#define MACCHIATO_H

/* device attributes */
#define MACCHIATO_HW_NAME "macchiato"
#define MACCHIATO_VENDOR_ID 0x1ced
#define MACCHIATO_DEVICE_ID 0x4acc
#define MACCHIATO_INTERNAL_BUFFER (64 * 1024 * 1024)

/* mmio settings */
enum macchiato_bar_id {
    MACCHIATO_BAR_ID_CTL,
    MACCHIATO_BAR_ID_MSIX,

    MACCHIATO_BAR_ID_CNT
};

/* MACCHIATO_BAR_ID_CTL registers */
enum macchiato_reg {
    MACCHIATO_REG_CMD,
    MACCHIATO_REG_LOWER_IRQ,
    /* ... */

    MACCHIATO_REG_CNT
};

/* interrupt vector values */
enum macchiato_irq {
    MACCHIATO_IRQ_COMPILER_OK,
    MACCHIATO_IRQ_COMPILER_ERR,
    MACCHIATO_IRQ_SCANNER_MATCH,
    MACCHIATO_IRQ_CONTINUE, /* MACCHIATO_OP_SCAN_BUFFER succeeded, continue */

    MACCHIATO_IRQ_CNT
};

/* device operations (values for MACCHIATO_CMD register) */
enum macchiato_op {
    MACCHIATO_OP_RESET_COMPILER,      /* yr_compiler_create() */
    MACCHIATO_OP_COMPILER_ADD_STRING, /* yr_compiler_add_string() */

    MACCHIATO_OP_RESET_SCANNER, /* yr_scanner_create() */
    MACCHIATO_OP_SCAN_BUFFER,   /* yr_rules_scan_mem() */

    MACCHIATO_OP_CNT
};

#endif /* MACCHIATO_H */
