#include <stdio.h>
#include <string.h>
#include "mbrop.h"

void mbrsec_init(struct mbrsec *mbr, u32 isosz)
{
	memset(mbr, 0, sizeof(struct mbrsec));
	mbr->a55 = 0x55;
	mbr->aa = 0xaa;
	mbr->pt[0].status = 0x80;
	mbr->pt[0].start = 0;
	mbr->pt[0].size = isosz;
}

static int check_uefi(const u8 *sec)
{
	const u8 *jboot0 = sec, *jboot2 = sec + 2;
	const u8 *ftype = sec + 54;

	if (*jboot0 != 0xeb && *jboot0 != 0xe9)
		return 0;
	if (*jboot0 == 0xeb && *jboot2 != 0x90)
		return 0;
	if (memcmp((const char *)ftype, "FAT12   ", 8) != 0 &&
			memcmp((const char *)ftype, "FAT16   ", 8) != 0 &&
			memcmp((const char *)ftype, "FAT     ", 8) != 0)
		return 0;
	if (*(sec+510) != 0x55 || *(sec+511) != 0xaa)
		return 0;
	return 1;
}

int mbrsec_add_image(struct mbrsec *mbr, u32 start, u32 size, const u8 *sec)
{
	int i, retv;

	if (check_uefi(sec)) {
		for (i = 0; i < 4; i++)
			if (mbr->pt[i].status != 0x80)
				break;
		if (i == 4)
			return -1;
		mbr->pt[i].status = 0x80;
		mbr->pt[i].start = start;
		mbr->pt[i].size = size;
		printf("An UEFI boot image at: %u, size: %u will be added.\n",
				start, size);
		retv = 1;
	} else {
		printf("A legacy iso boot program is at: %u, size: %u.\n",
				start, size);
		retv = 0;
	}
	return retv;
}
