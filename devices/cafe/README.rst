cafe device
===========

cafe is a PCIe device for QEMU that can read and write guest memory through
DMA, and dump the physical address space to a file on the host machine. It
comes with a linux kernel driver that exposes it to userspace as /dev/cafe.

The device has a 4 KiB MMIO register space, a 2 MiB DMA buffer and MSI
interrupts. The driver maps the registers to userspace through mmap, and
triggers memory dumps through the ioctl operation CAFE_IOCTL_DUMP_MEM.

1. Running
----------

Build QEMU and start a guest with the cafe device attached

    make cafe DRIVE=path/to/image.qcow2

See `docs/guest.rst <../../docs/guest.rst>`_ for setting up the guest.

2. Building the driver and tests
--------------------------------

Inside the guest, from the shared folder

    cd devices/cafe
    make guest
    insmod src/kmod/cafe.ko

3. Dumping memory
-----------------

Run dump inside the guest

    ./src/user/dump

The dump is written by QEMU on the host, as mem.dat in the directory QEMU was
started from (devices/cafe when using make cafe). See
`docs/volatility.rst <../../docs/volatility.rst>`_ for analyzing it.
