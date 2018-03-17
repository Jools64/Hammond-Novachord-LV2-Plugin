// Generic type and helper function definitions
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
#define s32 int32_t
#define s16 int16_t
#define s8 int8_t
#define f32 float
#define f64 double
#define null 0
#define true 1
#define false 0

// Enumeration of I/O ports
typedef enum PortIndex 
{
    NOVACHORD_CONTROL = 2,
    NOVACHORD_NOTIFY = 3,
    NOVACHORD_INPUT = 0,
    NOVACHORD_OUTPUT = 1,

    NOVACHORD_GAIN = 4,
    
    NOVACHORD_DEEP_TONE = 5, 
    NOVACHORD_FIRST_RESONATOR = 6, 
    NOVACHORD_SECOND_RESONATOR = 7, 
    NOVACHORD_THIRD_RESONATOR = 8,
    NOVACHORD_BRILLIANT_TONE = 9, 
    NOVACHORD_FULL_TONE = 10, 
    NOVACHORD_MELLOW = 11,
    NOVACHORD_BALANCE = 12, 
    NOVACHORD_ATTACK = 13,
    NOVACHORD_VOLUME = 14,
    NOVACHORD_NORMAL_VIBRATO = 15, 
    NOVACHORD_SMALL_VIBRATO = 16,
    NOVACHORD_LEFT_SUSTAIN = 17, 
    NOVACHORD_SWELL = 18,
    NOVACHORD_RIGHT_SUSTAIN = 19 

} PortIndex;