/*
 * dobot.h — 越疆机器人 TCP/IP 协议 C 语言底层接口
 * 
 * 编译: gcc -o example example.c dobot.c
 * 依赖: 仅需标准 C 库 + POSIX socket (Linux/嵌入式通用)
 */

#ifndef DOBOT_H
#define DOBOT_H

#include <stdint.h>   // uint16_t, uint64_t, int8_t

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  反馈数据包结构体 (端口 30004, 1440 字节定长, 小端序, 紧凑打包)
 *  对应 Python 端 MyType (numpy dtype, 总大小 1440 bytes)
 * ================================================================ */
#pragma pack(push, 1)
typedef struct {
    /* offset 0 */
    uint16_t len;
    int8_t   reserve[6];

    /* offset 8 */
    uint64_t DigitalInputs;       // 数字输入
    uint64_t DigitalOutputs;      // 数字输出
    uint64_t RobotMode;           // 4=断电 5=使能空闲 6=拖动 7=运行 9=报警
    uint64_t TimeStamp;
    uint64_t RunTime;
    uint64_t TestValue;           // 校验魔数 0x123456789abcdef

    /* offset 56 */
    int8_t   reserve2[8];
    double   SpeedScaling;

    /* offset 72 */
    int8_t   reserve3[16];
    double   VRobot;
    double   IRobot;
    double   ProgramState;

    /* offset 112 */
    uint16_t SafetyOIn;
    uint16_t SafetyOOut;

    /* offset 116 */
    int8_t   reserve4[76];

    /* offset 192 */
    double   QTarget[6];          // 目标关节角
    double   QDTarget[6];
    double   QDDTarget[6];
    double   ITarget[6];
    double   MTarget[6];

    /* offset 432 */
    double   QActual[6];          // 实际关节角 (degrees)
    double   QDActual[6];
    double   IActual[6];
    double   ActualTCPForce[6];
    double   ToolVectorActual[6];
    double   TCPSpeedActual[6];
    double   TCPForce[6];
    double   ToolVectorTarget[6];
    double   TCPSpeedTarget[6];
    double   MotorTemperatures[6];
    double   JointModes[6];
    double   VActual[6];

    /* offset 1008 */
    int8_t   HandType[4];
    int8_t   User;
    int8_t   Tool;
    int8_t   RunQueuedCmd;
    int8_t   PauseCmdFlag;
    int8_t   VelocityRatio;
    int8_t   AccelerationRatio;
    int8_t   reserve5;
    int8_t   XYZVelocityRatio;
    int8_t   RVelocityRatio;
    int8_t   XYZAccelerationRatio;
    int8_t   RAccelerationRatio;
    int8_t   reserve6[2];

    /* offset 1025 */
    int8_t   BrakeStatus;
    int8_t   EnableStatus;
    int8_t   DragStatus;
    int8_t   RunningStatus;
    int8_t   ErrorStatus;
    int8_t   JogStatusCR;
    int8_t   CRRobotType;
    int8_t   DragButtonSignal;
    int8_t   EnableButtonSignal;
    int8_t   RecordButtonSignal;
    int8_t   ReappearButtonSignal;
    int8_t   JawButtonSignal;
    int8_t   SixForceOnline;
    int8_t   CollisionState;
    int8_t   ArmApproachState;
    int8_t   J4ApproachState;
    int8_t   J5ApproachState;
    int8_t   J6ApproachState;

    /* offset 1043 */
    int8_t   reserve7[61];
    double   VibrationDisZ;        // offset 1104
    uint64_t CurrentCommandId;     // offset 1112 — 当前执行指令ID

    /* offset 1120 */
    double   MActual[6];
    double   Load;
    double   CenterX;
    double   CenterY;
    double   CenterZ;

    /* offset 1200 */
    double   UserValue[6];
    double   ToolValue[6];

    /* offset 1296 */
    int8_t   reserve8[8];

    /* offset 1304 */
    double   SixForceValue[6];
    double   TargetQuaternion[4];
    double   ActualQuaternion[4];

    /* offset 1416 */
    uint16_t AutoManualMode;
    uint16_t ExportStatus;
    int8_t   SafetyState;
    int8_t   reserve9[19];
} FeedbackPacket;
#pragma pack(pop)

/* 编译期断言结构体大小 (C11 _Static_assert) */
_Static_assert(sizeof(FeedbackPacket) == 1440, "FeedbackPacket must be 1440 bytes");

/* ================================================================
 *  API 函数声明
 * ================================================================ */

/* 连接机器人控制端口 (29999)，成功返回 socket fd，失败返回 -1 */
int dobot_connect_dashboard(const char *ip);

/* 连接机器人反馈端口 (30004)，成功返回 socket fd，失败返回 -1 */
int dobot_connect_feedback(const char *ip);

/* 发送指令并读取回复 (端口 29999)
 * 返回: 回复字符串 (静态缓冲区，下次调用会覆盖)，失败返回 NULL */
const char *dobot_send_cmd(int fd, const char *cmd);

/* 读取一帧反馈数据 (端口 30004)，阻塞直到收到 1440 字节
 * 返回: 0 成功，-1 失败
 * 注意: data 指向 1440 字节的 FeedbackPacket */
int dobot_read_feedback(int fd, FeedbackPacket *data);

/* 关闭连接 */
void dobot_close(int fd);

/* 便捷指令封装 */
const char *dobot_EnableRobot(int fd);
const char *dobot_DisableRobot(int fd);
const char *dobot_ClearError(int fd);
const char *dobot_Stop(int fd);
const char *dobot_RobotMode(int fd);
const char *dobot_PowerOn(int fd);
const char *dobot_SpeedFactor(int fd, int percent);
const char *dobot_StartDrag(int fd);
const char *dobot_StopDrag(int fd);

/* ServoJ: 实时关节伺服控制 (非阻塞, 高频率调用, 用于拖动跟随/回放)
 *   t:     运行时间(秒), -1 使用默认 0.05
 *   gain:  比例增益(200~1000), -1 使用默认 500        */
const char *dobot_ServoJ(int fd,
        double j1, double j2, double j3, double j4, double j5, double j6,
        double t, double gain);

const char *dobot_MovJ_pose(int fd, double x, double y, double z,
                            double rx, double ry, double rz);
const char *dobot_MovL_pose(int fd, double x, double y, double z,
                            double rx, double ry, double rz);
const char *dobot_GetAngle(int fd);
const char *dobot_GetPose(int fd);

#ifdef __cplusplus
}
#endif

#endif /* DOBOT_H */
