
#include "max30102.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "algorithm.h"
#include "i2c.h"
#include "mxc_device.h"
#include "mxc_delay.h"

#define REG_INTR_STATUS_1 	                0x00
#define REG_INTR_STATUS_2 	                0x01
#define REG_INTR_ENABLE_1 	                0x02
#define REG_INTR_ENABLE_2 	                0x03
#define REG_FIFO_WR_PTR 		            0x04
#define REG_OVF_COUNTER 		            0x05
#define REG_FIFO_RD_PTR 		            0x06
#define REG_FIFO_DATA 			            0x07
#define REG_FIFO_CONFIG 		            0x08
#define REG_MODE_CONFIG 		            0x09
#define REG_SPO2_CONFIG 		            0x0A
#define REG_LED1_PA 				        0x0C
#define REG_LED2_PA 				        0x0D
#define REG_PILOT_PA 				        0x10
#define REG_MULTI_LED_CTRL1                 0x11
#define REG_MULTI_LED_CTRL2                 0x12
#define REG_TEMP_INTR 			            0x1F
#define REG_TEMP_FRAC 			            0x20
#define REG_TEMP_CONFIG 		            0x21
#define REG_PROX_INT_THRESH                 0x30
#define REG_REV_ID 					        0xFE
#define REG_PART_ID 				        0xFF

#define SAMPLES_PER_SECOND 					100	//检测频率

#define MAX30102_ADDRESS 				0x57


int max30102_write_reg(uint8_t reg, uint8_t data)
{
	mxc_i2c_req_t req;
	uint8_t tx_data[2];
	int ret;

	tx_data[0] = reg;
	tx_data[1] = data;

	req.i2c = MXC_I2C1;
	req.addr = MAX30102_ADDRESS;
	req.tx_buf = tx_data;
	req.tx_len = 2;
	req.rx_len = 0;
	ret = MXC_I2C_MasterTransaction(&req);
	MXC_Delay(MXC_DELAY_MSEC(1));
	if (ret != E_NO_ERROR) {
		printf("I2C Write Error: %d\n", ret);
		return ret;
	}
	return 0;
}

uint8_t max30102_read_reg(uint8_t reg)
{
	mxc_i2c_req_t req;
	uint8_t tx_data[1];
	uint8_t rx_data[1];
	int ret;

	tx_data[0] = reg;

	req.i2c = MXC_I2C1;
	req.addr = MAX30102_ADDRESS;
	req.tx_buf = tx_data;
	req.tx_len = 1;
	req.rx_buf = rx_data;
	req.rx_len = 1;
	ret = MXC_I2C_MasterTransaction(&req);
	MXC_Delay(MXC_DELAY_USEC(5));
	if (ret != E_NO_ERROR) {
		printf("I2C Read Error: %d\n", ret);
		return 0;
	}
	
	return rx_data[0];
}
int max30102_read_data(uint8_t reg, uint8_t *data, uint8_t len)
{
	mxc_i2c_req_t req;
	uint8_t tx_data[1];
	int ret;

	tx_data[0] = reg;

	req.i2c = MXC_I2C1;
	req.addr = MAX30102_ADDRESS;
	req.tx_buf = tx_data;
	req.tx_len = 1;
	req.rx_buf = data;
	req.rx_len = len;
	ret = MXC_I2C_MasterTransaction(&req);
	MXC_I2C_Stop(MXC_I2C1);
	MXC_Delay(MXC_DELAY_USEC(5));
	if (ret != E_NO_ERROR) {
		printf("I2C Read Error: %d\n", ret);
		return ret;
	}
	return 0;
}



//---------------------------------------------------------------------------------------
void MAX30102_reset(void)
{
	max30102_write_reg(REG_MODE_CONFIG,0x43); //INTR setting
}
void enable_temp(void)
{
	max30102_write_reg(REG_TEMP_CONFIG,0x01);//temperature measurment
}

curve_dp cb_func = NULL;
void MAX30102_Config(void)
{
	MAX30102_reset();
	
	enable_temp();
	max30102_write_reg(REG_INTR_ENABLE_1,0xc0); //INTR setting
	max30102_write_reg(REG_INTR_ENABLE_2,0x00);//
	max30102_write_reg(REG_FIFO_WR_PTR,0x00);//FIFO_WR_PTR[4:0]
	max30102_write_reg(REG_OVF_COUNTER,0x00);//OVF_COUNTER[4:0]
	max30102_write_reg(REG_FIFO_RD_PTR,0x00);//FIFO_RD_PTR[4:0]
	
	max30102_write_reg(REG_FIFO_CONFIG,0x0f);//sample avg = 1, fifo rollover=false, fifo almost full = 17
	max30102_write_reg(REG_MODE_CONFIG,0x03);//0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
	max30102_write_reg(REG_SPO2_CONFIG,0x27);	// SPO2_ADC range = 4096nA, SPO2 sample rate (50 Hz), LED pulseWidth (400uS)  
	max30102_write_reg(REG_LED1_PA,0x32);//Choose value for ~ 10mA for LED1
	max30102_write_reg(REG_LED2_PA,0x32);// Choose value for ~ 10mA for LED2
	max30102_write_reg(REG_PILOT_PA,0x7f);// Choose value for ~ 25mA for Pilot LED
	printf("REG_PART_ID: %x\r\n",max30102_read_reg(REG_PART_ID));
	printf("REG_REV_ID: %x\r\n",max30102_read_reg(REG_REV_ID));
}

uint32_t un_min=0xffffff ,un_max=0,un_prev_data=0;
void max30102_read_fifo(uint32_t *fifo_red,uint32_t *fifo_ir)
{
	uint8_t ach_i2c_data[6] = {0};

	//read and clear status register
	max30102_read_reg(REG_INTR_STATUS_1);
	max30102_read_reg(REG_INTR_STATUS_2);

	max30102_read_data(REG_FIFO_DATA,ach_i2c_data,6);

	*fifo_red = (long)((long)((long)ach_i2c_data[0]&0x03)<<16) | (long)ach_i2c_data[1]<<8 | (long)ach_i2c_data[2];  //将值合并得到实际数字，数组400-500为新读取数据
	*fifo_ir = 	(long)((long)((long)ach_i2c_data[3]&0x03)<<16) | (long)ach_i2c_data[4]<<8 | (long)ach_i2c_data[5];   //将值合并得到实际数字，数组400-500为新读取数据
	// *fifo_red&=0x03FFFF;  //Mask MSB [23:18]
	// *fifo_ir&=0x03FFFF;  //Mask MSB [23:18]
	//printf("RED[%d]\n",*fifo_red);
}

uint8_t max30102_data_rdy(void)
{
	uint8_t n_temp = 0;
	n_temp=max30102_read_reg(REG_INTR_STATUS_1);
	if(n_temp&0x40)
	{
		return 1;
	}
	return 0;
}
uint8_t max30102_temp(void)
{
	uint8_t temp;
	temp=max30102_read_reg(REG_TEMP_INTR);
	return temp;
}


#include "mxc_delay.h"
int32_t n_sp02; // SPO2 value
int8_t ch_spo2_valid;   // indicator to show if the SP02 calculation is valid
int32_t n_heart_rate;   // heart rate value
int8_t  ch_hr_valid;    // indicator to show if the heart rate calculation is valid

uint32_t aun_ir_buffer[500]; // IR LED sensor data
uint32_t aun_red_buffer[500]; // Red LED sensor data
int32_t n_brightness;
#define MAX_BRIGHTNESS 24

void Calculate_Hr_Spo2(uint32_t *ir_buffer, uint32_t *red_buffer) 
{
    // 使用MAX30102提供的算法计算心率和血氧饱和度
    maxim_heart_rate_and_oxygen_saturation(ir_buffer,500, red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
}


void blood_data_update(void)
{
    //标志位被使能时 读取FIFO  
	printf("blood_data_update start \n");
	uint32_t piosi = 0;
	uint32_t count = 500; //采样点数
	while(count--){
	    while(!(max30102_read_reg(REG_INTR_STATUS_1) & 0x40)){	        
		}		
		// printf("REG_FIFO_WR_PTR [%x] \n",max30102_read_reg(REG_FIFO_WR_PTR));
		// printf("REG_OVF_COUNTER [%x] \n",max30102_read_reg(REG_OVF_COUNTER));
		// printf("REG_FIFO_RD_PTR [%x] \n",max30102_read_reg(REG_FIFO_RD_PTR));
		max30102_read_fifo(&aun_red_buffer[piosi], &aun_ir_buffer[piosi]);  //read from MAX30102 FIFO2  
		un_min = min(un_min, aun_red_buffer[piosi]);
		un_max = max(un_max, aun_red_buffer[piosi]);
		//printf("piosi[%d]count[%d] REG_FIFO_WR_PTR [%x] \n",piosi,count,max30102_read_reg(REG_FIFO_WR_PTR));
		piosi++;	    
	}
	maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, 500, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
}

#define DAOJISHU 10
uint32_t bili[10];
uint8_t get_data_show(uint32_t *hr_rate,uint32_t *spo2_rate)
{
    // 读取和计算max30102数据，总体用缓存的500组数据分析，实际每读取100组新数据分析一次
    un_min = 0x3FFFF;
    un_max = 0;
	//uint32_t count = 500; //采样点数
	uint32_t i;
	float f_temp;
    // 将前100组样本转储到内存中（实际没有），并将后400组样本移到顶部，将100-500缓存数据移位到0-400
    for(i = 100; i < 500; i++){
        aun_red_buffer[i - 100] = aun_red_buffer[i]; // 将100-500缓存数据移位到0-400
        aun_ir_buffer[i - 100] = aun_ir_buffer[i]; // 将100-500缓存数据移位到0-400
        // 更新信号的最小值和最大值
    }
    // 在计算心率前取100组样本，取的数据放在400-500缓存数组中
	uint16_t daojishu = 0;
	uint16_t	pisi = 0;
	//printf("555555555555555\n");
    for(i = 400; i < 500; i++){
       // un_prev_data = aun_red_buffer[i - 1]; // 临时记录上一次读取数据
	    while(!(max30102_read_reg(REG_INTR_STATUS_1) & 0x40)){
	        //printf("max30102_read_reg_REG_INTR_STATUS_1\n");
	        MXC_Delay(MXC_DELAY_USEC(50));  // 增加延迟，确保总线稳定
		}
		max30102_read_fifo(&aun_red_buffer[i], &aun_ir_buffer[i]);  //read from MAX30102 FIFO2  
		MXC_Delay(MXC_DELAY_USEC(50));  // 增加延迟，确保总线稳定
		un_min = min(un_min, aun_red_buffer[i]);
		un_max = max(un_max, aun_red_buffer[i]);
		//printf("piosi[%d] REG_FIFO_WR_PTR [%x] \n",i,max30102_read_reg(REG_FIFO_WR_PTR));
    }
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, 500, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); // 传入500个心率和血氧数据计算传感器检测结论，反馈心率和血氧测试结果

    for(i = 400; i < 500; i++){
		if(!pisi){
			if(daojishu < 10){
	            f_temp = (float)(aun_red_buffer[i]-un_min)/(un_max - un_min);
				//printf("bili[%f] --[%d]\n",f_temp,un_max - un_min);
	            f_temp *= MAX_BRIGHTNESS; // 公式（心率曲线）=（新数值-旧数值）/（最大值-最小值）*255
	            bili[daojishu] = (uint32_t)f_temp + 40;
				//printf("bili[%d]=%d  f_temp=%f -%d\n",daojishu,bili[daojishu],f_temp,i);
				daojishu++;
			}
			pisi = 9;
		}else{
			pisi--;
		}
	}
	if(cb_func) {
		cb_func(bili,daojishu);
	}

    if((1 == ch_hr_valid) && (n_heart_rate < 200))
    {
        *hr_rate = n_heart_rate;
       // printf("HeartRate=%i\r\n", n_heart_rate);
    }
    if((1 == ch_spo2_valid) && (n_sp02 < 101))
    {
        *spo2_rate = n_sp02;
      //  printf("BloodOxyg=%i\r\n",  n_sp02);
    }	
	return 1;
}