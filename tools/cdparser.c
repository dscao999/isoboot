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

static int get_boot_section(FILE *iso, struct bootentry *entry, int nument)
{
	int i, bcount;

	bcount = 0;
	for (i = 0; i < nument; i++) {
		fread(entry, sizeof(struct bootentry), 1, iso);
		if (entry->mtype != 0 || entry->bid != 0x88)
			continue;
		entry++;
		bcount++;
	}
	return bcount;
}

static int get_bootentry(FILE *iso, struct bootentry *bent, u32 catoff)
{
	int bcount, nument, last;
	struct bootentry *entry = bent;

	bcount = 0;
	do {
		fread(entry, sizeof(struct bootentry), 1, iso);
		if (entry->bid != 0x88)
			break;
		if (entry->mtype != 0)
			continue;
		bcount++;
		entry++;
	} while (bcount < 4);
	if (entry->bid == 0x90 || entry->bid == 0x91) {
		last =	(entry->bid == 0x91)? 1 : 0;
		do {
			nument = get_boot_section(iso, entry, entry->lseg);
			bcount += nument;
			entry += nument;
			if (last == 1)
				break;
			fread(entry, sizeof(struct bootentry), 1, iso);
			last =	(entry->bid == 0x91)? 1 : 0;
		} while (1);
	}

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
	const char *uimage;
	extern char *optarg;
	extern int optind, opterr, optopt;
	int c, fin;

	opterr = 0;
	uimage = NULL;
	fin = 0;
	do {
		c = getopt(argc, argv, ":o:");
		switch(c) {
		case -1:
			fin = 1;
			break;
		case '?':
			fprintf(stderr, "Unknown option: %c\n", (char)optopt);
			break;
		case ':':
			fprintf(stderr, "Missiong argument for option: %c\n",
					(char)optopt);
			break;
		case 'o':
			uimage = optarg;
			break;
		}
	} while (fin == 0);

	if (optind == argc) {
		fprintf(stderr, "Usage: %s [-o uimage] isoimage\n", argv[0]);
		return 104;
	}

	iso = fopen(argv[optind], "rb");
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

	if (uimage)
		iso2udisk(iso, mbr, uimage);

exit_10:
	free(buf);
	fclose(iso);
	return retv;
}

static void iso2udisk(FILE *iso, struct mbrsec *mbr, const char *uimg)
{
	FILE *uf;

	uf = fopen(uimg, "wb");
	if (!uf) {
		fprintf(stderr, "Cannot open file %s for write.\n", uimg);
		return;
	}
	fwrite(mbr, sizeof(struct mbrsec), 1, uf);
	fseek(iso, V_SECSIZE, SEEK_SET);
	fread(mbr, V_SECSIZE, 1, iso);
	while (!feof(iso)) {
		fwrite(mbr, V_SECSIZE, 1, uf);
		fread(mbr, V_SECSIZE, 1, iso);
	}
	fclose(uf);
}
