/**
 * @file tsk_config_and_callback.cpp
 * @author lez by yssickjgd
 * @brief 临时任务调度测试用函数, 后续用来存放个人定义的回调函数以及若干任务
 * @version 0.1
 * @date 2024-07-1 0.1 24赛季定稿
 * @copyright ZLLC 2024
 */

/**
 * @brief 注意, 每个类的对象分为专属对象Specialized, 同类可复用对象Reusable以及通用对象Generic
 *
 * 专属对象:
 * 单对单来独打独
 * 比如交互类的底盘对象, 只需要交互对象调用且全局只有一个, 这样看来, 底盘就是交互类的专属对象
 * 这种对象直接封装在上层类里面, 初始化在上层类里面, 调用在上层类里面
 *
 * 同类可复用对象:
 * 各调各的
 * 比如电机的对象, 底盘可以调用, 云台可以调用, 而两者调用的是不同的对象, 这种就是同类可复用对象
 * 电机的pid对象也算同类可复用对象, 它们都在底盘类里初始化
 * 这种对象直接封装在上层类里面, 初始化在最近的一个上层专属对象的类里面, 调用在上层类里面
 *
 * 通用对象:
 * 多个调用同一个
 * 比如裁判系统对象, 底盘类要调用它做功率控制, 发射机构要调用它做出膛速度与射击频率的控制, 因此裁判系统是通用对象.
 * 这种对象以指针形式进行指定, 初始化在包含所有调用它的上层的类里面, 调用在上层类里面
 *
 */

/**
 * @brief TIM开头的默认任务均1ms, 特殊任务需额外标记时间
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "tsk_config_and_callback.h"
#include "drv_bsp-boarda.h"
#include "drv_tim.h"
// #include "dvc_boarda-mpuahrs.h"
#include "dvc_boardc_bmi088.h"
#include "dvc_dmmotor.h"
#include "dvc_serialplot.h"
#include "ita_chariot.h"
#include "dvc_boardc_ist8310.h"
#include "dvc_imu.h"
#include "CharSendTask.h"
#include "GraphicsSendTask.h"
#include "drv_usb.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "config.h"
// #include "GraphicsSendTask.h"
// #include "ui.h"
#include "dvc_GraphicsSendTask.h"

/* Private macros ------------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/
extern float sum; // 用于测试
/* Private variables ---------------------------------------------------------*/
float theory_power;
uint32_t init_finished = 0;
bool start_flag = 0;
// 机器人控制对象
Class_Chariot chariot;

// 串口裁判系统对象
Class_Serialplot serialplot;

/* Private function declarations ---------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Chassis_CAN1回调函数
 *
 * @param CAN_RxMessage CAN1收到的消息
 */
__fp16 test_k1, test_k2;
#ifdef CHASSIS
void Chassis_Device_CAN1_Callback(Struct_CAN_Rx_Buffer *CAN_RxMessage)
{
    switch (CAN_RxMessage->Header.StdId)
    {
    case (0x201):
    {
        chariot.Chassis.Motor_Wheel[0].CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x202):
    {
        chariot.Chassis.Motor_Wheel[1].CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x203):
    {
        chariot.Chassis.Motor_Wheel[2].CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x204):
    {
        chariot.Chassis.Motor_Wheel[3].CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x206):
    {
    }
    break;
    case (0x207):
    {
    }
    break;
    case (0x20E):
    {
        // chariot.Chassis.Agv_Board[1].CAN_RxCpltCallback(CAN_RxMessage->Data);
        memcpy(&sum, CAN_RxMessage->Data, 4);
        memcpy(&test_k1, CAN_RxMessage->Data + 4, 2);
        memcpy(&test_k2, CAN_RxMessage->Data + 6, 2);
    }
    break;
    }
}
#endif
/**
 * @brief Chassis_CAN2回调函数
 *
 * @param CAN_RxMessage CAN2收到的消息
 */
#ifdef CHASSIS
void Chassis_Device_CAN2_Callback(Struct_CAN_Rx_Buffer *CAN_RxMessage)
{
    switch (CAN_RxMessage->Header.StdId)
    {
    case (0x208):
    {
    }
    break;
    case (0x150): // 留给上板通讯
    {
        chariot.CAN_Chassis_Rx_Gimbal_Callback(CAN_RxMessage->Data);
    }
    break;
    case (0x152): // 留给上板通讯
    {
        chariot.CAN_Chassis_Rx_Gimbal_Callback_State(CAN_RxMessage->Data);
    }
    break;
    case (0x206): // 留给yaw电机编码器回传 用于底盘随动
    {
        chariot.Motor_Yaw.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x67): // 留给超级电容
    {
        chariot.Chassis.Supercap.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case(0x55):
    {
        chariot.Chassis.Supercap.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    }
}
#endif
/**
 * @brief Gimbal_CAN1回调函数
 *
 * @param CAN_RxMessage CAN1收到的消息
 */
#ifdef GIMBAL
void Gimbal_Device_CAN1_Callback(Struct_CAN_Rx_Buffer *CAN_RxMessage)
{
    switch (CAN_RxMessage->Header.StdId)
    {
    case (0x203):
    {
        chariot.Booster.Motor_Driver.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x201):
    {
        chariot.Booster.Motor_Friction_Left.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x202):
    {
        chariot.Booster.Motor_Friction_Right.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x205):
    {
        chariot.Gimbal.Motor_Pitch.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x141):
    {
        chariot.Gimbal.Motor_Pitch_LK6010.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    }
}
#endif
/**
 * @brief Gimbal_CAN1回调函数
 *
 * @param CAN_RxMessage CAN2收到的消息
 */
#ifdef GIMBAL
void Gimbal_Device_CAN2_Callback(Struct_CAN_Rx_Buffer *CAN_RxMessage)
{
    switch (CAN_RxMessage->Header.StdId)
    {
    case (0x88): // 留给下板通讯
    {
        chariot.CAN_Gimbal_Rx_Chassis_Callback();
    }
    break;
    case (0x208):
    {
    }
    break;
    case (0x204):
    {
    }
    break;
    case (0x206): // 保留can2对6020编码器的接口
    {
        chariot.Gimbal.Motor_Yaw.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    case (0x205):
    {
        chariot.Gimbal.Motor_Pitch.CAN_RxCpltCallback(CAN_RxMessage->Data);
    }
    break;
    }
}
#endif
/**
 * @brief SPI5回调函数
 *
 * @param Tx_Buffer SPI5发送的消息
 * @param Rx_Buffer SPI5接收的消息
 * @param Length 长度
 */
// void Device_SPI5_Callback(uint8_t *Tx_Buffer, uint8_t *Rx_Buffer, uint16_t Length)
//{
//     if (SPI5_Manage_Object.Now_GPIOx == BoardA_MPU6500_CS_GPIO_Port && SPI5_Manage_Object.Now_GPIO_Pin == BoardA_MPU6500_CS_Pin)
//     {
//         boarda_mpu.SPI_TxRxCpltCallback(Tx_Buffer, Rx_Buffer);
//     }
// }

/**
 * @brief SPI1回调函数
 *
 * @param Tx_Buffer SPI1发送的消息
 * @param Rx_Buffer SPI1接收的消息
 * @param Length 长度
 */
void Device_SPI1_Callback(uint8_t *Tx_Buffer, uint8_t *Rx_Buffer, uint16_t Length)
{
}

/**
 * @brief UART1图传回调函数
 *
 * @param Buffer UART1收到的消息
 * @param Length 长度
 */
#ifdef GIMBAL
void Image_UART6_Callback(uint8_t *Buffer, uint16_t Length)
{
    chariot.DR16.Image_UART_RxCpltCallback(Buffer);

    // 底盘 云台 发射机构 的控制策略
    chariot.TIM_Control_Callback();
}
#endif

/**
 * @brief UART3遥控器回调函数
 *
 * @param Buffer UART1收到的消息
 * @param Length 长度
 */
#ifdef GIMBAL
void DR16_UART3_Callback(uint8_t *Buffer, uint16_t Length)
{

    chariot.DR16.DR16_UART_RxCpltCallback(Buffer);

    // 底盘 云台 发射机构 的控制策略
    chariot.TIM_Control_Callback();
}
#endif

/**
 * @brief IIC磁力计回调函数
 *
 * @param Buffer IIC收到的消息
 * @param Length 长度
 */
void Ist8310_IIC3_Callback(uint8_t *Tx_Buffer, uint8_t *Rx_Buffer, uint16_t Tx_Length, uint16_t Rx_Length)
{
}

/**
 * @brief UART裁判系统回调函数
 *
 * @param Buffer UART收到的消息
 * @param Length 长度
 */
#ifdef CHASSIS

void Referee_UART6_Callback(uint8_t *Buffer, uint16_t Length)
{
    chariot.Referee.UART_RxCpltCallback(Buffer, Length);
}
#endif
/**
 * @brief UART1超电回调函数
 *
 * @param Buffer UART1收到的消息
 * @param Length 长度
 */
#if defined CHASSIS && defined POWER_LIMIT
void SuperCAP_UART1_Callback(uint8_t *Buffer, uint16_t Length)
{
    chariot.Chassis.Supercap.UART_RxCpltCallback(Buffer);
}

#endif
/**
 * @brief USB MiniPC回调函数
 *
 * @param Buffer USB收到的消息
 *
 * @param Length 长度
 */
#ifdef GIMBAL
void MiniPC_USB_Callback(uint8_t *Buffer, uint32_t Length)
{
    chariot.MiniPC.USB_RxCpltCallback(Buffer);
}
#endif
/**
 * @brief TIM4任务回调函数
 *
 */
static uint16_t start_time = 0;
static uint16_t delta_time = 0;
void Task100us_TIM4_Callback()
{
#ifdef CHASSIS
    // 定义一个静态变量Referee_Sand_Cnt，用于计数
    static uint16_t Referee_Sand_Cnt = 0;
    // //暂无云台tim4任务
    if (Referee_Sand_Cnt % 50 == 1)
    {
        start_time = DWT_GetTimeline_us();
        //Task_Loop();
        delta_time= DWT_GetTimeline_us() - start_time;
            Referee_Sand_Cnt = 0;
    }

    Referee_Sand_Cnt++;

#elif defined(GIMBAL)
    // 单给IMU消息开的定时器 ims
    chariot.Gimbal.Boardc_BMI.TIM_Calculate_PeriodElapsedCallback();
#endif
}

/**
 * @brief TIM5任务回调函数
 *
 */
float actual = 0;
void Task1ms_TIM5_Callback()
{
    init_finished++;
    if (init_finished > 2000)
        start_flag = 1;

    /************ 判断设备在线状态判断 50ms (所有device:电机，遥控器，裁判系统等) ***************/

    chariot.TIM1msMod50_Alive_PeriodElapsedCallback();

    /****************************** 交互层回调函数 1ms *****************************************/
    if (start_flag == 1)
    {
#ifdef GIMBAL
        chariot.FSM_Alive_Control.Reload_TIM_Status_PeriodElapsedCallback();
#endif
        chariot.TIM_Calculate_PeriodElapsedCallback();

        /****************************** 驱动层回调函数 1ms *****************************************/
        // 统一打包发送
        TIM_CAN_PeriodElapsedCallback();

        // TIM_UART_PeriodElapsedCallback();

        static int mod5 = 0;
        mod5++;
        if (mod5 == 5)
        {
            TIM_USB_PeriodElapsedCallback(&MiniPC_USB_Manage_Object);
            mod5 = 0;
        }
    }
}

/**
 * @brief 初始化任务
 *
 */
extern "C" void Task_Init()
{

    DWT_Init(168);

/********************************** 驱动层初始化 **********************************/
#ifdef CHASSIS

    // 集中总线can1/can2
    CAN_Init(&hcan1, Chassis_Device_CAN1_Callback);
    CAN_Init(&hcan2, Chassis_Device_CAN2_Callback);

    // 裁判系统
    UART_Init(&huart6, Referee_UART6_Callback, 128); // 并未使用环形队列 尽量给长范围增加检索时间 减少丢包

#ifdef POWER_LIMIT
// 旧版超电
// UART_Init(&huart1, SuperCAP_UART1_Callback, 128);
#endif

#endif

#ifdef GIMBAL

    // 集中总线can1/can2
    CAN_Init(&hcan1, Gimbal_Device_CAN1_Callback);
    CAN_Init(&hcan2, Gimbal_Device_CAN2_Callback);

    // c板陀螺仪spi外设
    SPI_Init(&hspi1, Device_SPI1_Callback);

    // 磁力计iic外设
    IIC_Init(&hi2c3, Ist8310_IIC3_Callback);

    // 遥控器接收
    UART_Init(&huart3, DR16_UART3_Callback, 18);
    UART_Init(&huart6, Image_UART6_Callback, 40);

    // 上位机USB
    USB_Init(&MiniPC_USB_Manage_Object, MiniPC_USB_Callback);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

#endif

    // 定时器循环任务
    TIM_Init(&htim4, Task100us_TIM4_Callback);
    TIM_Init(&htim5, Task1ms_TIM5_Callback);

    /********************************* 设备层初始化 *********************************/

    // 设备层集成在交互层初始化中，没有显视地初始化

    /********************************* 交互层初始化 *********************************/

    chariot.Init();

    /********************************* 使能调度时钟 *********************************/

    HAL_TIM_Base_Start_IT(&htim4);
    HAL_TIM_Base_Start_IT(&htim5);
}
float Chassis_Power;
float buff_power;
/**
 * @brief 前台循环任务
 *
 */
extern "C" void Task_Loop()
{
#ifdef GIMBAL
    float now_angle_yaw = chariot.Gimbal.Motor_Yaw.Get_True_Angle_Yaw();
    float target_angle_yaw = chariot.Gimbal.MiniPC->Get_Rx_Yaw_Angle();
    // 如果是自瞄开启并且距离装甲板的瞄准弧度小于0.1m
    if (chariot.Gimbal.Get_Gimbal_Control_Type() == Gimbal_Control_Type_MINIPC &&
        (chariot.Gimbal.MiniPC->Get_Distance() * abs(now_angle_yaw - target_angle_yaw) / 180.0f * PI) < 0.1)
    {
        chariot.MiniPC_Aim_Status = MinPC_Aim_Status_ENABLE;
    }
    else
    {
        chariot.MiniPC_Aim_Status = MinPC_Aim_Status_DISABLE;
    }
#endif
#ifdef CHASSIS
   if (start_flag == 1)
   {
       JudgeReceiveData.robot_id = chariot.Referee.Get_ID();
       // JudgeReceiveData.robot_id = Referee_Data_Robots_ID_RED_HERO_1;
       JudgeReceiveData.Pitch_Angle = chariot.Gimbal_Tx_Pitch_Angle;   // pitch角度
       JudgeReceiveData.Bullet_Status = chariot.Bulletcap_Status;      // 弹舱
       JudgeReceiveData.Fric_Status = chariot.Fric_Status;             // 摩擦轮
       JudgeReceiveData.Minipc_Satus = chariot.MiniPC_Status;          // 自瞄是否离线
       JudgeReceiveData.MiniPC_Aim_Status = chariot.MiniPC_Aim_Status; // 自瞄是否瞄准
       // JudgeReceiveData.Supercap_Energy = chariot.Chassis.Supercap.Get_Stored_Energy();    // 超级电容储能
       // JudgeReceiveData.Supercap_Voltage = chariot.Chassis.Supercap.Get_Now_Voltage();     // 超级电容电压
       JudgeReceiveData.Chassis_Control_Type = chariot.Chassis.Get_Chassis_Control_Type(); // 底盘控制模式
       if (chariot.Referee_UI_Refresh_Status == Referee_UI_Refresh_Status_ENABLE)
           Init_Cnt = 10;
       GraphicSendtask();
       // DWT_Delay(0.1);
   }

    //  Chassis_Power = chariot.Chassis.Supercap.Get_Chassis_Power();
    //  buff_power = chariot.Chassis.Supercap.Get_Buffer_Power();
    //  printf("%f,%f\n", Chassis_Power, buff_power);
    //  DWT_Delay(0.01);
    
#endif
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
