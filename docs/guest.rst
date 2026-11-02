Setting up the guest
====================

The devices were developed with a debian 13 guest running the
6.12.43+deb13-amd64 kernel. Other kernels should work, but the kernel modules
are built against the running kernel's headers, and volatility needs the
symbols of the exact kernel the dump came from.

The guest gets the whole repository as a shared folder, so code can be edited
on the host and built inside the guest.

1. Installing debian
--------------------

Build QEMU first, from the repository root::

    make build

Create a disk image::

    qemu/build/qemu-img create -f qcow2 debian.qcow2 20G

and boot the debian netinst ISO, available at
`debian.org <https://www.debian.org/distrib/netinst>`_::

    qemu/build/qemu-system-x86_64 -M q35 -accel kvm -m 2G \
        -drive file=debian.qcow2,if=virtio -cdrom debian-13-amd64-netinst.iso

Install debian as usual. On macOS the QEMU binary is called
qemu-system-x86_64-unsigned, and -accel kvm must be replaced by -accel tcg.

make <device> runs the guest without a display, on the serial console. After
installing, enable it inside the guest by setting
GRUB_CMDLINE_LINUX="console=ttyS0" in /etc/default/grub, then::

    update-grub

2. Mounting the shared folder
-----------------------------

make <device> exports the repository with the tag share9p. Inside the guest,
add it to /etc/fstab, using any mount point::

    share9p /mnt/cafemachine 9p trans=virtio,version=9p2000.L,_netdev 0 0

and mount it::

    mkdir -p /mnt/cafemachine
    mount /mnt/cafemachine

3. Installing build tools
-------------------------

To build the kernel modules and userspace programs inside the guest::

    apt-get install build-essential linux-headers-$(uname -r)

Then build a device's guest code from its directory in the shared folder::

    cd /mnt/cafemachine/devices/<device>
    make guest
