#ifndef PRINT_H
#define PRINT_H
#include "../userprog/syscall.h"

int print(const char* fmt, ...)  
{   
	const char* digit = "0123456789";
	char c; 
	int j= 0;
	char* str;
	int d = 0;
	int len = 0;
	char digitStr[10];
	
	va_list arg_ptr; 
	va_start(arg_ptr, fmt);
	do 
	{ 
		c = *fmt; 
		if (c != '%') 
		{ 
			Write(&c, sizeof(c), ConsoleOutput);  
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
						digitStr[len+1] = '\0';
						while (len > 0) {
							digitStr[len] = digit[d % 10];
							d = d / 10;
							len--;
						}
						digitStr[0] = '-';
					}else if (j == 0) {
						digitStr[0] = '0';
						digitStr[1] = '\0';
					}else {
						d = j;
						while (d != 0) {
							len++;
							d = d / 10;
						}
						d = j;
						digitStr[len] = '\0';
						while (len > 0) {
							digitStr[len-1] = digit[d % 10];
							d = d / 10;
							len--;
						}
					}
					Write(digitStr, sizeof(digitStr), ConsoleOutput);
					break; 
				case 's': 
					str = va_arg(arg_ptr, char*);
					Write(str, sizeof(str), ConsoleOutput);
					break; 
				default: 
					break; 
			}   
			
		} 
		++fmt; 
	}while (*fmt != '\0'); 
	va_end(args);
	return 0;   
} 
#endif