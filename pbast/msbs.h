#ifndef	_MSBS_H_
#define	_MSBS_H_

#define TRUE	1
#define	FALSE	0

#define MODE_FILE   1
#define MODE_CACHE  0

#define	SECTOR_SIZE					512			
#define	SPARE_SIZE					16
#define	PAGE_SIZE					(SECTOR_SIZE+SPARE_SIZE)
#define	PAGES_PER_BLOCK				32
#define SECTORS_PER_BLOCK			(PAGES_PER_BLOCK)
#define	BLOCK_SIZE					(PAGE_SIZE*PAGES_PER_BLOCK)
#define	BLOCKS_PER_DEVICE			4500/*4096*/
#define	DEVICE_SIZE					(BLOCK_SIZE*BLOCKS_PER_DEVICE)

#define INPLACE_PART_RATIO			0.5
#define	INPLACE_SECTORS_PER_BLOCK	(int)(SECTORS_PER_BLOCK * INPLACE_PART_RATIO)
#define	INPLACE_PAGES_PER_BLOCK		INPLACE_SECTORS_PER_BLOCK

#define SPAREBLOCKS_PER_DEVICE		1
#define	REALBLOCKS_PER_DEVICE		(BLOCKS_PER_DEVICE - SPAREBLOCKS_PER_DEVICE)

#define	STATE_F			0xFF
#define	STATE_U			0xFE

/* type definition */
typedef char			byte;
typedef int				o_4b;
typedef unsigned int	o_u4b;
typedef int				o_bool;

typedef enum {BLOCK_FULL, LSN_CONFLICT, BLOCK_AVAIL} BLOCK_INSIDE;

typedef struct
{
	o_4b lbn;
	o_4b lsn;
	byte block_state;
	byte dummy[SPARE_SIZE - 9];
} SpareData;

typedef struct _block{
	int* spare[PAGES_PER_BLOCK];
	struct _block* logblock;

}BLOCK;

typedef struct
{
	o_4b L2P[BLOCKS_PER_DEVICE - 1];
	byte block_state[BLOCKS_PER_DEVICE];
	o_4b spare_pblock_num;
} RAMTABLE;

#endif
