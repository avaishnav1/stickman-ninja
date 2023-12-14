/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 *
 */

#define ADDRESS 0x68
#define I2C_CHAN0 i2c0
#define I2C_CHAN1 i2c1
#define SDA_PIN0  12
#define SCL_PIN0  13
#define SDA_PIN1  14
#define SCL_PIN1  15
#define I2C_BAUD_RATE 400000

// Fixed point data type
typedef signed int fix15 ;
#define multfix15(a,b) ((fix15)(((( signed long long)(a))*(( signed long long)(b)))>>16)) 
#define float2fix15(a) ((fix15)((a)*65536.0f)) // 2^16
#define fix2float15(a) ((float)(a)/65536.0f) 
#define int2fix15(a) ((a)<<16)
#define fix2int15(a) ((a)>>16)
#define divfix(a,b) ((fix15)(((( signed long long)(a) << 16 / (b)))))
// Parameter values
#define oneeightyoverpi 3754936
#define zeropt001 65
#define zeropt999 65470
#define zeropt01 655
#define zeropt99 64880
#define zeropt1 6553
#define zeropt9 58982

// VGA primitives - usable in main
void mpu6050_reset(void) ;
void mpu6050_read_raw0(fix15 accel[3], fix15 gyro[3]) ;
void mpu6050_read_raw1(fix15 accel[3], fix15 gyro[3]) ;