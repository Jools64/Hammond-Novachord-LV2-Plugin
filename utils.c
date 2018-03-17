// Utility macro functions
#define decibelCoeficient(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define clamp(value,min,max) value < min ? min : (value > max ? max : value)
#define allocate(type,count) (type*)calloc(count, sizeof(type))
#define new(type) allocate(type, 1)
#define sign(value) value < 0 ? -1 : (value > 0 ? 1 : 0)
#define max(a,b) a > b ? a : b
#define min(a,b) a < b ? a : b