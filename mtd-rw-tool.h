#ifndef EL_RW_TOOL_H
#define EL_RW_TOOL_H

#define PARKLINE_EXTRA						1

#define PARKLINE_DEFAULT					"408,161;613,161;508,261;236,424;785,424;50"
/*
#define PARKLINE_DEFAULT_A9_21MY            "357,206;735,207;551,306;229,388;884,388;55"
#define PARKLINE_DEFAULT_A9_22MY            "357,218;709,216;542,301;223,389;880,381;50"
#define PARKLINE_DEFAULTE_A9_CROSS_22MY     "357,208;723,210;544,294;228,382;871,378;50"
#define PARKLINE_DEFAULTE_A9_CROSS_23MY     PARKLINE_DEFAULTE_A9_CROSS_22MY
#define PARKLINE_DEFAULT_M_2WDL_23MY        PARKLINE_DEFAULT
#define PARKLINE_DEFAULT_M_4WD2WDH_23MY     PARKLINE_DEFAULT
*/

#define MAX_KEY_LEN							2048
#define MAX_PARKLINE_NUM					11
#ifdef PARKLINE_EXTRA
#define MAX_PARKLINE_NUM_EXT				1
#endif

// Production model
enum EL_MODEL
{
	EL_MODEL_DMXMG100 = 0,
	EL_MODEL_DMXMG101,
	EL_MODEL_DMXMG111,
	EL_MODEL_DMXMG102,
	EL_MODEL_DMXMG103,
	EL_MODEL_DMXMG104,
	EL_MODEL_DMXMG105,
	EL_MODEL_DMXMG106,
	EL_MODEL_DMXMG112,
	EL_MODEL_MAX
};


// Production model type
enum EL_MODEL_TYPE
{
	EL_MODEL_TYPE_A9 = 0,
	EL_MODEL_TYPE_M,
	EL_MODEL_TYPE_MAX
};

// Production model vehicle
enum EL_VEHICLE
{
    EL_VEHICLE_XPANDER_ICE = 0,     // Xpander ICE
    EL_VEHICLE_XPANDER_HEV,         // Xpander HEV
    EL_VEHICLE_XPANDER_CROSS_ICE,   // Xpander Cross ICE
    EL_VEHICLE_XPANDER_CROSS_HEV,   // Xpander Cross HEV
    EL_VEHICLE_TRITON1,             // Triton 2WD/4WD -> Double Cab/Club Cab -> High Rider
    EL_VEHICLE_TRITON2,             // Triton 2WD/4WD -> Double Cab/Club Cab -> Low Rider
    EL_VEHICLE_TRITON3,             // Triton 2WD/4WD -> Single Cab -> High Rider
    EL_VEHICLE_TRITON4,             // Triton 2WD/4WD -> Single Cab -> Low Rider
    EL_VEHICLE_TRITON5,             // Triton Reserve1
    EL_VEHICLE_TRITON6,             // Triton Reserve2
    EL_VEHICLE_TRITON7,             // Triton Reserve3
    EL_VEHICLE_XPANDER_26MY,		// Xpander 26MY
    EL_VEHICLE_MAX
};

// Production AREA
enum EL_AREA
{
    EL_AREA_INDONESIA = 0,
    EL_AREA_ASEAN,          // Asia (Except Indonesia), Asia (Except Thailand)
    EL_AREA_CSA,            // Central South America, Latin America
    EL_AREA_MEXICO,
    EL_AREA_MIDDLEEAST,     // Middle East, Egipt
    EL_AREA_THAILAND,
    EL_AREA_MAX
};


typedef struct parkLineInfo
{
	int points[5][2];
	int percentage;
} el_park_line_info_t;

typedef struct systemInfo
{
	// Signature header
	char strSignatureHeader[32];

	// Highest Priority: Settings in Production mode
	int iModel;
	int iModelType;
	char strPartsNumber[10];
	char strSerialNumber[10];

	// 2nd Priority: Service Diagnosis Mode (MMC diagnosis mode)
	int iModelVehicle;
	int iModelArea;

	int bCameraEnable;	// 2022/01/05, Change bCameraEnable definition, 0: n/a, 1: CCD, 2:AVM
	int bDongleEnable;
	int bDVREnable;
	int iAuxinEnable;	//2020/06/06, Add iAuxinEnable, 0: Off, 1: On

	char strParkLineList[MAX_PARKLINE_NUM][64];	// 2020/03/14，V1.0.3 change to 11 sets ParkLineList

	char mml_key[MAX_KEY_LEN];
	int  mml_key_len;

	char aa_rootCert[MAX_KEY_LEN];
	int  aa_rootCert_len;

	char aa_privateKey[MAX_KEY_LEN];
	int  aa_privateKey_len;

	char aa_clientCert[MAX_KEY_LEN];
	int  aa_clientCert_len;

	// CA verify time
	char aa_ca_time[32];
	int aa_ca_time_len;

#ifdef PARKLINE_EXTRA
	// MGDA2
	char strParkLineList_EXT_1[1][64];	// 2024/03/18, Version 1.0.7: Add a set of ParkLineList settings.
#endif
} el_system_info_t;

#endif
