#!/bin/bash
#
genisoimage -r -iso-level 3 -jcharset utf-8 -o boot.iso \
		-c boot.cat \
		-no-emul-boot -boot-info-table -b boot/grub/bootcd.bin \
		-eltorito-alt-boot -no-emul-boot -b boot/grub/efi.img \
		${1}
