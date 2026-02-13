openssl_dep = dependency('libssl', required: true)
libcrypto_dep = dependency('libcrypto', required: true)

 system_ss.add(when: 'CONFIG_LATTE', if_true: [
   files(
     'latte_device.c',
     'mmio.c',
     'irq.c',
     'dma.c',
     'mon.c',
     'dump.c'
     ),
   openssl_dep,  
   libcrypto_dep 
   ])
