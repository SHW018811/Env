#ifndef OCV_SOC_T_H
#define OCV_SOC_T_H

#define OCV_SOC_T_SIZE 986

extern double charge_ocv[986];
extern double charge_soc[986];
double OcvFromSoc(double soc);
double SocFromOcv(double ocv);

#endif