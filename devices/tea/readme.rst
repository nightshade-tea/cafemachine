tea device
==========

tea is a PCIe device for QEMU that dumps the guest physical address space
through DMA, without any driver in the guest. It forces bus mastering on, so
the guest can't stop it from reading memory, and is controlled entirely from
the host: dumps are triggered by hotplugging the device or by a client over
the network.

tea does not work if the guest has an IOMMU (e.g. QEMU's intel-iommu device),
since the guest then controls which addresses the device can reach. make tea
doesn't add one.

1. Running
----------

Build QEMU and start a guest with an empty PCIe root port for tea::

    make tea DRIVE=path/to/image.qcow2

See `docs/guest.rst <../../docs/guest.rst>`_ for setting up the guest.

2. Hotplugging
--------------

Press Ctrl-A c to switch to the QEMU monitor, and plug the device in::

    device_add tea,bus=rp1

tea immediately dumps memory to teadump.dat, in the directory QEMU was
started from (devices/tea when using make tea), and starts a server on host
port 7734.

The dump on hotplug can be turned off, or tuned, in include/tea.h:
TEA_HOTPLUG_DUMP toggles it, TEA_HOTPLUG_DUMP_START_ADDR sets the first
address, and TEA_HOTPLUG_DUMP_PAGES_ON_ERR retries failed reads page by page.

3. Dumping with the client
--------------------------

On the host, build the client from devices/tea::

    make host

Trigger a dump, then download it::

    src/client/client dump --basename mem localhost
    src/client/client get --basename mem localhost

This writes mem.dat in the current directory. Dumps are strict by default:
without --end, they read up to the highest possible physical address, and stop
after too many consecutive failed reads (--tolerance). --relaxed ignores failed
reads, and needs --end. --start and --end limit the physical address range.
Run src/client/client help dump for all options.

See `docs/volatility.rst <../../docs/volatility.rst>`_ for analyzing dumps.
