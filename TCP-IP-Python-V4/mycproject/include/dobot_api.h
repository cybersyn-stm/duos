#ifndef __DOBOT_API_H__
#define __DOBOT_API_H__

#include "stdint.h"
#include "sys/socket.h"

#define DOBOT_API_CONTROL_PORT 29999
#define DOBOT_API_FEEDBACK_PORT 30004

typedef struct {
    uint16_t len;
    int8_t reserve[6];
    uint64_t DigitalInputs;
    uint64_t DigitalOutputs;
    uint64_t RobotMode;
    uint64_t TimeStamp;
    uint64_t RunTime;
    uint64_t TestValue;
    int8_t reserve2[8];
    double SpeedScaling;
    int8_t reserve3[16];
    double VRobot;
    double IRobot;
    double ProgramState;
    uint16_t SafetyOIn;
    uint16_t SafetyOOut;
    int8_t reserve4[76];
    double QTarget[6];
    double QDTarget[6];
    double QDDTarget[6];
    double ITarget[6];
    double MTarget[6];
    double QActual[6];
    double QDActual[6];
    double IActual[6];
    double ActualTCPForce[6];
    double ToolVectorActual[6];
    double TCPSpeedActual[6];
    double TCPForce[6];
    double ToolVectorTarget[6];
    double TCPSpeedTarget[6];
    double MotorTemperatures[6];
    double JointModes[6];
    double VActual[6];
    int8_t HandType[4];
    int8_t User;
    int8_t Tool;
    int8_t RunQueuedCmd;
    int8_t PauseCmdFlag;
    int8_t VelocityRatio;
    int8_t AccelerationRatio;
    int8_t reserve5;
    int8_t XYZVelocityRatio;
    int8_t RVelocityRatio;
    int8_t XYZAccelerationRatio;
    int8_t RAccelerationRatio;
    int8_t reserve6[2];
    int8_t BrakeStatus;
    int8_t EnableStatus;
    int8_t DragStatus;
    int8_t RunningStatus;
    int8_t ErrorStatus;
    int8_t JogStatusCR;
    int8_t CRRobotType;
    int8_t DragButtonSignal;
    int8_t EnableButtonSignal;
    int8_t RecordButtonSignal;
    int8_t ReappearButtonSignal;
    int8_t JawButtonSignal;
    int8_t SixForceOnline;
    int8_t CollisionState;
    int8_t ArmApproachState;
    int8_t J4ApproachState;
    int8_t J5ApproachState;
    int8_t J6ApproachState;
    int8_t reserve7[61];
    double VibrationDisZ;
    uint64_t CurrentCommandId;
    double MActual[6];
    double Load;
    double CenterX;
    double CenterY;
    double CenterZ;
    double UserValue[6]; /* 原字段名 'UserValue[6]' */
    double ToolValue[6]; /* 原字段名 'ToolValue[6]' */
    int8_t reserve8[8];
    double SixForceValue[6];
    double TargetQuaternion[4];
    double ActualQuaternion[4];
    uint16_t AutoManualMode;
    uint16_t ExportStatus;
    int8_t SafetyState;
    int8_t reserve9[19];
} FeedbackData;

_Static_assert(sizeof(FeedbackData) == 1440, "Size mismatch!");

/*==============DOBOT_API================*/
static int DOBOT_TCP_Connect(char *ip, int port);
#endif
