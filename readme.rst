cafemachine
===========

cafemachine is a set of PCIe devices for QEMU, built to explore what hardware
with direct memory access can do for memory forensics and system security.
Each device comes with the software that runs inside the guest: a linux kernel
driver, test programs or tools.

- `cafe <devices/cafe/readme.rst>`_: reads and writes guest memory through
  DMA and dumps the physical address space, controlled by its kernel driver.
- `tea <devices/tea/readme.rst>`_: dumps the physical address space without
  any driver in the guest, triggered over the network.
- `latte <devices/latte/readme.rst>`_: monitors the integrity of guest kernel
  code and panics the guest when it is tampered with.
- `macchiato <devices/macchiato/readme.rst>`_: scans guest data with YARA
  rules, as a malware scanning coprocessor.

1. Building
-----------

Install QEMU's build dependencies, plus libyara and OpenSSL. On debian, with
source repositories enabled::

    apt-get build-dep qemu
    apt-get install libyara-dev libssl-dev

On macOS, with Homebrew::

    brew install $(brew deps --include-build qemu) yara openssl@3

Homebrew doesn't link OpenSSL into the default paths, so point pkg-config to
it before building::

    export PKG_CONFIG_PATH=$(brew --prefix openssl@3)/lib/pkgconfig

Then clone the repository and build QEMU with all the devices::

    git clone https://github.com/nightshade-tea/cafemachine.git
    cd cafemachine
    make build

make build fetches QEMU as a git submodule, links the devices into its source
tree with scripts/patch.sh, and builds qemu-system-x86_64.

2. Running
----------

Start a guest with one of the devices attached::

    make <device> DRIVE=path/to/image.qcow2

ACCEL (default kvm), SMP, RAM and OPT can be set to change how QEMU runs, e.g.
ACCEL=tcg on hosts without KVM.

See `docs/guest.rst <docs/guest.rst>`_ for setting up the guest, and
`docs/volatility.rst <docs/volatility.rst>`_ for analyzing memory dumps from
cafe and tea.

3. Layout
---------

::

    devices/<name>/
        include/        headers shared by the device and the guest code
        src/<name>/     the QEMU device
        src/...         guest and host code: kernel modules, programs
        Makefile        make run, build, guest, host, clean
        readme.rst      what the device does and how to use it
    docs/               guest setup and memory dump analysis
    scripts/            QEMU patching, shared device rules, format configs

Authors
-------

- Pedro Henrique Friedrich Ramos <phfr24@inf.ufpr.br>
  (`nightshade-tea <https://github.com/nightshade-tea>`_)
- Davi Campos Ribeiro <dcr23@inf.ufpr.br>
  (`d4vi13 <https://github.com/d4vi13>`_)
- Theo Simonetti Schult <tss24@inf.ufpr.br>
  (`tss6 <https://github.com/tss6>`_)

Acknowledgements
----------------

- Marcus Botacin <botacin@tamu.edu>
  (`marcusbotacin <https://github.com/marcusbotacin>`_), for proposing the
  research ideas behind the devices
- Jorge Pires Correia <jpcorreia@inf.ufpr.br>
  (`jorge-correia <https://github.com/jorge-correia>`_), for leading the
  project
- Ruibin Mei <rm23@inf.ufpr.br>
  (`rm1003 <https://github.com/rm1003>`_), for testing and documentation
