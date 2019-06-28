#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sched.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <syscall.h>

void do_server()
{
	// source from https://rtfm.co.ua/c-sokety-i-primer-modeli-client-server/
	int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025];
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
                                                                                                                                                                              
    ticks = time(NULL);
    snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
    write(connfd, sendBuff, strlen(sendBuff));

    close(connfd);
    sleep(1);

}

void print_process_info()
{
	printf("PID of the process: %ld \nPID of parent process: %ld \n",
	 (long)getpid(), (long)getppid());
	printf("ls result: \n");
	system("ls");
	printf("\npwd result: \n");
	system("pwd");
	printf("\nMounted file systems: \n");
	//system("df -hT");
	printf("\nIp link result:\n\n");
	system("ip link");
	printf("\n\nProcess list:\n\n");
	system("ps aux");
	printf("\n\n");
}

void show_benchmarks()
{
	system("sysbench --test=cpu --cpu-max-prime=2000 run");	//run cpu test
	system("sysbench --test=fileio --file-total-size=200M prepare");
	system("sysbench --test=fileio --file-total-size=200M --file-test-mode=rnddrw --init-rng=on --max-time=300 --max-requests=0 run"); //HDD test
}

void create_mount_point()
{
	system("mount proc /proc -t proc");	//we mount it to isolate info about processes
	//proc is a pseudo file system with information about processes
}

void create_file_system()
{
	system("dd if=/dev/zero of=my_drive.img bs=1M count=100"); //creata a file which will be used as a file system
	system("mkfs -t ext4 ./my_drive.img");	//format a file to ext4 file system
	system("mkdir /mnt/VHD_my_container/");
	system("mount -t auto -o loop ./my_drive.img /mnt/VHD_my_container/");
}

void add_ip_address()
{
	system("ifconfig enp0s3 192.168.3.17 up");	//this command will add new adress to the network device
	//uncfortunately device enp0s3 and ip are hardcoded now, but it might be improved in the future
}

static int child_process()
{
	unshare(CLONE_NEWNET);	//isolate network interfaces	
	create_mount_point();	//isolate processes
	create_file_system();	//create and mount new ext4 file system
	add_ip_address();	//creates new ip for the net device
	printf("Child process started\n");
	print_process_info();
	printf("Run tests...");
	show_benchmarks();
	printf("Starting of server...");
	do_server();
	return 0;
}

static char child_stack[1048576];

int main(int argc, char *argv[])
{
	printf("Main process started\n");
	print_process_info();
	printf("Run tests...");
	show_benchmarks();
	pid_t child_pid = clone(child_process, child_stack+1048576, CLONE_NEWPID | CLONE_NEWNET |CLONE_NEWNS | CLONE_NEWIPC | SIGCHLD, NULL);
	if (child_pid < 0)
		perror("Unable to clone process: ");
	printf("clone() = %ld\n", (long)child_pid);
 	
 	waitpid(child_pid, NULL, 0);
	return 0;
}
