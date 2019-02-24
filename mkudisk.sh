#!/bin/bash
#
#[ $UID -eq 0 ] || { echo "Must be root!"; exit 1; }
#
target=$1
[ -z "$target" ] && { echo "A USB disk device file must be specified."; exit 1; }
devnam=$(basename $target)
[ -b $target ] || { echo "Not a disk device file"; exit 2; }
#
eval $(udevadm info --name=$target | awk '/ID_/ {print $2}')
[ "$ID_BUS" = "usb" ] || { echo "Not a USB disk"; exit 3; }
dsize=$(cat /sys/block/$devnam/size)
echo "UDisk Size: $((dsize/2/1024/1024))GiB. All data will be destroyed!"
read -p "Continue? " answ
[[ "$answ" == "y" || "$answ" == "Y" ]] || exit 0
#
if mount | fgrep $devnam > /dev/null 2>&1
then
	mount | fgrep sdc | awk '{print $1}' | xargs umount -f 
fi
#
echo "Wipe disk blank! ..."
dd if=/dev/zero bs=1M count=128 2> /dev/null |
	dd iflag=fullblock bs=1M of=$target oflag=direct 2> /dev/null
#
p1start=2048      # 1MiB
lvsize=6291456    # 3GiB
lvstart=$((dsize-lvsize+1))
p1size=$((dsize-p1start-lvsize))
echo "p1: $p1start $p1size, live: $lvstart $lvsize"
#
sfdisk --no-reread --no-tell-kernel $target <<-EOD
unit: sectors
label: dos
$p1start $p1size 7 -
$lvstart $lvsize c *
EOD
#
partprobe $target > /dev/null 2>&1
dpart2=${target}2
echo -n "Waiting for partition device ."
sleep 1
while [ ! -b $dpart2 ]
do
	echo -n .
	sleep 1
	partprobe $target > /dev/null 2>&1
done
echo
#
mkfs.vfat -n CustomMedia $dpart2
udmount=/tmp/udisk$$
mkdir $udmount
mount $dpart2 $udmount
#
lines=73
tail -n +$lines $0 | tar -Jxf -
inf=${PWD}/boot-data/media.cpio
pushd $udmount > /dev/null 2>&1
echo "Copying system files, please wait for a moment ..."
cpio -id < $inf > /dev/null 2>&1
popd > /dev/null 2>&1
umount $dpart2
echo "Done!"
rmdir $udmount
#
dd if=boot-data/bios-boot.bin of=/dev/${devnam} bs=1 > /dev/null 2>&1
dd if=boot-data/grub-second.bin of=/dev/${devnam} bs=512 seek=1 > /dev/null 2>&1
sync $target
#
rm -r boot-data
exit 0
