#include <stdint.h>


typedef void (*curve_dp)(int32_t *bili,uint16_t cnt); 

void MAX30102_Config(void);
uint8_t max30102_data_rdy(void);
uint8_t max30102_temp(void);
void blood_data_update(void);
uint8_t get_data_show(uint32_t *hr_rate,uint32_t *spo2_rate);
void enable_temp(void);