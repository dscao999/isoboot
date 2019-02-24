format=i386-pc-eltorito
oimage=bootcd.bin
grublib="-d grublib/i386-pc"
#format=x86_64-efi
#oimage=X86_64.EFI
#grublib=
grub-mkimage --output=$oimage --format=$format  --prefix=/boot/grub $grublib \
	fat iso9660 ext2 \
	biosdisk nativedisk disk \
	serial terminal part_msdos part_gpt \
	search search_label search_fs_file search_fs_uuid disk
#
#  --format=i386-pc-eltorito for CD/DVD boot
#
