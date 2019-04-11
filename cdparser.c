#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

const int SECSIZE = 0x800;
const int BVOL_OFFSET = 0x08800;

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
};
struct boothead {
	u8 hid;
	u8 pid;
	u16 nument;
	u8 idstr[0x1c];
};
struct bootentry {
	u8 bid;
	u8 mtype;
	u16 lseg;
	u8 stype;
	u8 unused;
	u16 vsec_count;
	u32 vdstart;
	u8 pad[0x14];
};

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
				entry->mtype & 7, entry->lseg, entry->vdstart*4, entry->vsec_count, entry->stype);
		bcount++;
		entry++;
	} while (1);

	return bcount;
}

int main(int argc, char *argv[])
{
	FILE *iso;
	u8 *buf;
	struct bootvol *bvol;
	struct bootentry *bent;
	u32 catoff;
	int nument, retv = 0;

	iso = fopen(argv[1], "rb");
	if (!iso) {
		fprintf(stderr, "Cannot open the given iso file: %s\n", argv[1]);
		return 4;
	}

	buf = malloc(SECSIZE);
	bvol = (struct bootvol *)buf;
	catoff = get_bootvol(iso, bvol);
	if ((int) catoff == -1) {
		fprintf(stderr, "Not a bootable iso!\n");
		retv = 8;
		goto exit_10;
	}

	bent = (struct bootentry *)buf;
	nument = get_bootentry(iso, bent, catoff);
	printf("Total number of boot entries: %d\n", nument);

exit_10:
	free(buf);
	fclose(iso);
	return retv;
}
