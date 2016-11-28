#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "msbs.h"

#define COST_PER_READ	1

extern FILE *devicefp;
extern int read_page_num;
extern int write_page_num;
extern int erase_block_num;

extern int a, b, c, d, e, f;

int main(int argc, char *argv[])
{
	FILE *workloadfp;
	byte sectorbuf[SECTOR_SIZE];
	o_4b sectornum;
	int i, r, times;
	float total_elapsed_time;


	if (argc != 3)
	{
		printf("usage: msbs data_file times\n");
		exit(1);
	}

	devicefp = fopen("database", "r+b");

	if (devicefp == NULL)
	{
		devicefp = fopen("database", "w+b");
		if (devicefp == NULL)
		{
			printf("database file open error\n");
			exit(1);
		}
		populate_init_database();
	}

	workloadfp = fopen(argv[1], "r");
	if (workloadfp == NULL)
	{
		printf("pattern file %s open error\n", argv[1]);
		exit(1);
	}

	ftl_open();

	times = atoi(argv[2]);

	for (i = 0; i < times; i++)
	{

		r = fscanf(workloadfp, "%d", &sectornum);

		while (r != EOF)
		{
			if (sectornum >= 0 && sectornum < PAGES_PER_BLOCK * REALBLOCKS_PER_DEVICE)
			{
//				printf("++++++++++++++++++++++++++++ input sector: %d\n", sectornum);
				ftl_write(sectorbuf, sectornum);

			}
			else
			{
				printf("given sector %d is over-ranged!!!\n", sectornum);
			}

			r = fscanf(workloadfp, "%d", &sectornum);
		}

		fseek(workloadfp, 0L, SEEK_SET);

		total_elapsed_time = (read_page_num + 20 * write_page_num + 200 * erase_block_num) * COST_PER_READ;
		printf("%7d %7d %7d %7d %7.2f\n", i + 1, read_page_num, write_page_num, erase_block_num,
		       total_elapsed_time);

	}


	printf("******* Total number of reads: %d\n", read_page_num);
	printf("******* Total number of writes: %d\n", write_page_num);
	printf("******* Total number of erases: %d\n", erase_block_num);
	// printf("******* %d %d %d %d %d %d\n", a, b, c, d, e, f);

	fclose(devicefp);
	fclose(workloadfp);

	return 0;
}
