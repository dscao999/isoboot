#ifndef MBROP_DSCAO__
#define MBROP_DSCAO__
#include "miscs.h"

struct partition {
	u8 status;
	u8 f_chs[3];
	u8 type;
	u8 l_chs[3];
	u32 start;
	u32 size;
} __attribute__((packed));

struct mbrsec {
	u8 pcode[446];
	struct partition pt[4];
	u8 a55;
	u8 aa;
} __attribute__((packed));

void mbrsec_init(struct mbrsec *mbr, u32 isosz);
int mbrsec_add_image(struct mbrsec *mbr, u32 start, u32 size, const u8 *sec);

#endif  /* MBROP_DSCAO__ */
