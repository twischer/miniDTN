#ifndef _MBR_H_
#define _MBR_H_

struct mbr_primary_partition {
	uint8_t status;
	uint8_t chs_first_sector[3];
	uint8_t type;
	uint8_t chs_last_sector[3];
	uint32_t lba_first_sector;
	uint32_t lba_num_sectors;
};

struct mbr { //ignores everything but primary partitions (saves 448 bytes)
	struct mbr_primary_partition partition[4];
};

int mbr_init(/*device*/);

int mbr_read(/*device from, */struct mbr *to);
int mbr_write(struct mbr *from/*, device to*/);

int mbr_addPartition(struct mbr *mbr/*, type, num, start, len*/);
int mbr_delPartition(struct mbr *mbr/*, num*/);

#endif
