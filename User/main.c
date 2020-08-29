#include "stm32f0xx.h"
#include "uart.h"
#include "lis2dh12.h"
#include "delay.h"
#include "spi.h"
#include "lis2dh12_reg.h"
#include "throttle_control.h"


int main(void)
{	
  	uint8_t STATUS_RES;
	uint16_t count_sum_acceleration = 0;
	int32_t sum_acceleration_x = 0;
	int32_t sum_acceleration_y = 0;
	int32_t sum_acceleration_z = 0;
	int16_t	average_acceleration_x[11] = {0};
	int16_t	average_acceleration_y[11] = {0};
	int16_t	average_acceleration_z[11] = {0};
	int16_t average_to_sum_x = 0;
	int16_t average_to_sum_y = 0;
	int16_t average_to_sum_z = 0;
	uint8_t i;
	uint8_t count_speed_up = 0;
	uint8_t count_speed_cut = 0;
	uint8_t speed_up_self_lock = 0;
	uint8_t speed_cut_self_lock = 0;
	uint8_t Count_invalid_value = 0;
	uint32_t Total_number_of_cycles_cut = 0;
	uint8_t speed_cut_number_of_cycles = 0;
	uint32_t Total_number_of_cycles_up = 0;
	uint8_t speed_up_number_of_cycles = 0;
	int16_t LIS2DH12_OUT[3] = {0};
	
  	Delay_Init(48); // 延时函数初始化
  	usart_Init_config(115200); // 初始化串口
	SPI1_Init(); // SPI初始化
	delay_ms(5);
	lis2dh12_Init(); // lis2dh12初始化
	throttle_control_Init(); //油门控制初始化

	
	while(1)
	{	
		lis2dh12_xl_data_ready_get(null, &STATUS_RES);
		if(STATUS_RES)
		{
		  	Read_acceleration_Data(LIS2DH12_OUT); 			
			sum_acceleration_x += LIS2DH12_OUT[0]; //将读取的x加速度值求和
			sum_acceleration_y += LIS2DH12_OUT[1]; //将读取的y加速度值求和
			sum_acceleration_z += LIS2DH12_OUT[2]; //将读取的z加速度值求和
			count_sum_acceleration++;
		}
		
		
		if(count_sum_acceleration == 400)
		{
		  	Total_number_of_cycles_cut++;
			Total_number_of_cycles_up++;
			
		  	for(i=0; i<10; i++)
			{
			  	average_acceleration_x[i] = average_acceleration_x[i+1]; //向左滑动一位，丢弃最低位
			}
		  	for(i=0; i<10; i++)
			{
			  	average_acceleration_y[i] = average_acceleration_y[i+1]; //向左滑动一位，丢弃最低位
			}
		  	for(i=0; i<10; i++)
			{
			  	average_acceleration_z[i] = average_acceleration_z[i+1]; //向左滑动一位，丢弃最低位
			}
			
		  	average_acceleration_x[10] = sum_acceleration_x/400; //400点求平均值,再将值放入最高位
			average_acceleration_y[10] = sum_acceleration_y/400; //400点求平均值,再将值放入最高位
		  	average_acceleration_z[10] = sum_acceleration_z/400; //400点求平均值,再将值放入最高位
			printf("average_acceleration_x[10] = %d\n",average_acceleration_x[10]);
			printf("average_acceleration_y[10] = %d\n",average_acceleration_y[10]);
			printf("average_acceleration_z[10] = %d\n",average_acceleration_z[10]);
			
			count_sum_acceleration = 0;	
			sum_acceleration_x = 0;
			sum_acceleration_y = 0;
			sum_acceleration_z = 0;
			
			for(i=0; i<10; i++)
			{
				if(average_acceleration_x[10] > average_acceleration_x[i])			
					average_to_sum_x += (average_acceleration_x[10] - average_acceleration_x[i]); //将第11个值分别减去前十个值再求和
				else
					average_to_sum_x += (average_acceleration_x[i] - average_acceleration_x[10]);	
			}
			for(i=0; i<10; i++)
			{
				if(average_acceleration_y[10] > average_acceleration_y[i])			
					average_to_sum_y += (average_acceleration_y[10] - average_acceleration_y[i]); //将第11个值分别减去前十个值再求和
				else
					average_to_sum_y += (average_acceleration_y[i] - average_acceleration_y[10]);	
			}
			for(i=0; i<10; i++)
			{
				if(average_acceleration_z[10] > average_acceleration_z[i])			
					average_to_sum_z += (average_acceleration_z[10] - average_acceleration_z[i]); //将第11个值分别减去前十个值再求和
				else
					average_to_sum_z += (average_acceleration_z[i] - average_acceleration_z[10]);	
			}
			printf("average_to_sum_x = %d\n",average_to_sum_x);	
			printf("average_to_sum_y = %d\n",average_to_sum_y);	
			printf("average_to_sum_z = %d\n",average_to_sum_z);	
			
			/* 计数达到加速阀值得个数 */
			if(average_to_sum_x > 160)
			{
				count_speed_up++;
				speed_up_number_of_cycles++;
				/* 判断是否连续3次进入if(average_to_sum_x > 150) */
				if(speed_up_number_of_cycles != Total_number_of_cycles_up)
				{
				  	count_speed_up = 1;
					Total_number_of_cycles_up = 0;
					speed_up_number_of_cycles = 0;
				}
			}
			
			/* 计数达到减速速阀值得个数 */
			if((average_to_sum_x < 150) && (average_to_sum_y < 150) && (average_to_sum_z < 500) && (speed_up_self_lock == 1))
			{				
				count_speed_cut++;
				speed_cut_number_of_cycles++;
				/* 判断是否连续3次进入if((average_to_sum_x < 200) && (speed_up_self_lock == 1)) */
				if(speed_cut_number_of_cycles != Total_number_of_cycles_cut)
				{
				  	count_speed_cut = 1;
					Total_number_of_cycles_cut = 0;
					speed_cut_number_of_cycles = 0;
				}
				count_speed_up = 0;
			}
			average_to_sum_x = 0;
			average_to_sum_y = 0;
			average_to_sum_z = 0;
			
			/* 判断是否加速 */
			if((count_speed_up == 3) && (speed_up_self_lock != 1))	
			{
				printf("The car speed up\n");
				speed_up();//加油门
				speed_up_self_lock = 1;//加速自锁，防止重复加速
				speed_cut_self_lock = 0;//减速解锁
				count_speed_up = 0;
			}
			/* 判断是否减速 */
			if((count_speed_cut == 4) && (speed_cut_self_lock != 1))
			{
				printf("The car speed cut\n");
				speed_cut();//减油门
				
				/* 过滤减油门瞬间的共振 */
				while((Count_invalid_value < 12) && (speed_up_self_lock == 1))//前者判断条件为无效值得个数是否达到，后者条件判断是为了防止刚上电就进入循环
				{
					lis2dh12_xl_data_ready_get(null, &STATUS_RES);
					if(STATUS_RES)
					{
						Read_acceleration_Data(LIS2DH12_OUT); 			
						sum_acceleration_x += LIS2DH12_OUT[0]; //将读取的x加速度值求和
						sum_acceleration_y += LIS2DH12_OUT[1]; //将读取的y加速度值求和
						sum_acceleration_z += LIS2DH12_OUT[2]; //将读取的z加速度值求和
						count_sum_acceleration++;
					}
										
					if(count_sum_acceleration == 400)
					{						
						for(i=0; i<10; i++)
						{
							average_acceleration_x[i] = average_acceleration_x[i+1]; //向左滑动一位，丢弃最低位
						}
						for(i=0; i<10; i++)
						{
							average_acceleration_y[i] = average_acceleration_y[i+1]; //向左滑动一位，丢弃最低位
						}
						for(i=0; i<10; i++)
						{
							average_acceleration_z[i] = average_acceleration_z[i+1]; //向左滑动一位，丢弃最低位
						}
						
						average_acceleration_x[10] = sum_acceleration_x/400; //400点求平均值,再将值放入最高位
						average_acceleration_y[10] = sum_acceleration_y/400; //400点求平均值,再将值放入最高位
						average_acceleration_z[10] = sum_acceleration_z/400; //400点求平均值,再将值放入最高位
						
						count_sum_acceleration = 0;	
						sum_acceleration_x = 0;
						sum_acceleration_y = 0;
						sum_acceleration_z = 0;
						
						for(i=0; i<10; i++)
						{
							if(average_acceleration_x[10] > average_acceleration_x[i])			
								average_to_sum_x += (average_acceleration_x[10] - average_acceleration_x[i]); //将第11个值分别减去前十个值再求和
							else
								average_to_sum_x += (average_acceleration_x[i] - average_acceleration_x[10]);	
						}
						for(i=0; i<10; i++)
						{
							if(average_acceleration_y[10] > average_acceleration_y[i])			
								average_to_sum_y += (average_acceleration_y[10] - average_acceleration_y[i]); //将第11个值分别减去前十个值再求和
							else
								average_to_sum_y += (average_acceleration_y[i] - average_acceleration_y[10]);	
						}
						for(i=0; i<10; i++)
						{
							if(average_acceleration_z[10] > average_acceleration_z[i])			
								average_to_sum_z += (average_acceleration_z[10] - average_acceleration_z[i]); //将第11个值分别减去前十个值再求和
							else
								average_to_sum_z += (average_acceleration_z[i] - average_acceleration_z[10]);	
						}
						printf("average_to_sum_x = %d\n",average_to_sum_x);	
						printf("average_to_sum_y = %d\n",average_to_sum_y);	
						printf("average_to_sum_z = %d\n",average_to_sum_z);		
						Count_invalid_value++; //标志加1
						average_to_sum_x = 0;
						average_to_sum_y = 0;
						average_to_sum_z = 0;
					}
				}
				count_speed_cut = 0;
				Count_invalid_value = 0;
				speed_cut_self_lock = 1;//减速自锁
				speed_up_self_lock = 0;//加速解锁
			}
		}		
	}
}


