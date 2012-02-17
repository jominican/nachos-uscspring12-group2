/* testfiles.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

char buf[200];
int len = 0;

void print(int x) {
	int num = 1;
	int t = x/10;
	int i;
	char a[11];
	while (t) {
		num++;
		t /= 10;
	}
	for(i = 1; i <= num; i++) {
		a[num-i] = (x%10) + '0';
		x /= 10;
	}
	for(i = 0; i < num; i++) {
		buf[len++] = a[i];
	}
	/*	Write(a, num, ConsoleOutput); */
}

void prints(char *a) {
	int i;
	for(i = 0; a[i]; i++)
		buf[len++] = a[i];
	if (buf[len-1] == '\n') {
		Write(buf,len,ConsoleOutput);
		len = 0;
	}
}

int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

	int lock;
	int cv;
	int i;
    Create("testfile", 8);
    fd = Open("testfile", 8);

    Write("testing a write\n", 16, fd );
    Close(fd);


    fd = Open("testfile", 8);
    bytesread = Read( buf, 100, fd );
    Write( buf, bytesread, ConsoleOutput );
    Close(fd);
	lock = CreateLock("a", 1);
	prints("Create ");print(lock);prints("\n");
	Acquire(lock);
	prints("Acquire Lock\n");
	for(i = 0; i < 100000; i++);
	Release(lock);
	prints("Release Lock\n");
	
	lock = CreateLock("a", 1);
	cv = CreateCondition("a", 1);
	Acquire(lock);
	Broadcast(cv,lock);
	Release(lock);
	DestroyLock(lock);
	DestroyCondition(cv); 
	
	lock = CreateLock("a", 1);
	cv = CreateCondition("a", 1);
	Acquire(lock);
	Wait(cv,lock);
	prints("Finish\n");
	Release(lock);
	DestroyLock(lock);
	DestroyCondition(cv);
		
	
}
