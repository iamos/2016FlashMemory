//#define	PRINT_FOR_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include "msbs.h"

RAMTABLE ramtable;
extern FILE *devicefp;
extern int read_page_num;
extern int write_page_num;
extern int erase_block_num;

/****************  proto-types ****************/
void ftl_open();
o_bool ftl_read(byte *sectorbuf, o_4b lsectornum);
void ftl_write(byte *sectorbuf, o_4b lsectornum);
void swap_copy(o_4b src_block_num, o_4b dest_block_num, o_4b lsector_num);
void smart_copy(o_4b src_block_num, o_4b dest_block_num, o_4b lsector_num);
o_4b read_pagebuf_with_inplace_sector(byte *buf, o_4b pblocknum, o_4b lsectornum);
int get_physicalpage_num(o_4b pblocknum, o_4b lsectornum);
BLOCK_INSIDE is_block_available(o_4b lsector_num, int *next_outofplace_offset);
void update_block_info(o_4b pblock_num, o_4b lblock_num, byte block_state);
o_4b merge(o_4b lsector_num);
o_4b allocate_f_block();
//void erase_block(o_4b pblock_num);
o_bool populate_init_database();
void print_ramtable_info();
void print_block_info(o_4b pblocknum);

void ftl_open()
{
	byte *pagebuf;
	SpareData *sdata;
	o_4b pagenum;
	int i;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		ramtable.L2P[i] = -1;
	}

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		pagenum = i * PAGES_PER_BLOCK;
		read(pagebuf, pagenum, MODE_FILE);
		memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE); 

		ramtable.block_state[i] = sdata->block_state;

		if(ramtable.block_state[i] == (byte)STATE_U)
		{
			ramtable.L2P[sdata->lbn] = i;
		}
	}

	ramtable.spare_pblock_num = -1;
	ramtable.spare_pblock_num = allocate_f_block();
	assert(ramtable.spare_pblock_num >= 0);

#ifdef PRINT_FOR_DEBUG
	printf("+++++++ Ram Table Info +++++++\n");
	print_ramtable_info();
#endif

	free(pagebuf);
	free(sdata);

	read_page_num = 0;
	write_page_num = 0;
	erase_block_num = 0;

	return;
}

o_bool ftl_read(byte *sectorbuf, o_4b lsectornum)
{
	byte *pagebuf;
	SpareData *sdata;
	o_4b lblock_num, pblock_num, pagenum;
	o_bool found, r;
	int i;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	lblock_num = lsectornum / INPLACE_SECTORS_PER_BLOCK;
	pblock_num = ramtable.L2P[lblock_num]; 

	found = FALSE;
	for(i = PAGES_PER_BLOCK-1; i >= INPLACE_PAGES_PER_BLOCK; i--)
	{
		pagenum = pblock_num * PAGES_PER_BLOCK + i;
		read(pagebuf, pagenum, MODE_FILE);
		memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE); 

		if(sdata->lsn == lsectornum)
		{
			memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
			r = TRUE;	
			found = TRUE;
		}
	}

	if(found == FALSE)
	{
		read_pagebuf_with_inplace_sector(pagebuf, pblock_num, lsectornum);
		memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE); 

		if(sdata->lsn == lsectornum)
		{
			memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
			r = TRUE;	
		}
		else
		{
			r = FALSE;	
		}
	}

	free(pagebuf);
	free(sdata);

	return r;
}

void ftl_write(byte *sectorbuf, o_4b lsectornum)
{
	byte *pagebuf;
	SpareData *sdata;
	int pblock_num, lblock_num, pagenum;
	int i, next_outofplace_offset;
	BLOCK_INSIDE r;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	lblock_num = lsectornum / INPLACE_SECTORS_PER_BLOCK;
	offset = lsectornum % INPLACE_SECTORS_PER_BLOCK;
	pblock_num = ramtable.L2P[lblock_num];

	if(pblock_num < 0)	/* a logical block is first allocated a physical block */
	{
		pblock_num = allocate_f_block();

		ramtable.L2P[lblock_num] = pblock_num;
		ramtable.block_state[pblock_num] = (byte)STATE_U;

		sdata->lbn = lblock_num;
		sdata->lsn = lsectornum;
		sdata->block_state = (byte)STATE_U;

		memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
		memcpy(pagebuf+SECTOR_SIZE, sdata, SPARE_SIZE);

		pagenum = get_physicalpage_num(pblock_num, lsectornum);
		write(pagebuf, pagenum, MODE_FILE);

		if(pagenum != pblock_num * PAGES_PER_BLOCK)
		{
			update_block_info(pblock_num, lblock_num, STATE_U);
		}

#ifdef PRINT_FOR_DEBUG
		printf("++++++ for the first time, writing data to the sector %d +++++++\n", lsectornum);
		print_ramtable_info();
		print_block_info(ramtable.L2P[lblock_num]);
#endif
	}
	else
	{
//		assert(ramtable.block_state[pblock_num] == (byte)STATE_U);
		if(ramtable.block_state[pblock_num] != (byte)STATE_U)
		{
			print_ramtable_info();
			print_block_info(pblock_num);

			assert(0);
		}

		r = is_block_available(lsectornum, &next_outofplace_offset);

		if(r == BLOCK_AVAIL || r == BLOCK_FULL)
		{
			if(r == BLOCK_FULL)
			{
				pblock_num = merge(lsectornum);
			}

			pagenum = get_physicalpage_num(pblock_num, lsectornum);
		}
		else if(r == LSN_CONFLICT)
		{
			pagenum = pblock_num * PAGES_PER_BLOCK + next_outofplace_offset;
		}

		sdata->lbn = lblock_num;
		sdata->lsn = lsectornum;
		sdata->block_state = (byte)STATE_U;
		
		memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
		memcpy(pagebuf+SECTOR_SIZE, sdata, SPARE_SIZE);

		write(pagebuf, pagenum, MODE_FILE);

#ifdef PRINT_FOR_DEBUG
		printf("++++++ in case of state U, writing data to the sector %d +++++++\n", lsectornum);
		print_ramtable_info();
		print_block_info(ramtable.L2P[lblock_num]);
#endif
	}

	free(pagebuf);
	free(sdata);

	return;
}

BLOCK_INSIDE is_block_available(o_4b lsector_num, int *next_outofplace_offset)
{
	byte *pagebuf;
	SpareData *sdata;
	o_4b pblock_num, lblock_num, pagenum;
	BLOCK_INSIDE r;
	int i;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	lblock_num = lsector_num / INPLACE_SECTORS_PER_BLOCK;
	pblock_num = ramtable.L2P[lblock_num];

	pagenum = get_physicalpage_num(pblock_num, lsector_num);

	read(pagebuf, pagenum, MODE_FILE);
	memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);

	if(sdata->lsn < 0)
	{
		r = BLOCK_AVAIL;
	}
	else
	{
		r = BLOCK_FULL;
		for(i = INPLACE_PAGES_PER_BLOCK; i < PAGES_PER_BLOCK; i++)
		{
			pagenum = pblock_num * PAGES_PER_BLOCK + i;
			read(pagebuf, pagenum, MODE_FILE);
			memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);

			if(sdata->lsn < 0)
			{
				r = LSN_CONFLICT;
				*next_outofplace_offset = i;
				break;
			}
		}
	}

	free(sdata);
	free(pagebuf);

	return r;
}

o_4b read_pagebuf_with_inplace_sector(byte *buf, o_4b pblocknum, o_4b lsectornum)
{
	int pagenum;

	pagenum = get_physicalpage_num(pblocknum, lsectornum);

	read(buf, pagenum, MODE_FILE);

	return  pagenum;
}

int get_physicalpage_num(o_4b pblocknum, o_4b lsectornum)
{
	int pagenum, offset;

	offset = lsectornum % INPLACE_SECTORS_PER_BLOCK;
	pagenum = (pblocknum * PAGES_PER_BLOCK) + offset;

	return pagenum;
}

void update_block_info(o_4b pblock_num, o_4b lblock_num, byte block_state)
{
	byte *pagebuf;
	SpareData *sdata;
	o_4b lsn;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	read(pagebuf, pblock_num*PAGES_PER_BLOCK, MODE_FILE);
	memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);

	if(sdata->lbn < 0)
	{
		lsn = sdata->lsn;

		sdata->lbn = lblock_num;
		sdata->lsn = lsn;
		sdata->block_state = block_state;
		memcpy(pagebuf+SECTOR_SIZE, sdata, SPARE_SIZE);

		write(pagebuf, pblock_num*PAGES_PER_BLOCK, MODE_FILE);
	}

	free(pagebuf);
	free(sdata);

	return;
}

o_4b merge(o_4b lsector_num)	
{
	o_4b lblock_num, pblock_num, new_pblock_num;

	lblock_num = lsector_num / INPLACE_SECTORS_PER_BLOCK;
	pblock_num = ramtable.L2P[lblock_num];

	assert(ramtable.block_state[ramtable.spare_pblock_num] == (byte)STATE_F);

	smart_copy(pblock_num, ramtable.spare_pblock_num, lsector_num); 
	swap_copy(pblock_num, ramtable.spare_pblock_num, lsector_num);

	ramtable.block_state[ramtable.spare_pblock_num] = STATE_U;
	ramtable.L2P[lblock_num] = ramtable.spare_pblock_num;

	new_pblock_num = ramtable.spare_pblock_num;

	erase(pblock_num);
	assert(ramtable.block_state[pblock_num] == (byte)STATE_U);
	ramtable.block_state[pblock_num] = (byte)STATE_F;
	ramtable.spare_pblock_num = pblock_num;

	return new_pblock_num;
}

void swap_copy(o_4b src_block_num, o_4b dest_block_num, o_4b lsector_num)
{
	byte *sectorbuf, *src_pagebuf, *dest_pagebuf;
	SpareData *src_sdata, *dest_sdata;
	o_4b src_pagenum, dest_pagenum;
	int i;

	src_pagebuf = (byte *)malloc(PAGE_SIZE);
	dest_pagebuf = (byte *)malloc(PAGE_SIZE);
	src_sdata = (SpareData *)malloc(SPARE_SIZE);
	dest_sdata = (SpareData *)malloc(SPARE_SIZE);
	sectorbuf = (byte *)malloc(SECTOR_SIZE);

#ifdef PRINT_FOR_DEBUG
	printf("### swap_copy ###\n");
	printf("src: %d dest: %d\n", src_block_num, dest_block_num);
	print_block_info(src_block_num);
	print_block_info(dest_block_num);
#endif

	for(i = 0; i < INPLACE_PAGES_PER_BLOCK; i++)
	{
		src_pagenum = src_block_num * PAGES_PER_BLOCK + i;
		read(src_pagebuf, src_pagenum, MODE_FILE); 
		memcpy(src_sdata, src_pagebuf+SECTOR_SIZE, SPARE_SIZE);

		dest_pagenum = dest_block_num * PAGES_PER_BLOCK + i;
		read(dest_pagebuf, dest_pagenum, MODE_FILE); 
		memcpy(dest_sdata, dest_pagebuf+SECTOR_SIZE, SPARE_SIZE);

		if(src_sdata->lsn >= 0 && src_sdata->lsn != lsector_num)
		{
			if(dest_sdata->lsn < 0)
			{
#ifdef PRINT_FOR_DEBUG
			printf("### srclsn: %d -> destlsn: %d\n", src_sdata->lsn, dest_sdata->lsn);
#endif
				memcpy(sectorbuf, src_pagebuf, SECTOR_SIZE);
				memcpy(dest_pagebuf, sectorbuf, SECTOR_SIZE); 
				memcpy(dest_pagebuf+SECTOR_SIZE, src_sdata, SPARE_SIZE);

				write(dest_pagebuf, dest_pagenum, MODE_FILE);
			}
		}
		else
		{
			if(i == 0 && src_sdata->lsn != lsector_num)
			{
				memcpy(dest_pagebuf+SECTOR_SIZE, src_sdata, SPARE_SIZE);
				write(dest_pagebuf, dest_pagenum, MODE_FILE);
			}
		}
	}

	free(src_pagebuf);
	free(dest_pagebuf);
	free(src_sdata);
	free(dest_sdata);
	free(sectorbuf);

	return;
}

void smart_copy(o_4b src_block_num, o_4b dest_block_num, o_4b lsector_num)
{
	byte *sectorbuf, *src_pagebuf, *dest_pagebuf;
	SpareData *src_sdata, *dest_sdata;
	o_4b src_pagenum, dest_pagenum, lblock_num;
	int i, j, offset;

	src_pagebuf = (byte *)malloc(PAGE_SIZE);
	dest_pagebuf = (byte *)malloc(PAGE_SIZE);
	src_sdata = (SpareData *)malloc(SPARE_SIZE);
	dest_sdata = (SpareData *)malloc(SPARE_SIZE);
	sectorbuf = (byte *)malloc(SECTOR_SIZE);

	lblock_num = lsector_num / INPLACE_SECTORS_PER_BLOCK;

#ifdef PRINT_FOR_DEBUG
	printf("### smart copy ###\n");
	printf("src: %d dest: %d\n", src_block_num, dest_block_num);
	print_block_info(src_block_num);
	print_block_info(dest_block_num);
#endif

	for(i = PAGES_PER_BLOCK-1; i >= INPLACE_PAGES_PER_BLOCK; i--)
	{
		src_pagenum = src_block_num * PAGES_PER_BLOCK + i;
		read(src_pagebuf, src_pagenum, MODE_FILE); 
		memcpy(src_sdata, src_pagebuf+SECTOR_SIZE, SPARE_SIZE);

		if(src_sdata->lsn >= 0 && src_sdata->lsn != lsector_num)
		{
			dest_pagenum = read_pagebuf_with_inplace_sector(dest_pagebuf, dest_block_num, src_sdata->lsn);
			memcpy(dest_sdata, dest_pagebuf+SECTOR_SIZE, SPARE_SIZE);

			if(dest_sdata->lsn < 0)
			{
#ifdef PRINT_FOR_DEBUG
				printf("### srclsn: %d -> destlsn: %d\n", src_sdata->lsn, dest_sdata->lsn);
#endif
				memcpy(sectorbuf, src_pagebuf, SECTOR_SIZE);
				memcpy(dest_pagebuf, sectorbuf, SECTOR_SIZE); 
				memcpy(dest_pagebuf+SECTOR_SIZE, src_sdata, SPARE_SIZE);

				write(dest_pagebuf, dest_pagenum, MODE_FILE);
			}
		}
	}

	free(src_pagebuf);
	free(dest_pagebuf);
	free(src_sdata);
	free(dest_sdata);
	free(sectorbuf);

	return;
}

o_4b allocate_f_block()
{
	o_4b first_f_block_idx;
	int i;

	first_f_block_idx = -1;

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		if(i == ramtable.spare_pblock_num)
		{
			continue;
		}

		if(ramtable.block_state[i] == (byte)STATE_F)
		{
			first_f_block_idx = i;
			break;
		}
	}

	assert(first_f_block_idx >= 0);

	return first_f_block_idx;
}

/*
void erase_block(o_4b pblock_num)
{
	byte *pagebuf;
	int i;

	pagebuf = (byte *)malloc(PAGE_SIZE);
	memset(pagebuf, 0xFF, PAGE_SIZE);

	for(i = 0; i < PAGES_PER_BLOCK; i++)
	{
		write(pagebuf, pblock_num*PAGES_PER_BLOCK+i, MODE_FILE);
	}

	free(pagebuf);

	return;
}
*/

o_bool populate_init_database()
{
	byte *blockbuf;
	int i;

	blockbuf = (byte *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE+1; i++)
	{
		fwrite(blockbuf, BLOCK_SIZE, 1, devicefp);	
	}

	free(blockbuf);

	return TRUE;
}

void print_ramtable_info()
{
	int i;

	for(i = 0; i < REALBLOCKS_PER_DEVICE; i++)
	{
		printf("\t[%4d -> %4d]\n", i, ramtable.L2P[i]);
	}

	printf("\tFree Block List: ");
	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		if(ramtable.block_state[i] == (byte)STATE_F)
		{
			printf("%d ", i);
		}

		assert(ramtable.block_state[i] == (byte)STATE_F || ramtable.block_state[i] == (byte)STATE_U);
		/*
		if(ramtable.block_state[i] == (byte)STATE_F)
		{
			printf("%d(%c) ", i, 'F');
		}
		else if(ramtable.block_state[i] == (byte)STATE_U)
		{
			printf("%d(%c) ", i, 'U');
		}
		*/
	}
	printf("\n");

	printf("\tspare block: %d\n", ramtable.spare_pblock_num);
//	print_block_info(ramtable.spare_pblock_num);
}

void print_block_info(o_4b pblocknum)
{
	byte pagebuf[PAGE_SIZE];
	SpareData sdata;
	int i, pagenum;

	printf("\tphysical block num: %d", pblocknum);
	if(ramtable.block_state[pblocknum] == (byte)STATE_F)
	{
		printf("(%c) -----\n", 'F');
	}
	else if(ramtable.block_state[pblocknum] == (byte)STATE_U)
	{
		printf("(%c) -----\n", 'U');
	}
	else
	{
		printf("(%c) -----\n", (char)ramtable.block_state[pblocknum]);
		assert(0);
	}

	printf("\n");

	for(i = 0; i < PAGES_PER_BLOCK; i++)
	{
		pagenum = pblocknum * PAGES_PER_BLOCK + i;
		read(pagebuf, pagenum, MODE_FILE);
		memcpy(&sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);
		if(i == 0)
		{
			printf("\t%5d-[%5d %5d ", pagenum, sdata.lbn, sdata.lsn);
			if(sdata.block_state == (byte)STATE_F)
			{
				printf(" %5c]\n", 'F');
			}
			else if(sdata.block_state == (byte)STATE_U)
			{
				printf(" %5c]\n", 'U');
			}
		}
		else
		{
			printf("\t%5d-[%5d %5d ", pagenum, -1, sdata.lsn);
			printf(" %5c]\n", ' ');
		}

		if(i == INPLACE_PAGES_PER_BLOCK-1)
		{
			printf("\t   ---------------------\n");
		}
	}
}

/*********************************************************************************************
int main()
{
	byte sectorbuf[SECTOR_SIZE];
	o_4b i, sectornum, lblocknum, MAX, inputnum;
	o_bool r;
	byte command[2];

	devicefp = fopen("database", "r+");

	if(devicefp == NULL)
	{
		devicefp = fopen("database", "w+");
		if(devicefp == NULL)
		{
			printf("file open error\n");
			exit(1);
		}
		populate_init_database();
	}

	ftl_open();

	printf("++ initialize database(y or n)? ");
	scanf("%s", command);
	if(command[0] == 'y')
	{
		MAX = 32;
		for(i = 0; i < MAX; i++)
		{
			ftl_write(sectorbuf, i);
		}
	}

	inputnum = 0;
	command[0] = 'w';
	srand(time(NULL));

	while(TRUE)
	{
		printf("++ input command(r/w/s) and sector number: ");
		scanf("%s %d", command, &sectornum);

		if(command[0] == 'r')
		{
			if(sectornum < SECTORS_PER_BLOCK*BLOCKS_PER_DEVICE)
			{
				r = ftl_read(sectorbuf, sectornum);
				if(r)
				{
					printf("given sector EXISTs!!!\n");
				}
				else
				{
					printf("given sector does NOT EXIST!!!\n");
				}

#ifdef PRINT_FOR_DEBUG
				lblocknum = sectornum / SECTORS_PER_BLOCK;
				print_block_info(ramtable.L2P[lblocknum]);
#endif
			}
			else
			{
				printf("given sector is over-ranged!!!\n");
			}
		}
		else if(command[0] == 'w')
		{
			if(inputnum < 20000)
				sectornum = rand() % (SECTORS_PER_BLOCK*BLOCKS_PER_DEVICE) / 2 ;
			else
				sectornum = rand() % (SECTORS_PER_BLOCK*BLOCKS_PER_DEVICE);
				
			if(sectornum < SECTORS_PER_BLOCK*BLOCKS_PER_DEVICE)
			{
				ftl_write(sectorbuf, sectornum);
			}
			else
			{
				printf("given sector is over-ranged!!!\n");
			}
			
			printf("input data number: %d\n", inputnum);

			if(inputnum++ > 50000)
			{
				break;
			}
		}
		else if(command[0] == 's')
		{
			break;
		}
	}

	fclose(devicefp);

	return 0;
}
*********************************************************************************************/
