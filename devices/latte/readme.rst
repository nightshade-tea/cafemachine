latte device
============

latte is a PCIe device for QEMU that monitors the integrity of guest kernel
code. It reads protected memory regions through DMA, hashes them with SHA-256
every 5 seconds, and raises an NMI when a hash changes. The linux kernel driver
registers the regions with the device and panics the guest when it gets the
NMI.

The driver currently protects x64_sys_call, the kernel function that
dispatches system calls, a common target for rootkits.

1. Running
----------

Build QEMU and start a guest with the latte device attached::

    make latte DRIVE=path/to/image.qcow2

See `docs/guest.rst <../../docs/guest.rst>`_ for setting up the guest.

2. Building and loading the driver
----------------------------------

Inside the guest, from the shared folder::

    cd devices/latte
    make guest
    insmod src/kmod/latte.ko

Once loaded, the device starts checking the registered regions. If any of them
is modified, the guest panics with::

    SYSTEM INTEGRITY VIOLATION: kernel code tampering detected

3. Testing with sugar
---------------------

sugar is a proof of concept linux rootkit that hides files and directories
named "sugarcoated". It hooks the getdents64 system call by patching the
kernel code of x64_sys_call, the region latte protects.

With the latte driver loaded, insert sugar from devices/latte::

    insmod src/sugar/sugar.ko

Within 5 seconds, latte detects the modified code and the guest panics with
the integrity violation message above.
