  #ifndef KEYNUM_H
   #define KEYNUM_H
/* 红外遥控器按键命令字节，数值以遥控器键码图为准。
 * 使用时与解码得到的命令字节比较，不是完整红外数据帧。
 * 外层 KEYNUM_H 宏用于防止头文件被重复包含。
 */
#define IR_POWER   0xA2  /* 电源键 */
#define IR_MENU    0xE2  /* 菜单键 */
#define IR_TEST    0x22  /* 测试键 */
#define IR_PLUS    0x02  /* 加号键 */
#define IR_RETURN  0xC2  /* 返回键 */
#define IR_LEFT    0xE0  /* 向左键 */
#define IR_PLAY    0xA8  /* 播放/确认键 */
#define IR_RIGHT   0x90  /* 向右键 */
#define IR_0       0x68  /* 数字 0 键 */
#define IR_MINUS   0x98  /* 减号键 */
#define IR_C       0xB0  /* C 键 */

#define IR_1       0x30  /* 数字 1 键 */
#define IR_2       0x18  /* 数字 2 键 */
#define IR_3       0x7A  /* 数字 3 键 */
#define IR_4       0x10  /* 数字 4 键 */
#define IR_5       0x38  /* 数字 5 键 */
#define IR_6       0x5A  /* 数字 6 键 */
#define IR_7       0x42  /* 数字 7 键 */
#define IR_8       0x4A  /* 数字 8 键 */
#define IR_9       0x52  /* 数字 9 键 */
 #endif /* KEYNUM_H */
 