// Placeholder Code:
// Calls custom system call on linux, works after
// system call is added to the linux kernel.
// Tested on Ubuntu 16.04 on linux-4.17.4 
// on Oracle VM VirtualBox VM Version 6.1.32 r149290 (Qt5.12.8)
// Reference: 
// https://medium.com/anubhav-shrimal/adding-a-hello-world-system-call-to-linux-kernel-dad32875872

#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
int main()
{
    long int amma = syscall(548);
    printf("System call sys_hello returned %ld\n", amma);
    return 0;
}

// Check output of "dmesg" after compile and run