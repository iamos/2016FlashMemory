#include <stdio.h>
#include <string.h>
#include "msbs.h"

FILE	*devicefp;					//	flash device file
char	CACHE_REG[PAGE_SIZE];		//	Page Register

int read_page_num;
int write_page_num;
int erase_block_num;
	
int read(char *data, int ppn, int mode)
{
	int	ret;

	read_page_num ++;

	if(mode == MODE_FILE) {
		fseek(devicefp, PAGE_SIZE*ppn, SEEK_SET);
		ret = fread((void *)data, PAGE_SIZE, 1, devicefp);
		if(ret == PAGE_SIZE) {
			return 0;
		}
		else {
			if(feof(devicefp)) {
				memset((void*)data, (int)'\0', PAGE_SIZE);
				return 0;
			}
			else
				return -1;
		}
	} else {
		return 0;
	}
}

int write(char *data, int ppn, int mode)
{
	int ret;

	write_page_num ++;

	if(mode == MODE_FILE) {
		fseek(devicefp, PAGE_SIZE*ppn, SEEK_SET);
		ret = fwrite((void *)data, PAGE_SIZE, 1, devicefp);
		if(ret == PAGE_SIZE) {			
			return 0;
		}
		else
			return -1;
	} else {
		memcpy((void *)&CACHE_REG, (void *)data, PAGE_SIZE);	// ??
		return 0;
	}
}

int erase(int pbn)
{
	char data[BLOCK_SIZE];
	int	ret;

	erase_block_num ++;

//	memset((void*)data, (int)'\0', BLOCK_SIZE);
	memset((void*)data, (char)0xFF, BLOCK_SIZE);
	
	fseek(devicefp, BLOCK_SIZE*pbn, SEEK_SET);
	
	ret = fwrite((void *)data, BLOCK_SIZE, 1, devicefp);
	
	if(ret == BLOCK_SIZE) 			
		return 0;
	else
		return -1;

}

int device_init()
{

#ifdef K9XXG08
	printf("initialization ... \n");

	devicefp = fopen("K9XXG08", "w+");

	printf("Flash Device Information ....\n");
	printf("page size : %d bytes\n", PAGE_SIZE);
	printf("block size : %d bytes\n", BLOCK_SIZE);
	printf("device size : %d bytes\n", DEVICE_SIZE);

#endif
}

/*
main()
{
	char data[PAGE_SIZE];
	char data2[PAGE_SIZE];
	int i;

	device_init();

	memset((void*)data, (int)'*', PAGE_SIZE);

	for(i=60; i < 63; ++i)
		write(data, i, MODE_FILE);

	read(data2, 0, MODE_FILE);

	for(i=0; i < PAGE_SIZE; ++i)
		printf("%c", data2[i]);
	printf("\n");

	read(data2, 60, MODE_FILE);

	for(i=0; i < PAGE_SIZE; ++i)
		printf("%c", data2[i]);
	printf("\n");

	erase(0);

	read(data2, 5, MODE_FILE);

	for(i=0; i < PAGE_SIZE; ++i)
		printf("%c", data2[i]);

	printf("\n");
}
*/
