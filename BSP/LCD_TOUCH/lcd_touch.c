#include "lcd_touch.h" 
#include "lcd.h"
#include "math.h"

#define ERR_RANGE 50 	//误差范围 
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
//读X，读Y的命令
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;
_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Read_XY2,
	0,
	0, 
	4000,
	4000,
	120,	  	 		
	120,	 		
};					
 	 			    					   
/**
 * @brief SPI写数据
 * 
 * @param num 要写入的数据
 */
void TP_Write_Byte(uint8_t num)    
{  
	uint8_t count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)TDIN(1);  
		else TDIN(0);   
		num<<=1;    
		TCLK(0); 
		HAL_uDelay(1);
		TCLK(1);		//上升沿有效	        
	}		 			    
} 		 

/**
 * @brief 软件模拟SPI读数据
 * 
 * @param CMD 命令字
 * @return * uint16_t 读取到的数据
 */
uint16_t TP_Read_AD(uint8_t CMD)	  
{ 	 
	uint8_t count=0; 	  
	uint16_t Num=0; 
	TCLK(0);		//先拉低时钟 	 
	TDIN(0); 	//拉低数据线
	TCS(0); 		//选中触摸屏IC
	TP_Write_Byte(CMD);//发送命令字
	HAL_uDelay(6);//ADS7846的转换时间最长为6us
	TCLK(0); 	     	    
	HAL_uDelay(1);    	   
	TCLK(1);		//给1个时钟，清除BUSY
	HAL_uDelay(1);    
	TCLK(0); 	     	    
	for(count=0;count<15;count++)//读出16位数据,只有高12位有效 
	{ 				  
		
		TCLK(0);	//下降沿有效  	    	   
		HAL_uDelay(1);    
 		TCLK(1);
 		if(DOUT)Num++; 		 
		Num<<=1;
	}  	
	Num>>=4;   	//只有高12位有效.
	TCS(1);		//释放片选	 
	return(Num);   
}

/**
 * @brief 读取X或Y坐标值，会进行中值滤波，通过READ_TIMES控制读取次数，通过LOST_VAL控制两端数据丢弃量
 * 
 * @param xy 读取的坐标 CMD_RDX/CMD_RDY
 * @return uint16_t 读取到的坐标值
 */
uint16_t TP_Read_XOY(uint8_t xy)
{
	uint16_t i, j;
	uint16_t buf[READ_TIMES];
	uint16_t sum=0;
	uint16_t temp;
	for(i=0;i<READ_TIMES;i++){
		buf[i]=TP_Read_AD(xy);
	}
		 		    
	for(i=0;i<READ_TIMES-1; i++)
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
/**
 * @brief 读取触摸屏的坐标值
 * 
 * @param x x坐标指针
 * @param y y坐标指针
 * @return uint8_t 1：读取成功 0：读取失败
 */
uint8_t TP_Read_XY(uint16_t *x,uint16_t *y)
{
	uint16_t xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);	  												   
	*x=xtemp;
	*y=ytemp;
	return 1;//读数成功
}

/**
 * @brief 读取触摸屏IC的坐标值，映射到屏幕坐标，会读取两次，如果两次读取的值差距在ERR_RANGE内，则返回平均值
 * 
 * @param x x坐标指针
 * @param y y坐标指针
 * @return uint8_t 1：读取成功 0：读取失败
 */
uint8_t TP_Read_XY2(uint16_t *x,uint16_t *y) 
{
	uint16_t x1,y1;
 	uint16_t x2,y2;
 	uint8_t flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
		*x = (*x-120.0f)/(3700)*240.0f;
		*y = (*y-120.0f)/(3700)*320.0f;
		x1 = *x;
        return 1;
    }else return 0;	  
}   
				  
/**
 * @brief 触摸屏初始化
 * 
 * @return uint8_t 1：初始化成功 0：初始化失败
 */
uint8_t TP_Init(void)
{	
    GPIO_InitTypeDef GPIO_Initure;
        
	__HAL_RCC_GPIOH_CLK_ENABLE();			//开启GPIOH时钟
	__HAL_RCC_GPIOI_CLK_ENABLE();			//开启GPIOI时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();			//开启GPIOG时钟
	
	//PH6
	GPIO_Initure.Pin=GPIO_PIN_6;            //PH6
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;     //高速
	HAL_GPIO_Init(GPIOH,&GPIO_Initure);     //初始化
	
	//PI3,8
	GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_8; //PI3,8
	HAL_GPIO_Init(GPIOI,&GPIO_Initure);     //初始化
	
	//PH7
	GPIO_Initure.Pin=GPIO_PIN_7;            //PH7
	GPIO_Initure.Mode=GPIO_MODE_INPUT;      //输入
	HAL_GPIO_Init(GPIOH,&GPIO_Initure);     //初始化
	
	//PG3
	GPIO_Initure.Pin=GPIO_PIN_3;            //PG3
	HAL_GPIO_Init(GPIOG,&GPIO_Initure);     //初始化
	 	
	return 1; 									 
}









