macchiato device
================

macchiato is a PCIe device for QEMU that scans guest data with YARA rules,
working as a malware scanning coprocessor. The guest sends rules and data
through DMA, the device runs the scan on the host with libyara, and returns
the names of the matching rules.

The linux kernel driver exposes the device as /dev/macchiato0, one user at a
time. Userspace maps the DMA buffer with mmap, writes rules and data into it,
and drives the scan with ioctl operations: ADD_RULE, BEGIN_SCAN,
CONTINUE_SCAN, REWIND_SCAN, END_SCAN and GET_RESULTS. Files larger than the
buffer are sent in chunks, and the device asks for the next chunk, or for the
first one again, as YARA needs them.

1. Running
----------

Building QEMU with macchiato needs libyara installed on the host. Build QEMU
and start a guest with the macchiato device attached::

    make macchiato DRIVE=path/to/image.qcow2

See `docs/guest.rst <../../docs/guest.rst>`_ for setting up the guest.

2. Building and loading the driver
----------------------------------

Inside the guest, from the shared folder::

    cd devices/macchiato
    make guest
    insmod src/kmod/macchiato.ko

3. Scanning a file
------------------

scan takes a file with YARA rules and a file to scan, and prints whether each
rule matched. make guest builds it. For example, from devices/macchiato::

    echo 'rule milk { strings: $a = "MILK" condition: $a }' > rules.yar
    echo 'some data with MILK in it' > target
    src/user/scan rules.yar target

which prints::

    match    milk
