#include "macchiato_device.h"
#include "irq.h"
#include "macchiato_log.h"
#include "mmio.h"
#include "scan.h"
#include <pthread.h>
#include <yara.h>

/* we only need to do begin_scan in the worker thread, all scan calls are done
 * internally (by yara_scanner_scan_mem_blocks) so the stack/ctx will be of this
 * worker, which, differently from the main thread, can be blocked by semaphores
 * or, in our case, condition variables */
static void *macchiato_wrk(void *opaque)
{
    MacchiatoState *dev = opaque;

    macchiato_log("wrk thread started\n");
    pthread_mutex_lock(&dev->wrk_lock);

    while (dev->wrk_running) {

        /* wait for cmd (begin or exit) */
        while (!dev->begin_scan && dev->wrk_running) {
            pthread_cond_wait(&dev->wrk_cond, &dev->wrk_lock);
        }

        /* asked for exit */
        if (!dev->wrk_running) {
            break;
        }

        /* asked for begin_scan */

        dev->begin_scan = false;
        pthread_mutex_unlock(&dev->wrk_lock);

        macchiato_log("wrk: beginning yara scan...\n");
        begin_scan(dev);

        pthread_mutex_lock(&dev->wrk_lock);
    }

    pthread_mutex_unlock(&dev->wrk_lock);
    macchiato_log("wrk thread exiting\n");
    return NULL;
}

/* instantiate a macchiato device object (MacchiatoState) */
static void macchiato_realize(PCIDevice *pci_dev, Error **errp)
{
    MacchiatoState *dev = MACCHIATO_DEVICE(pci_dev);

    /* set pci express capability on pci configuration space */
    if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
        pcie_endpoint_cap_init(pci_dev, 0) >= 0) {
        macchiato_log("express capability set\n");
    }

    macchiato_mmio_init(dev, errp);
    macchiato_irq_init(dev, errp);

    dev->compiler = NULL;
    dev->scanner = NULL;

    /* start wrk thread */
    dev->wrk_running = true;
    dev->begin_scan = false;
    pthread_mutex_init(&dev->wrk_lock, NULL);
    pthread_cond_init(&dev->wrk_cond, NULL);

    if (pthread_create(&dev->wrk_thread, NULL, macchiato_wrk, dev) != 0) {
        macchiato_log("failed to create wrk thread\n");
    }

    macchiato_log("device realized\n");
}

static void macchiato_exit(PCIDevice *pci_dev)
{
    MacchiatoState *dev = MACCHIATO_DEVICE(pci_dev);

    macchiato_log("device exiting, joining wrk...\n");

    /* make wrk exit */
    pthread_mutex_lock(&dev->wrk_lock);
    dev->wrk_running = false;
    pthread_cond_signal(&dev->wrk_cond);
    pthread_mutex_unlock(&dev->wrk_lock);

    /* wait for it */
    pthread_join(dev->wrk_thread, NULL);

    /* free stuff (needed?) */
    pthread_mutex_destroy(&dev->wrk_lock);
    pthread_cond_destroy(&dev->wrk_cond);

    /* TODO: check what else needs deinit/free */
    // macchiato_irq_deinit(dev); // for example

    macchiato_log("device unrealized\n");
}

/* initialize macchiato device class (MacchiatoClass) methods and static
 * attributes */
static void macchiato_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = macchiato_realize;
    pci_device_class->exit = macchiato_exit;
    pci_device_class->vendor_id = MACCHIATO_VENDOR_ID;
    pci_device_class->device_id = MACCHIATO_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    if (yr_initialize()) {
        macchiato_log("failed to initialize yara\n");
    }

    macchiato_log("class initialized\n");
}

/* register macchiato device type on qemu (Class + State) */
static const TypeInfo macchiato_info = {
    .name = TYPE_MACCHIATO_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(MacchiatoState),
    .class_init = macchiato_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void macchiato_register_types(void)
{
    type_register_static(&macchiato_info);
}

type_init(macchiato_register_types)
