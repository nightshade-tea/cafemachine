#include "tea_device.h"
#include "dump.h"
#include "tea_log.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

static void *dump_thread_start(void *dev)
{
    struct sockaddr_in serv_addr, cli_addr;
    int listen_sock, conn;

#if TEA_HOTPLUG_DUMP
    tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
#endif

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        tea_log("failed to create socket : %s\n", strerror(errno));
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(TEA_SERVER_PORT);

    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof serv_addr) <
        0) {
        tea_log("failed to bind socket : %s\n", strerror(errno));
        return 0;
    }

    if (listen(listen_sock, 1) < 0) {
        tea_log("failed to listen socket : %s\n", strerror(errno));
    }

    for (;;) {
        struct tea_msg msg;
        socklen_t clilen;
        errno = 0;

        tea_log("waiting for connection \n");

        clilen = sizeof cli_addr;
        if ((conn = accept(listen_sock, (struct sockaddr *)&cli_addr,
                           &clilen)) < 0) {
            tea_log("accept() failed : %s\n", strerror(errno));
            continue;
        }

        tea_log("connection accepted\n");

        ssize_t bytes_recv;
        while ((bytes_recv = recv(conn, &msg, sizeof msg, 0)) > 0) {

            tea_log("bytes recv %zd\n", bytes_recv);

            if (msg.magic != TEA_MAGIC) {
                tea_log("invalid message received. ignoring\n");
                continue;
            }

            switch (msg.cmd) {
            case TEA_CMD_DUMP:
                tea_log("taking a dump\n");
                tea_dump_mem_strict(dev, TEA_DEFAULT_DUMP_BASENAME, 0);
                break;
            default:
                tea_log("invalid command\n");
                break;
            }
        }

        tea_log("after while, bytes recv %zd\n", bytes_recv);

        if (errno) {
            tea_log("recv() failed : %s\n", strerror(errno));
        }

        close(conn);
    }

    close(listen_sock);

    return 0;
}

/* instantiate a tea device object (TeaState) */
static void tea_realize(PCIDevice *pci_dev, Error **errp)
{
    TeaState *dev = TEA_DEVICE(pci_dev);
    uint16_t cmd, wmask, cmask;
    pthread_t dump_thread;
    int err;

    /* set pci express capability on pci configuration space */
    if (pci_bus_is_express(pci_get_bus(pci_dev)) &&
        pcie_endpoint_cap_init(pci_dev, 0) >= 0) {
        tea_log("express capability set\n");
    }

    /* enable bus mastering */
    memory_region_set_enabled(&pci_dev->bus_master_enable_region, true);

    /* mask bus master bit to avoid OS overwrite */
    cmd = pci_get_word(pci_dev->config + PCI_COMMAND);
    cmd |= PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->config + PCI_COMMAND, cmd);

    cmask = pci_get_word(pci_dev->cmask + PCI_COMMAND);
    cmask |= PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->cmask + PCI_COMMAND, cmask);

    wmask = pci_get_word(pci_dev->wmask + PCI_COMMAND);
    wmask &= ~PCI_COMMAND_MASTER;
    pci_set_word(pci_dev->wmask + PCI_COMMAND, wmask);

    /* hotplug dump */
    if ((err = pthread_create(&dump_thread, NULL, dump_thread_start, dev))) {
        tea_log("failed to create dump thread: %s\n", strerror(err));
    }

    tea_log("device realized\n");
}

/* initialize tea device class (TeaClass) methods and static attributes */
static void tea_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = tea_realize;
    pci_device_class->vendor_id = TEA_VENDOR_ID;
    pci_device_class->device_id = TEA_DEVICE_ID;
    pci_device_class->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);

    tea_log("class initialized\n");
}

/* register tea device type on qemu (Class + State) */
static const TypeInfo tea_info = {
    .name = TYPE_TEA_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(TeaState),
    .class_init = tea_class_init,
    .interfaces = (InterfaceInfo[]){{INTERFACE_PCIE_DEVICE}, {}},
};

static void tea_register_types(void)
{
    type_register_static(&tea_info);
}

type_init(tea_register_types)
