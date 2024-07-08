/*
 * 	Version 1.0.0: first commit, no version string.
 *	Version 1.0.1: Change bCameraEnable definition, 0: n/a, 1: CCD, 2:AVM
 *  Version 1.0.2: Re-define Model, ModelType, ModelVehicle, ModelArea variabe range (0 ~ 15)
 *  Version 1.0.3: Re-define Model, ModelVehicle, ModelArea, ParkLineList number set to 11.
 *  Version 1.0.4: Add iAuxinEnable, 0: OFF, 1: ON (default: OFF)
 *  Version 1.0.5: And debug mode, default showes less log message
 *  Version 1.0.6: modify sys_info file location to /tmp directory.
 *  Version 1.0.7: Add a set of ParkLineList settings.
 *  Version 1.0.7a: Add a new configuration set for EL_VEHICLE.
 */

#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mtd-rw-tool.h"

#ifdef DEBUG
#define DBG(x...) printf(x)
#else
#define DBG(x...)
#endif

#define VERSION_STR			"1.0.7a"

#define RW_MTD_PARTION		"/dev/mtd25"
#define SYS_INFO_PATH		"/media/flash/nvm/sys_info"

#define TRUE					1
#define FALSE					0

#define MTD_USED_CNT			10	// 0x800 * 10 = 20480 bytes

#define PARTS_NUMBER_LENGTH		8
#define SERIAL_NUMBER_LENGTH	8

#define PARKLINE_STR_FORMAT	"%d,%d;%d,%d;%d,%d;%d,%d;%d,%d;%d"

#define MIN_LIMIT_VAL			0
#define MAX_LIMIT_VAL			16

el_system_info_t sys_info;
const char signature_header[] = "I_LOVE_ELEAD_ELEAD_LOVE_ME";

/*
 *  Return 0 for success, 1 for failed.
 */
int mtd_erase()
{
	mtd_info_t mtd_info;		   // the MTD structure
	erase_info_t ei;			   // the erase block structure
	int i;
	int cnt = 0;

	unsigned char read_buf[20] = {0x00};				// empty array for reading

	int fd = open(RW_MTD_PARTION, O_RDWR); // open the mtd device for reading and
	// writing. Note you want mtd0 not mtdblock0
	// also you probably need to open permissions
	// to the dev (sudo chmod 777 /dev/mtd0)

	ioctl(fd, MEMGETINFO, &mtd_info);	// get the device info

	// dump it for a sanity check, should match what's in /proc/mtd
	DBG("MTD Type: %x\nMTD total size: %x(hex) bytes\nMTD erase size: %x(hex) bytes\nMTD write size: %x(hex) bytes\n",
		   mtd_info.type, mtd_info.size, mtd_info.erasesize, mtd_info.writesize);

	ei.length = mtd_info.erasesize;   //set the erase block size
	for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
	{
		ioctl(fd, MEMUNLOCK, &ei);
		// printf("Eraseing Block %#x\n", ei.start); // show the blocks erasing
		// warning, this prints a lot!
		ioctl(fd, MEMERASE, &ei);
	}

	lseek(fd, 0, SEEK_SET); 			  // go to the first block
	read(fd, read_buf, sizeof(read_buf)); // read 20 bytes
	close(fd);

	// sanity check, should be all 0xFF if erase worked
	for(i = 0; i<20; i++)
	{
		//printf("buf[%d] = 0x%02x ", i, (unsigned int)read_buf[i]);
		if (read_buf[i] != 0xFF)
		{
			printf("Sanity check, buf[%d] = 0x%02x, not equal to 0xFF.\n", i, read_buf[i]);
			cnt++;
		}
	}
	DBG("\n");

	if (cnt != 0)
	{
		printf("MTD erase failed");
		return 1;
	}

	return 0;
}


void get_sys_info_from_mtd(el_system_info_t *info)
{
	mtd_info_t mtd_info;		   // the MTD structure
	erase_info_t ei;			   // the erase block structure
	int i;

	int fd = open(RW_MTD_PARTION, O_RDWR); // open the mtd device for reading and
	// writing. Note you want mtd0 not mtdblock0
	// also you probably need to open permissions
	// to the dev (sudo chmod 777 /dev/mtd0)

	ioctl(fd, MEMGETINFO, &mtd_info);	// get the device info
	/**********************************************************
	 *	 important part!									  *
	 *	 notice the size of data array is mtd_info.writesize  *
	 **********************************************************/

	uint32_t write_size = mtd_info.writesize * MTD_USED_CNT;
	unsigned char data[write_size];
	bzero(data, write_size);

	lseek(fd, 0, SEEK_SET); 			 // go back to first block's start
	read(fd, data, sizeof(*info));

	memset(info, 0, sizeof(*info));
	memcpy(info, data, sizeof(*info));

	close(fd);
}


void save_sys_info(el_system_info_t *info)
{
	mtd_erase();					// erase entire MTD first

	mtd_info_t mtd_info;		   // the MTD structure
	erase_info_t ei;			   // the erase block structure
	int i;

	unsigned char read_buf[20] = {0x00};				// empty array for reading
	el_system_info_t *pSysInfo;

	int fd = open(RW_MTD_PARTION, O_RDWR); // open the mtd device for reading and
	// writing. Note you want mtd0 not mtdblock0
	// also you probably need to open permissions
	// to the dev (sudo chmod 777 /dev/mtd0)

	ioctl(fd, MEMGETINFO, &mtd_info);	// get the device info
	/**********************************************************
	 *	 important part!									  *
	 *	 notice the size of data array is mtd_info.writesize  *
	 **********************************************************/
	uint32_t write_size = mtd_info.writesize * MTD_USED_CNT;
	DBG("write_size = %d, sys_info_size = %d\n", write_size, sizeof(*info));

	unsigned char data[write_size];
	bzero(data, write_size);
	memcpy(&data, (char *)info, sizeof(*info));

	lseek(fd, 0, SEEK_SET); 	   // go back to first block's start
	write(fd, data, sizeof(data)); // write our message

	lseek(fd, 0, SEEK_SET); 			 // go back to first block's start
	//read(fd, read_buf, sizeof(read_buf));// read the data

	bzero(data, write_size);
	read(fd, data, sizeof(*info));

	//memset(&sys_info, 0, sizeof(sys_info));
	memcpy(info, data, sizeof(*info));

#ifdef DEBUG
	dump_sys_info(info);
#endif

	close(fd);
}

void dump_sys_info(el_system_info_t *info)
{
	int i;

	printf("Signature \t= %s\n", info->strSignatureHeader);
	printf("Model \t\t= %d\n", info->iModel);
	printf("ModelType \t= %d\n", info->iModelType);
	printf("PartsNumber \t= %s\n", info->strPartsNumber);
	printf("SerialNumber \t= %s\n", info->strSerialNumber);
	printf("ModelVehicle \t= %d\n", info->iModelVehicle);
	printf("ModelArea \t= %d\n", info->iModelArea);
	printf("CameraEnable \t= %d\n", info->bCameraEnable);
	printf("DongleEnable \t= %d\n", info->bDongleEnable);
	printf("DVREnable \t= %d\n", info->bDVREnable);
	printf("AuxinEnable \t= %d\n", info->iAuxinEnable);

	for (i = 0; i < MAX_PARKLINE_NUM; i++)
		printf("ParkLineList_%d \t= %s\n", i, info->strParkLineList[i]);

#ifdef PARKLINE_EXTRA
		printf("ParkLineList_%d \t= %s\n", MAX_PARKLINE_NUM, info->strParkLineList_EXT_1[0]);
#endif

	// MML Key
	printf("------------------------------------------------------------\n");
	printf("MML Key Length  = %d\n", info->mml_key_len);
	printf("MML KEY	\t= ");
	if (info->mml_key_len > 0)
	{
		printf("\n");
		for (i = 0; i < info->mml_key_len; i++)
			printf("%c", info->mml_key[i]);
	}
	else
	{
		printf("N/A\n");
	}

	// AA RootCA
	printf("------------------------------------------------------------\n");
	printf("AA RootCA Length = %d\n", info->aa_rootCert_len);
	printf("AA RootCA = ");
	if (info->aa_rootCert_len > 0)
	{
		printf("\n");
		for (i = 0; i < info->aa_rootCert_len; i++)
			printf("%c", info->aa_rootCert[i]);
	}
	else
	{
		printf("N/A\n");
	}

	// AA ClientCA
	printf("------------------------------------------------------------\n");
	printf("AA ClientCA Length = %d\n", info->aa_clientCert_len);
	printf("AA ClientCA = ");
	if (info->aa_clientCert_len > 0)
	{
		printf("\n");
		for (i = 0; i < info->aa_clientCert_len; i++)
			printf("%c", info->aa_clientCert[i]);
	}
	else
	{
		printf("N/A\n");
	}

	// AA PrivateKey
	printf("\nAA PrivateKey Length = %d\n", info->aa_privateKey_len);
	printf("AA PrivateKey = ");
	if (info->aa_privateKey_len > 0)
	{
		printf("\n");
		for (i = 0; i < info->aa_privateKey_len; i++)
			printf("%c", info->aa_privateKey[i]);
	}
	else
	{
		printf("N/A\n");
	}

	// AA CA verification time
	printf("------------------------------------------------------------\n");
	printf("AA CA Verify Time Length = %d\n", info->aa_ca_time_len);
	printf("AA CA Verification Time  = ");
	if (info->aa_ca_time_len > 0)
	{
		for (i = 0; i < info->aa_ca_time_len; i++)
			printf("%c", info->aa_ca_time[i]);
		printf("\n");
	}
	else
	{
		printf("N/A\n");
	}
}

void set_default_for_sys_info()
{
	int i;

	sys_info.iModel = EL_MODEL_DMXMG100;
	sys_info.iModelType = EL_MODEL_TYPE_A9;
	strcpy(sys_info.strPartsNumber, "12345678");
	strcpy(sys_info.strSerialNumber, "87654321");

	sys_info.iModelVehicle = EL_VEHICLE_XPANDER_ICE;
	sys_info.iModelArea = EL_AREA_INDONESIA;

	sys_info.bCameraEnable = FALSE;
	sys_info.bDongleEnable = FALSE;
	sys_info.bDVREnable = FALSE;
	sys_info.iAuxinEnable = FALSE;

	for (i = 0; i < MAX_PARKLINE_NUM; i++)
	{
		strncpy(sys_info.strParkLineList[i], PARKLINE_DEFAULT, sizeof(PARKLINE_DEFAULT));
	}
#ifdef PARKLINE_EXTRA
	strncpy(sys_info.strParkLineList_EXT_1[0], PARKLINE_DEFAULT, sizeof(PARKLINE_DEFAULT));
#endif

/*
	strncpy(sys_info.strParkLineList[0], PARKLINE_DEFAULT_A9_21MY, sizeof(PARKLINE_DEFAULT_A9_21MY));
	strncpy(sys_info.strParkLineList[1], PARKLINE_DEFAULT_A9_22MY, sizeof(PARKLINE_DEFAULT_A9_22MY));
	strncpy(sys_info.strParkLineList[2], PARKLINE_DEFAULTE_A9_CROSS_22MY, sizeof(PARKLINE_DEFAULTE_A9_CROSS_22MY));
	strncpy(sys_info.strParkLineList[3], PARKLINE_DEFAULTE_A9_CROSS_23MY, sizeof(PARKLINE_DEFAULTE_A9_CROSS_23MY));
	strncpy(sys_info.strParkLineList[4], PARKLINE_DEFAULT_M_2WDL_23MY, sizeof(PARKLINE_DEFAULT_M_2WDL_23MY));
	strncpy(sys_info.strParkLineList[5], PARKLINE_DEFAULT_M_4WD2WDH_23MY, sizeof(PARKLINE_DEFAULT_M_4WD2WDH_23MY));
*/

	sys_info.mml_key_len = 0;
	memset(sys_info.mml_key, 0, sizeof(sys_info.mml_key));

	sys_info.aa_rootCert_len = 0;
	memset(sys_info.aa_rootCert, 0, sizeof(sys_info.aa_rootCert));

	sys_info.aa_clientCert_len = 0;
	memset(sys_info.aa_clientCert, 0, sizeof(sys_info.aa_clientCert));

	sys_info.aa_privateKey_len = 0;
	memset(sys_info.aa_privateKey, 0, sizeof(sys_info.aa_privateKey));

	sys_info.aa_ca_time_len = 0;
	memset(sys_info.aa_ca_time, 0, sizeof(sys_info.aa_ca_time));
}


int dump_sys_info_to_file(el_system_info_t *info)
{
	int i;
	char str[128] = {0};

	FILE *outfile;

	// open file for writing
	outfile = fopen (SYS_INFO_PATH, "w");
	if (outfile == NULL)
	{
		fprintf(stderr, "\nError opend file\n");
		exit (1);
	}

	snprintf(str, sizeof(str), "Model=%d\n", info->iModel);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "ModelType=%d\n", info->iModelType);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "PartsNumber=%s\n", info->strPartsNumber);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "SerialNumber=%s\n", info->strSerialNumber);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "ModelVehicle=%d\n", info->iModelVehicle);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "ModelArea=%d\n", info->iModelArea);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "CameraEnable=%d\n", info->bCameraEnable);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "DongleEnable=%d\n", info->bDongleEnable);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "DVREnable=%d\n", info->bDVREnable);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	snprintf(str, sizeof(str), "AuxinEnable=%d\n", info->iAuxinEnable);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));

	for (i = 0; i < MAX_PARKLINE_NUM; i++)
	{
		snprintf(str, sizeof(str), "ParkLineList_%d=%s\n", i, info->strParkLineList[i]);
		fwrite(str, 1, strlen(str), outfile);
		memset(&str, 0, sizeof(str));
	}
#ifdef PARKLINE_EXTRA
	snprintf(str, sizeof(str), "ParkLineList_%d=%s\n", MAX_PARKLINE_NUM, info->strParkLineList_EXT_1[0]);
	fwrite(str, 1, strlen(str), outfile);
	memset(&str, 0, sizeof(str));
#endif

	/*
	 if(fwrite != 0)
	     printf("contents to file written successfully !\n");
	 else
	     printf("error writing file !\n");
	*/

	// close file
	fclose (outfile);

	return 0;
}


int sys_info_set_by_name(int argc, char **argv)
{
	int i;
	char *set_key, *set_val;

	/*
	printf("--------------------------------\n");
	for(i = 0; i < argc; i++) {
	    printf("%s \n", argv[i]);
	}
	printf("--------------------------------\n");
	*/

	if (argc != 4)
	{
		printf("\n\nUsage:\n");
		printf("	%s set key value \n\n", argv[0]);
		return -1;
	}
	else
	{
		set_key = argv[2];
		set_val = argv[3];
		DBG("Key = %s, Val = %s\n", set_key, set_val);
	}

	get_sys_info_from_mtd(&sys_info);

	if (strncmp(set_key, "Model", strlen(set_key)) == 0)
	{
		// int iModel;
		i = atoi(set_val);
		if (i < EL_MODEL_DMXMG100 || i >= EL_MODEL_MAX)
		//if (i < MIN_LIMIT_VAL || i > MAX_LIMIT_VAL)
		{
			printf("[Err] Model value ... [%s]\n", set_val);
			//printf("[Err] Model value must be %d ~ %d ... [%s]\n", MIN_LIMIT_VAL, MAX_LIMIT_VAL, set_val);
			return -1;
		}

		sys_info.iModel = atoi(set_val);
	}
	else if (strncmp(set_key, "ModelType", strlen(set_key)) == 0)
	{
		// int iModelType;
		i = atoi(set_val);
		if (i < EL_MODEL_TYPE_A9 || i >= EL_MODEL_TYPE_MAX)
		//if (i < MIN_LIMIT_VAL || i > MAX_LIMIT_VAL)
		{
			printf("[Err] ModelType value ... [%s]\n", set_val);
			//printf("[Err] ModelType value must be %d ~ %d ... [%s]\n", MIN_LIMIT_VAL, MAX_LIMIT_VAL, set_val);
			return -1;
		}

		sys_info.iModelType = atoi(set_val);
	}
	else if (strncmp(set_key, "PartsNumber", strlen(set_key)) == 0)
	{
		// char strPartsNumber[10];
		// 8 digits
		if (strlen(set_val) != PARTS_NUMBER_LENGTH)
		{
			printf("[Err] PartsNumber length is not equal to %d ... [%s]\n", PARTS_NUMBER_LENGTH, set_val);
			return -1;
		}

		strncpy(sys_info.strPartsNumber, set_val, strlen(set_val));
	}
	else if (strncmp(set_key, "SerialNumber", strlen(set_key)) == 0)
	{
		// char strSerialNumber[10];
		// 8 digits
		if (strlen(set_val) != SERIAL_NUMBER_LENGTH)
		{
			printf("[Err] SerialNumber length is not equal to %d ... [%s]\n", SERIAL_NUMBER_LENGTH, set_val);
			return -1;
		}
		strncpy(sys_info.strSerialNumber, set_val, strlen(set_key));
	}
	else if (strncmp(set_key, "ModelVehicle", strlen("ModelVehicle")) == 0)
	{
		// int iModelVehicle;
		i = atoi(set_val);
		if (i < EL_VEHICLE_XPANDER_ICE || i >= EL_VEHICLE_MAX)
		//if (i < MIN_LIMIT_VAL || i > MAX_LIMIT_VAL)
		{
			printf("[Err] ModelVehicle value ... [%s]\n", set_val);
			//printf("[Err] ModelVehicle value must be %d ~ %d ... [%s]\n", MIN_LIMIT_VAL, MAX_LIMIT_VAL, set_val);
			return -1;
		}

		sys_info.iModelVehicle = atoi(set_val);
	}
	else if (strncmp(set_key, "ModelArea", strlen(set_key)) == 0)
	{
		// int iModelArea;
		i = atoi(set_val);
		if (i < EL_AREA_INDONESIA || i >= EL_AREA_MAX)
		//if (i < MIN_LIMIT_VAL || i > MAX_LIMIT_VAL)
		{
			printf("[Err] ModelArea value ... [%s]\n", set_val);
			//printf("[Err] ModelArea value must be %d ~ %d ... [%s]\n", MIN_LIMIT_VAL, MAX_LIMIT_VAL, set_val);
			return -1;
		}

		sys_info.iModelArea = atoi(set_val);
	}
	else if (strncmp(set_key, "CameraEnable", strlen(set_key)) == 0)
	{
		// int bCameraEnable;
		i = atoi(set_val);
		if (i < 0 || i > 2)
		{
			printf("[Err] CameraEnable value must be 0 ~ 2 ... [%s]\n", set_val);
			return -1;
		}

		sys_info.bCameraEnable = atoi(set_val);
	}
	else if (strncmp(set_key, "DongleEnable", strlen(set_key)) == 0)
	{
		// int bDongleEnable;
		i = atoi(set_val);
		if (i != TRUE && i != FALSE)
		{
			printf("[Err] DongleEnable value must be 0 or 1 ... [%s]\n", set_val);
			return -1;
		}

		sys_info.bDongleEnable = atoi(set_val);
	}
	else if (strncmp(set_key, "DVREnable", strlen(set_key)) == 0)
	{
		// int bDVREnable;
		i = atoi(set_val);
		if (i != TRUE && i != FALSE)
		{
			printf("[Err] DVREnable value must be 0 or 1 ... [%s]\n", set_val);
			return -1;
		}

		sys_info.bDVREnable = atoi(set_val);
	}
	else if (strncmp(set_key, "AuxinEnable", strlen(set_key)) == 0)
	{
		// int iAuxinEnable;
		i = atoi(set_val);
		if (i != TRUE && i != FALSE)
		{
			printf("[Err] AuxinEnable value must be 0 or 1 ... [%s]\n", set_val);
			return -1;
		}

		sys_info.iAuxinEnable = atoi(set_val);
	}
	//else if (strncmp(set_key, "ParkLineList_", strlen(set_key) - 1) == 0)
	else if (strncmp(set_key, "ParkLineList_", strlen("ParkLineList_")) == 0)
	{
		int idx = -1;
		int cnt_1 = 0, cnt_2 = 0;

		DBG("idx=%c\n", set_key[strlen("ParkLineList_")]);

		idx = atoi(&set_key[strlen("ParkLineList_")]);
#ifdef PARKLINE_EXTRA
		if (idx < 0 || idx > MAX_PARKLINE_NUM + MAX_PARKLINE_NUM_EXT)
		{
			printf("[Err] ParkLineList idx value between 0 ~ %d ... [%d]\n", MAX_PARKLINE_NUM + MAX_PARKLINE_NUM_EXT, idx);
			return -1;
		}
#else
		if (idx < 0 || idx >= MAX_PARKLINE_NUM)
		{
			printf("[Err] ParkLineList idx value between 0 ~ %d ... [%d]\n", MAX_PARKLINE_NUM, idx);
			return -1;
		}
#endif

		int ret;
		el_park_line_info_t park_line_info;

		ret = sscanf(set_val, PARKLINE_STR_FORMAT,
					 &park_line_info.points[0][0], &park_line_info.points[0][1],
					 &park_line_info.points[1][0], &park_line_info.points[1][1],
					 &park_line_info.points[2][0], &park_line_info.points[2][1],
					 &park_line_info.points[3][0], &park_line_info.points[3][1],
					 &park_line_info.points[4][0], &park_line_info.points[4][1],
					 &park_line_info.percentage);

		if (ret == 11)
		{
			DBG("PARKLINE Format Check OK!!\n");
			for (i = 0; i < 5; i++)
				DBG("\tParkLine Point %d: [%d][%d]\n", i + 1, park_line_info.points[i][0], park_line_info.points[i][1]);
			DBG("\tPercentage: %d\n", park_line_info.percentage);
		}
		else
		{
			printf("[Err] PARKLINE format mismatch .. [%s]\n", set_val);
			printf("\t example: 357,208;723,210 ;544,294;228,382;871,378;50\n");
			return -1;
		}

#ifdef PARKLINE_EXTRA
	if (idx < MAX_PARKLINE_NUM)
	{
#endif
		memset(sys_info.strParkLineList[idx], 0, sizeof(sys_info.strParkLineList[idx]));

		snprintf(sys_info.strParkLineList[idx], sizeof(sys_info.strParkLineList[idx]), PARKLINE_STR_FORMAT,
				 park_line_info.points[0][0], park_line_info.points[0][1],
				 park_line_info.points[1][0], park_line_info.points[1][1],
				 park_line_info.points[2][0], park_line_info.points[2][1],
				 park_line_info.points[3][0], park_line_info.points[3][1],
				 park_line_info.points[4][0], park_line_info.points[4][1],
				 park_line_info.percentage);

		DBG("set_val = %s, len = %d\n", sys_info.strParkLineList[idx], strlen(sys_info.strParkLineList[idx]));

		strncpy(sys_info.strParkLineList[idx], set_val, strlen(set_val));
#ifdef PARKLINE_EXTRA
	}
	else
	{
		if (idx == MAX_PARKLINE_NUM)
		{
			memset(sys_info.strParkLineList_EXT_1[0], 0, sizeof(sys_info.strParkLineList_EXT_1[0]));

			snprintf(sys_info.strParkLineList_EXT_1[0], sizeof(sys_info.strParkLineList_EXT_1[0]), PARKLINE_STR_FORMAT,
				 park_line_info.points[0][0], park_line_info.points[0][1],
				 park_line_info.points[1][0], park_line_info.points[1][1],
				 park_line_info.points[2][0], park_line_info.points[2][1],
				 park_line_info.points[3][0], park_line_info.points[3][1],
				 park_line_info.points[4][0], park_line_info.points[4][1],
				 park_line_info.percentage);

			DBG("set_val = %s, len = %d\n", sys_info.strParkLineList_EXT_1[0], strlen(sys_info.strParkLineList_EXT_1[0]));

			strncpy(sys_info.strParkLineList_EXT_1[0], set_val, strlen(set_val));
		}
		else
			printf("[Err] Wrong ParkLineList idx !!!\n");

	}
#endif
		/*
				// "357,208;723,210 ;544,294;228,382;871,378;50"
				for (i = 0; i < strlen(set_val); i++)
				{
					if (set_val[i] == ';')
						cnt_1++;
					if (set_val[i] == ',')
						cnt_2++;
				}

				if (cnt_1 != 5 && cnt_2 != 5)
				{
					printf("[Err] ParkLineList format mismatch .. [%s]\n", set_val);
					printf("\t example: 357,208;723,210 ;544,294;228,382;871,378;50\n");
					printf("\t Check Criteria: 5 comon and 5 semicolon \n");
					return -1;
				}

				memset(sys_info.strParkLineList[idx], 0, sizeof(sys_info.strParkLineList[idx]));
				strncpy(sys_info.strParkLineList[idx], set_val, strlen(set_val));

				printf("set_val = %s, len = %d\n", set_val, strlen(set_val));
		*/
	}
	else
	{
		printf("unknown error occurred !!!\n");
		return -1;
	}

	save_sys_info(&sys_info);

	return 0;
}


int read_key_func(char *key_type, char *rw_filename)
{
	int i, key_len;
	char *key_array;
	FILE *fp;

	// read sys_info first.
	get_sys_info_from_mtd(&sys_info);

	// read data from file
	fp = fopen(rw_filename, "r"); // read mode
	if (fp == NULL)
	{
		perror("Error while opening the file.\n");
		return -1;
	}

	// get file size
	fseek(fp, 0L, SEEK_END);
	key_len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	DBG("file size = %d\n", key_len);

	if (!strncmp(key_type, "key_mml", strlen("key_mml")))
	{
		sys_info.mml_key_len = key_len;
		key_array = sys_info.mml_key;
		memset(key_array, 0, MAX_KEY_LEN);
	}
	else if (!strncmp(key_type, "key_aa_rca", strlen("key_aa_rca")))
	{
		sys_info.aa_rootCert_len = key_len;
		key_array = sys_info.aa_rootCert;
		memset(key_array, 0, MAX_KEY_LEN);
	}
	else if (!strncmp(key_type, "key_aa_cca", strlen("key_aa_cca")))
	{
		sys_info.aa_clientCert_len = key_len;
		key_array = sys_info.aa_clientCert;
		memset(key_array, 0, MAX_KEY_LEN);
	}
	else if (!strncmp(key_type, "key_aa_private", strlen("key_aa_private")))
	{
		sys_info.aa_privateKey_len = key_len;
		key_array = sys_info.aa_privateKey;
		memset(key_array, 0, MAX_KEY_LEN);
	}
	else if (!strncmp(key_type, "key_aa_time", strlen("key_aa_time")))
	{
		sys_info.aa_ca_time_len = key_len;
		key_array = sys_info.aa_ca_time;
		memset(key_array, 0, 32);
	}
	else
	{
		printf("Wrong key_type: %s \n", key_type);
		printf("Support key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private\n");

		return -1;
	}

	// Save MML Cert to NAND
	for (i = 0; i < key_len; i++)
	{
		key_array[i] = fgetc(fp);
		DBG("%c", key_array[i]);
	}
	fclose(fp);

	save_sys_info(&sys_info);
	return 0;
}

int write_key_func(char *key_type, char *rw_filename)
{
	int i, key_len;
	char *key_array;
	FILE *fp;

	// read sys_info first.
	get_sys_info_from_mtd(&sys_info);

	if (!strncmp(key_type, "key_mml", strlen("key_mml")))
	{
		key_len = sys_info.mml_key_len;
		key_array = sys_info.mml_key;
	}
	else if (!strncmp(key_type, "key_aa_rca", strlen("key_aa_rca")))
	{
		key_len = sys_info.aa_rootCert_len;
		key_array = sys_info.aa_rootCert;
	}
	else if (!strncmp(key_type, "key_aa_cca", strlen("key_aa_cca")))
	{
		key_len = sys_info.aa_clientCert_len;
		key_array = sys_info.aa_clientCert;
	}
	else if (!strncmp(key_type, "key_aa_private", strlen("key_aa_private")))
	{
		key_len = sys_info.aa_privateKey_len;
		key_array = sys_info.aa_privateKey;
	}
	else if (!strncmp(key_type, "key_aa_time", strlen("key_aa_time")))
	{
		key_len = sys_info.aa_ca_time_len;
		key_array = sys_info.aa_ca_time;
	}
	else
	{
		printf("Wrong key_type: %s \n", key_type);
		printf("Support key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private\n");

		return -1;
	}

	// check mml_key_len
	if (key_len <= 0)
	{
		printf("No %s founded!!\n", key_type);
		return -1;
	}

	fp = fopen(rw_filename, "w");
	if (fp == NULL)
	{
		printf("Error opening the file %s", rw_filename);
		return -1;
	}

	// write to the text file
	for (i = 0; i < key_len; i++)
	{
		fprintf(fp, "%c", key_array[i]);
		//printf("%c", sys_info.mml_key[i]);
	}

	// close the file
	fclose(fp);
	return 0;
}

int dump_key_func(char *key_type)
{
	int i;
	int key_len;
	char *key_array;

	// read sys_info first.
	get_sys_info_from_mtd(&sys_info);

	// key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private
	if (!strncmp(key_type, "key_mml", strlen("key_mml")))
	{
		key_len = sys_info.mml_key_len;
		key_array = sys_info.mml_key;
	}
	else if (!strncmp(key_type, "key_aa_rca", strlen("key_aa_rca")))
	{
		key_len = sys_info.aa_rootCert_len;
		key_array = sys_info.aa_rootCert;
	}
	else if (!strncmp(key_type, "key_aa_cca", strlen("key_aa_cca")))
	{
		key_len = sys_info.aa_clientCert_len;
		key_array = sys_info.aa_clientCert;
	}
	else if (!strncmp(key_type, "key_aa_private", strlen("key_aa_private")))
	{
		key_len = sys_info.aa_privateKey_len;
		key_array = sys_info.aa_privateKey;
	}
	else if (!strncmp(key_type, "key_aa_time", strlen("key_aa_time")))
	{
		key_len = sys_info.aa_ca_time_len;
		key_array = sys_info.aa_ca_time;
	}
	else
	{
		printf("Wrong key_type: %s \n", key_type);
		printf("Support key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private\n");

		return -1;
	}

	if (key_len <= 0)
	{
		printf("No %s founded!!\n", key_type);
		return -1;
	}

	for (i = 0; i < key_len; i++)
		DBG("%c", key_array[i]);

	DBG("\n");
	return 0;
}


int main(int argc, char *argv[])
{
	char *sub_cmd;
	int i;

	// Add Version String
	//printf("----- Version: %s -----\n", VERSION_STR);
	if(argc < 2)
	{
		printf("\nUsage:\n");
		printf("	%s sub_cmd ... \n\n", argv[0]);
		printf("       sub_cmd: default -> load default sys_info\n");
		printf("       sub_cmd: show    -> show sys_info\n");
		printf("       sub_cmd: dump    -> dump sys_info to a file\n");
		printf("       sub_cmd: erase   -> erase entire MTD partition\n");
		printf("       sub_cmd: set     -> set sys_info configurations\n");
		printf("                set 	KEY VALUE	-> set VALUE by KEY\n");
		printf("       sub_cmd: [key_mml | key_aa_rca | key_aa_cca | key_aa_private]		-> Key or CA for MML and AA\n");
		printf("                 		read  filename		-> read Key or CA from file\n");
		printf("                 		write filename		-> write Key or CA from file\n");
		printf("                	 	dump				-> dump Key or CA to console\n");
		return -1;
	}
	else
		sub_cmd = argv[1];

	/*
	printf("--------------------------------\n");
	for(i=0; i<argc; i++) {
	    printf("%s \n", argv[i]);
	}
	printf("--------------------------------\n");
	*/

	// Initialize global data.
	memset(&sys_info, 0, sizeof(sys_info));
	strncpy(sys_info.strSignatureHeader, signature_header, sizeof(sys_info.strSignatureHeader));

	if (strcmp(sub_cmd, "default") == 0)
	{
		set_default_for_sys_info();
		//dump_sys_info(&sys_info);
		save_sys_info(&sys_info);
	}
	else if (strcmp(sub_cmd, "show") == 0)
	{
		get_sys_info_from_mtd(&sys_info);
		dump_sys_info(&sys_info);
	}
	else if (strcmp(sub_cmd, "set") == 0)
	{
		return sys_info_set_by_name(argc, argv);
	}
	else if (strcmp(sub_cmd, "erase") == 0)
	{
		mtd_erase();
	}
	else if (strcmp(sub_cmd, "dump") == 0)
	{
		printf("----- Version: %s -----\n", VERSION_STR);

		get_sys_info_from_mtd(&sys_info);
#ifdef DEBUG
		dump_sys_info(&sys_info);
#endif
		dump_sys_info_to_file(&sys_info);
	}
#if 0
	else if (strcmp(sub_cmd, "read_mml") == 0)
	{
		if (argc != 3)
		{
			printf("Argument Usage: read_mml filename\n");
			return -1;
		}

		// read sys_info first.
		get_sys_info_from_mtd(&sys_info);

		char *mml_lic_path;
		mml_lic_path = argv[2];

		// check MML License exist?
		if (access(mml_lic_path, F_OK) == -1)
		{
			printf("MML license not found. (%s)\n", mml_lic_path);
			return -1;
		}

		// read data from file
		FILE *fp;
		int sz = 0;

		fp = fopen(mml_lic_path, "r"); // read mode
		if (fp == NULL)
		{
			perror("Error while opening the file.\n");
			return -1;
		}

		// get file size
		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		printf("file size = %d\n", sz);

		// Save MML Cert to NAND
		memset(sys_info.mml_key, 0, sizeof(sys_info.mml_key));

		sys_info.mml_key_len = sz;

		for (i = 0; i < sz; i++)
		{
			sys_info.mml_key[i] = fgetc(fp);
			printf("%c", sys_info.mml_key[i]);
		}
		fclose(fp);

		save_sys_info(&sys_info);

	}
	else if (strcmp(sub_cmd, "write_mml") == 0)
	{
		FILE *fp;
		char *mml_lic_path;

		if (argc != 3)
		{
			printf("Argument Usage: write_mml filename\n");
			return -1;
		}

		// read sys_info first.
		get_sys_info_from_mtd(&sys_info);

		// check mml_key_len
		if (sys_info.mml_key_len <= 0)
		{
			printf("No MML License founded!!\n");
			return -1;
		}

		mml_lic_path = argv[2];

		fp = fopen(mml_lic_path, "w");
		if (fp == NULL)
		{
			printf("Error opening the file %s", mml_lic_path);
			return -1;
		}

		// write to the text file
		for (i = 0; i < sys_info.mml_key_len; i++)
		{
			fprintf(fp, "%c", sys_info.mml_key[i]);
			//printf("%c", sys_info.mml_key[i]);
		}

		// close the file
		fclose(fp);
	}
	else if (strcmp(sub_cmd, "dump_mml") == 0)
	{
		// read sys_info first.
		get_sys_info_from_mtd(&sys_info);

		// check mml_key_len
		if (sys_info.mml_key_len <= 0)
		{
			printf("No MML License founded!!\n");
			return -1;
		}

		for (i = 0; i < sys_info.mml_key_len; i++)
			printf("%c", sys_info.mml_key[i]);
	}
#endif
	else if (strncmp(sub_cmd, "key_", 4) == 0)
	{
		// key_mml, key_aa_rca, key_aa_cca, key_aa_private

		int i;
		char *key_type, *rw_mode, *rw_filename;

		key_type = argv[1];
		rw_mode = argv[2];

		DBG("--------------------------------\n");
		for(i = 0; i < argc; i++)
		{
			DBG("%s \n", argv[i]);
		}
		DBG("--------------------------------\n");

		// key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private
		if (strncmp(key_type, "key_mml", strlen("key_mml")) &&
				strncmp(key_type, "key_aa_rca", strlen("key_aa_rca")) &&
				strncmp(key_type, "key_aa_cca", strlen("key_aa_cca")) &&
				strncmp(key_type, "key_aa_private", strlen("key_aa_private")) &&
				strncmp(key_type, "key_aa_time", strlen("key_aa_time")))
		{
			printf("Wrong key_type: %s \n", key_type);
			printf("Support key_type: key_mml, key_aa_rca, key_aa_cca, key_aa_private, key_aa_time\n");

			return -1;
		}

		// rw_mode: read, write, dump
		if (!strncmp(rw_mode, "read", strlen("read")))
		{
			if (argc == 4)
			{
				rw_filename = argv[3];
				if (access(rw_filename, F_OK) == -1)
				{
					printf("%s: read file not found. (%s)\n", key_type, rw_filename);
					return -1;
				}
			}
			else
			{
				printf("Argument Usage: %s read filename\n", key_type);
				return -1;
			}

			read_key_func(key_type, rw_filename);
		}
		else if (!strncmp(rw_mode, "write", strlen("write")))
		{
			if (argc == 4)
			{
				rw_filename = argv[3];
			}
			else
			{
				printf("Argument Usage: %s write filename\n", key_type);
				return -1;
			}

			write_key_func(key_type, rw_filename);
		}
		else if (!strncmp(rw_mode, "dump", strlen("dump")))
		{
			dump_key_func(key_type);
		}
		else
		{
			printf("Wrong rw_mode: %s \n", rw_mode);
			printf("Support rw_mode: read, write, dump\n");
			printf("\t read, write need filename parameter.\n");

			return -1;
		}
	}
	else
	{
		printf("Unknown sub_cmd: %s\n\n", sub_cmd);
		return 1;
	}

	return 0;
}
