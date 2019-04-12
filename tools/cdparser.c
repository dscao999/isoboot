#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "miscs.h"
#include "mbrop.h"

static const int SECSIZE = 0x800;
static const int BVOL_OFFSET = 0x08800;
static const int V_SECSIZE = 0x200;

struct bootvol {
	u8 bootid;
	u8 isoid[5];
	u8 version;
	u8 eltorito[0x20];
	u8 unused[0x20];
	u32 catoff;
	u8 pad[0x7b5];
} __attribute__((packed));
struct valentry {
	u8 hid;
	u8 pid;
	u16 reserv;
	u8 idstr[0x18];
	u16 checksum;
	u8 a55;
	u8 aa;
} __attribute__((packed));
struct boothead {
	u8 hid;
	u8 pid;
	u16 nument;
	u8 idstr[0x1c];
} __attribute__((packed));
struct bootentry {
	u8 bid;
	u8 mtype;
	u16 lseg;
	u8 stype;
	u8 unused;
	u16 v_count;
	u32 start;
	u8 pad[0x14];
} __attribute__((packed));

static const struct bootvol BV = {
	.bootid = 0,
	.isoid = {'C', 'D', '0', '0', '1'},
	.version = 1,
	.eltorito = {'E', 'L', ' ', 'T', 'O', 'R', 'I', 'T',
			'O', ' ', 'S', 'P', 'E', 'C', 'I', 'F',
			'I', 'C', 'A', 'T', 'I', 'O', 'N'},
};

static int check_valid(struct valentry *val)
{
	u16 *ws = (u16 *)val, *we = (u16 *)(val + 1);
	u16 sum;

	sum = 0;
	while (ws < we) {
		sum += *ws;
		ws++;
	}
	return sum;
}

static int get_bootvol(FILE *iso, struct bootvol *bv)
{
	u32 catoff;
	struct valentry valid;

	fseek(iso, BVOL_OFFSET, SEEK_SET);
	fread(bv, 1, SECSIZE, iso);
	catoff = bv->catoff;
	bv->catoff = 0;
	if (memcmp(bv, &BV, sizeof(struct bootvol)))
		return -1;
	fseek(iso, catoff*SECSIZE, SEEK_SET);
	fread(&valid, sizeof(valid), 1, iso);
	if (check_valid(&valid))
		return -1;
	return catoff;
}

static int get_bootentry(FILE *iso, struct bootentry *bent, u32 catoff)
{
	int bcount;
	struct bootentry *entry = bent;

	bcount = 0;
	do {
		fread(entry, sizeof(struct bootentry), 1, iso);
		if (entry->bid != 0x88)
			break;
		printf("Boot Image. Emulation: %d, Load Seg: %d, Start: %d, Size: %d, Sys Type: %d\n",
				entry->mtype & 7, entry->lseg, entry->start*4, entry->v_count, entry->stype);
		bcount++;
		entry++;
	} while (bcount < 4);

	return bcount;
}

static void iso2udisk(FILE *iso, struct mbrsec *mbr, const char *uimg);

int main(int argc, char *argv[])
{
	FILE *iso;
	u8 *buf, *secbuf;
	struct mbrsec *mbr;
	struct bootvol *bvol;
	struct bootentry *bent;
	u32 catoff;
	int nument, retv = 0, i;
	struct stat *isostat;

	iso = fopen(argv[1], "rb");
	if (!iso) {
		fprintf(stderr, "Cannot open the given iso file: %s\n", argv[1]);
		return 4;
	}

	buf = malloc(3*SECSIZE);
	isostat = (struct stat *)buf;
	mbr = (struct mbrsec *)(buf + SECSIZE);
	secbuf = buf + 2*SECSIZE;
	fstat(fileno(iso), isostat);
	if (isostat->st_size > 0xffffffffu) {
		fprintf(stderr, "The ISO size is too large.\n");
		goto exit_10;
	}
	mbrsec_init(mbr, (u32)isostat->st_size);

	bvol = (struct bootvol *)buf;
	catoff = get_bootvol(iso, bvol);
	if ((int)catoff == -1) {
		fprintf(stderr, "Not a bootable iso!\n");
		retv = 8;
		goto exit_10;
	}

	bent = (struct bootentry *)buf;
	nument = get_bootentry(iso, bent, catoff);
	printf("Total number of boot entries: %d\n", nument);
	for (i = 0; i < nument; i++, bent++) {
		fseek(iso, bent->start*SECSIZE, SEEK_SET);
		fread(secbuf, 1, V_SECSIZE, iso);
		mbrsec_add_image(mbr, bent->start*4, bent->v_count, secbuf);
	}

	iso2udisk(iso, mbr, "udisk.img");

exit_10:
	free(buf);
	fclose(iso);
	return retv;
}

static void iso2udisk(FILE *iso, struct mbrsec *mbr, const char *uimg)
{
	FILE *uf;

	uf = fopen(uimg, "wb");
	fwrite(mbr, sizeof(struct mbrsec), 1, uf);
	fseek(iso, V_SECSIZE, SEEK_SET);
	fread(mbr, V_SECSIZE, 1, iso);
	while (!feof(iso)) {
		fwrite(mbr, V_SECSIZE, 1, uf);
		fread(mbr, V_SECSIZE, 1, iso);
	}
	fclose(uf);
}
