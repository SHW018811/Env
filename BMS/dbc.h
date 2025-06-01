#ifndef DBC_H
#define DBC_H

#include "all_headers.h"

/*================================================================
dbc.h
dbc for BMS-CAN-charger communication
set dbc structures
set fixed CAN frame
=================================================================*/

// Structure for BMS_Company_Info (ID: 1568, 0x620)
// This message contains an 8-byte CompanyName field.
typedef struct __attribute__((packed)) {
    uint8_t CompanyName[8]; // Assumed to be an ASCII string (not null-terminated)
} BMS_Company_Info_t;


// Structure for VIN_car_info (ID: 1569, 0x621)
// This message contains an 8-byte Carname field.
typedef struct __attribute__((packed)) {
    uint8_t Carname[8]; // Assumed to be an ASCII string (not null-terminated)
} VIN_car_info_t;

// Structure for BMS_Status (ID: 1570, 0x622)
// Message layout: 1 byte Status, 2 bytes Time, 1 byte Flags, 1 byte DTC1, 1 byte DTC2.
typedef struct __attribute__((packed)) {
    uint8_t  Status;   // 8 bits: Status
    uint16_t Time;     // 16 bits: Time (in seconds)
    uint8_t  Flags;    // 8 bits: Flags
    uint8_t  DTC1;     // 8 bits: Diagnostic Trouble Code 1
    uint8_t  DTC2;     // 8 bits: Diagnostic Trouble Code 2
} BMS_Status_t;

// Structure for BMS_Battery_Info (ID: 1571, 0x623)
// Message layout: 2 bytes Voltage, 1 byte MinVoltage, 1 byte MinVoltageID,
// 1 byte MaxVoltage, 1 byte MaxVoltageID.
typedef struct __attribute__((packed)) {
    uint16_t Voltage;       // 16 bits: Battery Voltage (in V)
    uint8_t  MinVoltage;    // 8 bits: Minimum Voltage (in V, factor 0.1)
    uint8_t  MinVoltageID;  // 8 bits: Identifier for minimum voltage
    uint8_t  MaxVoltage;    // 8 bits: Maximum Voltage (in V, factor 0.1)
    uint8_t  MaxVoltageID;  // 8 bits: Identifier for maximum voltage
} BMS_Battery_Info_t;

// Structure for BMS_Charge_Current_Limits (ID: 1572, 0x624)
// Message layout: 2 bytes Current, 2 bytes ChargeLimit, 2 bytes DischargeLimit.
typedef struct __attribute__((packed)) {
    int16_t  Current;         // 16 bits: Charge/Discharge Current (in A, may be negative)
    uint16_t ChargeLimit;     // 16 bits: Maximum Charge Limit (in A)
    uint16_t DischargeLimit;  // 16 bits: Maximum Discharge Limit (in A)
} BMS_Charge_Current_Limits_t;

// Structure for BMS_SOC (ID: 1574, 0x626)
// Message layout: 1 byte SOC, 2 bytes DOD, 2 bytes Capacity, 1 byte SOH.
typedef struct __attribute__((packed)) {
    uint8_t  SOC;       // 8 bits: State of Charge (in %)
    uint16_t DOD;       // 16 bits: Depth of Discharge (in Ah)
    uint16_t Capacity;  // 16 bits: Battery Capacity (in Ah)
    uint8_t  SOH;       // 8 bits: State of Health (in %)
} BMS_SOC_t;

// Structure for BMS_Temperature (ID: 1575, 0x627)
// Message layout: 1 byte Temperature, 1 byte AirTemp, 1 byte MinTemp, 1 byte MinTempID,
// 1 byte MaxTemp, 1 byte MaxTempID.
typedef struct __attribute__((packed)) {
    int8_t  Temperature;  // 8 bits: Temperature (in °C)
    int8_t  AirTemp;      // 8 bits: Ambient/Air Temperature (in °C)
    int8_t  MinTemp;      // 8 bits: Minimum Temperature (in °C)
    uint8_t MinTempID;    // 8 bits: Identifier for minimum temperature
    int8_t  MaxTemp;      // 8 bits: Maximum Temperature (in °C)
    uint8_t MaxTempID;    // 8 bits: Identifier for maximum temperature
} BMS_Temperature_t;

// Structure for BMS_Resistance (ID: 1576, 0x628)
// Message layout: 2 bytes Resistance, 1 byte MinResistance, 1 byte MinResistanceID,
// 1 byte MaxResistance, 1 byte MaxResistanceID.
typedef struct __attribute__((packed)) {
    uint16_t Resistance;      // 16 bits: Resistance (in Ω)
    uint8_t  MinResistance;   // 8 bits: Minimum Resistance (in mΩ, factor 0.1)
    uint8_t  MinResistanceID; // 8 bits: Identifier for minimum resistance
    uint8_t  MaxResistance;   // 8 bits: Maximum Resistance (in mΩ, factor 0.1)
    uint8_t  MaxResistanceID; // 8 bits: Identifier for maximum resistance
} BMS_Resistance_t;

// Structure for BMS_DC_Charging (ID: 1577, 0x629)
// Message layout: 2 bytes DCLineVoltage, 2 bytes DCLineCurrent,
// 1 byte MaxChargeCurrent, 1 byte MaxDischargeCurrent, 2 bytes DCLinePower.
typedef struct __attribute__((packed)) {
    uint16_t DCLineVoltage;      // 16 bits: DC Line Voltage (in V)
    uint16_t DCLineCurrent;      // 16 bits: DC Line Current (in A)
    uint8_t  MaxChargeCurrent;   // 8 bits: Maximum Charge Current (in A)
    uint8_t  MaxDischargeCurrent;// 8 bits: Maximum Discharge Current (in A)
    uint16_t DCLinePower;        // 16 bits: DC Line Power (in W)
} BMS_DC_Charging_t;

typedef struct __attribute__((packed)) {
    double_t temp;
    double_t R0, R1, C1;
    uint16_t capacity;
    uint8_t charge_current;
    double_t voltage_terminal;
    double_t voltage_delay;
    double_t SOC;
} Battery_t;                                // battery as an object


// Temperature, R0, R1, C1, capacity, chareg_current, voltage_delay, voltage_terminal
typedef struct __attribute__((packed)) {
    double_t Temperature;
    double_t R0, R1, C1;                       // resistance
    double_t capacity;
    double_t charge_current;
    double_t voltage;
} Cell_Data_t;                              // Cell data via sensor


typedef struct {
    double SOC;
    double V1;
    double Voltage_terminal;
} Estimate_t;


typedef struct {
    int init;
    double F[2][2];
    double Q[2][2];
    double P[2][2];
    double Pp[2][2];
    double R;
} State_t;



/*================================================================
extern declarations
=================================================================*/
extern BMS_Company_Info_t bms_company_info;
extern VIN_car_info_t vin_car_info;
extern BMS_Status_t bms_status;
extern BMS_Battery_Info_t bms_battery_info;
extern BMS_Charge_Current_Limits_t bms_charge_current_limits;
extern BMS_SOC_t bms_soc;
extern BMS_Temperature_t bms_temperature;
extern BMS_Resistance_t bms_resistance;
extern BMS_DC_Charging_t bms_dc_charging;

extern Battery_t default_battery;
extern Battery_t battery[BATTERY_CELLS];
extern Cell_Data_t cell_data[BATTERY_CELLS];

extern Estimate_t estimate[BATTERY_CELLS];
extern State_t battery_state[BATTERY_CELLS];
#endif // DBC_H