#include <time.h>

void datetime_init(void);
struct tm datetime_now(void);
void datetime_datef(char* buff, size_t buff_size, struct tm* timeinfo);
void datetime_timef(char* buff, size_t buff_size, struct tm* timeinfo);