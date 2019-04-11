#!/bin/bash
#
#genisoimage -r -iso-level 3 -jcharset utf-8 -o boot.iso \
genisoimage -r -J --joliet-long -l -input-charset utf-8 -o boot.iso \
		-V Fedora-LXDE-Live-29-Lenovo \
		-b boot/grub/bootcd.bin -boot-info-table -c boot/grub/boot.cat -no-emul-boot \
		-eltorito-alt-boot -b boot/grub/efiboot.img -no-emul-boot \
		${1}
