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

#define LOGBLOCKS_PER_DEVICE		8
#define	REALBLOCKS_PER_DEVICE		(BLOCKS_PER_DEVICE - LOGBLOCKS_PER_DEVICE)

#define	STATE_F			0xFF
#define	STATE_U			0xFE

/* type definition */
typedef char			byte;
typedef int				o_4b;
typedef unsigned int	o_u4b;
typedef int				o_bool;

typedef enum {BLOCK_FULL, LSN_CONFLICT, BLOCK_AVAIL} BLOCK_INSIDE;
typedef enum {NEW_PAGE = -1, VALID_PAGE = 1, INVAL_PAGE= 2} PAGE_STATE;

typedef struct{
	o_4b lbn;
	o_4b lsn;
	byte block_state;
	PAGE_STATE page_state;
	byte dummy[SPARE_SIZE - 5];
} SpareData;

typedef struct{
	int lbn;
	int first_bn;
	int log_bn;
} _logblock;
typedef struct{
	_logblock logblock[LOGBLOCKS_PER_DEVICE];
	int count;
} LOGBLOCK_INFO;

typedef struct{
	o_4b L2P[BLOCKS_PER_DEVICE];
	o_4b L2L[BLOCKS_PER_DEVICE];
	byte block_state[BLOCKS_PER_DEVICE];
	o_4b spare_pblock_num;

} RAMTABLE;

#endif
