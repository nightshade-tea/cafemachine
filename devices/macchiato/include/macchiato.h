#ifndef MACCHIATO_H
#define MACCHIATO_H

/* device attributes */
#define MACCHIATO_HW_NAME "macchiato"
#define MACCHIATO_VENDOR_ID 0x1ced
#define MACCHIATO_DEVICE_ID 0x4acc
#define MACCHIATO_DEV_BUF_SZ (64 * 1024 * 1024) /* dma buf */

/* mmio settings */
enum macchiato_bar_id {
    MACCHIATO_BAR_ID_CTL, /* control registers */
    MACCHIATO_BAR_ID_MSIX,

    MACCHIATO_BAR_ID_CNT
};

/* MACCHIATO_BAR_ID_CTL registers */
enum macchiato_reg {
    MACCHIATO_REG_CMD,       /* In */
    MACCHIATO_REG_LOWER_IRQ, /* In */
    MACCHIATO_REG_ADDR,      /* In */
    MACCHIATO_REG_SIZE,      /* In/Out */
    MACCHIATO_REG_STATUS,    /* Out */
    MACCHIATO_REG_INT_TEST,  /* debugging: write interrupt number to this
                                register to trigger it */

    MACCHIATO_REG_CNT
};

/* interrupt vector values */
enum macchiato_irq {
    MACCHIATO_IRQ_DONE,   /* no error */
    MACCHIATO_IRQ_FAILED, /* error, check status register */
    MACCHIATO_IRQ_NEXT_BLOCK,
    MACCHIATO_IRQ_FIRST_BLOCK,
    MACCHIATO_IRQ_CNT
};

/* device operations (values for MACCHIATO_CMD register) */
enum macchiato_op {
    /* reset yara compiler internal state, reset rules
     *
     * no args. */
    MACCHIATO_OP_RESET_COMPILER,

    /* reset yara scanner internal state, reset results data
     *
     * no args. */
    MACCHIATO_OP_RESET_SCANNER,

    /* add rule
     *
     * in: addr, size
     * out: status */
    MACCHIATO_OP_ADD_RULE,

    /* scan user buffer
     *
     * in: addr, size
     * out: status, size
     *
     * obs: size of serialized results is written to size */
    MACCHIATO_OP_SCAN_BUFFER,

    /* write serialized results to user buffer
     *
     * in: addr, size
     * out: status */
    MACCHIATO_OP_GET_RESULTS,

    MACCHIATO_OP_CNT
};

/* correct order of operations:
 *
 * RESET_COMPILER :: once
 * RESET_SCANNER  :: once
 * ADD_RULES      :: n times
 * SCAN_BUFFER    :: n times
 * GET_RESULTS    :: once
 *
 */

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define MACCHIATO_IOC_MAGIC 'Y'

#define MACCHIATO_IOC_RESET_COMPILER _IO(MACCHIATO_IOC_MAGIC, 0)
#define MACCHIATO_IOC_RESET_SCANNER _IO(MACCHIATO_IOC_MAGIC, 1)
#define MACCHIATO_IOC_ADD_RULE _IO(MACCHIATO_IOC_MAGIC, 2)
#define MACCHIATO_IOC_SCAN_BUFFER _IO(MACCHIATO_IOC_MAGIC, 3)
#define MACCHIATO_IOC_GET_RESULTS _IO(MACCHIATO_IOC_MAGIC, 4)

#endif /* MACCHIATO_H */
