yara_dep = dependency('yara', required: true)

 system_ss.add(when: 'CONFIG_MACCHIATO', if_true: [
   files(
     'macchiato_device.c',
     'mmio.c',
     'irq.c',
     ),
   yara_dep
   ])
