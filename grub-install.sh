grub-install --target=i386-pc --boot-directory=/media/udisk/boot \
	--modules=serial --modules=terminal --modules=usb \
	--modules=fat --modules=iso9660 --modules=ext2 \
	--modules=part_msdos --modules=part_gpt \
	--modules=search_fs_uuid_disk --modules=search --modules=search_label --modules=search_fs_file \
	/dev/sdc
