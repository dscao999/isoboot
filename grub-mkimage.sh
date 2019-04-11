#!/bin/bash
#
target=$1
[ -z "$target" ] && target=uefi
case $target in
uefi)
	echo "An UEFI grub boot image will be built."
	format=x86_64-efi
	oimage=X86_64.EFI
	grublib="grublib/grub-pc/usr/lib/grub/x86_64-efi"
	biosdisk=
	;;
legacy)
	echo "An iso grub boot image will be built."
	format=i386-pc-eltorito
	oimage=bootcd.bin
	grublib="grublib/grub-pc/usr/lib/grub/i386-pc"
	biosdisk=biosdisk
	;;
*)
	echo "Unknown grub target!"
	exit 1
esac
#
grub-mkimage --output=$oimage --format=$format  --prefix=/boot/grub -d $grublib \
	-c early-grub.cfg \
	normal fat iso9660 ext2 \
	$biosdisk nativedisk disk \
	serial terminal part_msdos part_gpt \
	search search_label search_fs_file search_fs_uuid
#
#  --format=i386-pc-eltorito for CD/DVD boot
#
