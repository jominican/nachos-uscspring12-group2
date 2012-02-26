#ifndef PRINT_H
#define PRINT_H
#include "../userprog/syscall.h"

int length(const char *buf)
{
    int i = 0;
	while (*buf++){i++;}
    return i;
}

int print(const char* fmt, ...)  
{   
	char buf[256];
	const char* digit = "0123456789";
	char c; 
	int j = 0;
	int i = 0;
	char* str;
	char* s;
	int d = 0;
	int len = 0;
	char digitStr[10];
	va_list arg_ptr; 
	va_start(arg_ptr, fmt);
	for (i = 0; i<256; i++) {
		buf[i] = '\0';
	}
	for (i = 0; i<10; i++) {
		digitStr[i] = '\0';
	}
	s = buf;
	do 
	{ 
		c = *fmt; 
		if (c != '%') 
		{ 
			*s++ = c;  
		} 
		else 
		{ 
			fmt = fmt + 1;
			switch(*(fmt))   
			{ 
				case 'd': 
					j = va_arg(arg_ptr, int);
					if (j < 0) {
						d = j;
						d = -d;
						while (d != 0) {
							len++;
							d = d / 10;
						}
						d = -j;
						while (len > 0) {
							digitStr[len] = digit[d % 10];
							d = d / 10;
							len--;
						}
						digitStr[0] = '-';
					}else if (j == 0) {
						digitStr[0] = '0';
					}else {
						d = j;
						while (d != 0) {
							len++;
							d = d / 10;
						}
						d = j;
						while (len > 0) {
							digitStr[len-1] = digit[d % 10];
							d = d / 10;
							len--;
						}
					}
					i = 0;
					while (digitStr[i]){
						*s++ = digitStr[i];
						i++;
					}
					break; 
				case 's': 
					str = va_arg(arg_ptr, char*);
					while(*str){
						c = *str;
						*s++ = c;  
						str++;
					}
					break; 
				default: 
					break; 
			}   
			
		} 
		++fmt; 
	}while (*fmt != '\0'); 
	*s = '\0';
	va_end(arg_ptr);
	len = length(buf);
	Write(buf, len, ConsoleOutput);

	return 0;   
} 
#endif