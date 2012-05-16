/*  Author: Aishvarya Vishvesh Singh
 *  Rollno: 201001060  */

/*  Assumptions :
 *  1.In commands like "grep "new" temp.txt", string(to be searched) must not be put inside double quotes, it should be without quotes,something like,
 *    grep new temp.txt   : correct
 *    grep "new" temp.txt : wrong 
 *  2.Ctrl-D is not a signal. Its just a NULL input. So either press enter after one ctrl-D or press ctrl-D twice(because it waits for another input from       stdin after a normal input,so it is either '\n' or another ctrl-D).  */

#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>

char curdir[1000]="~";
char homedir[1000];
char history[1000][100];
int hind=0;

char pidname[1000][100];	//stores the name of the process
int pidstat[1000]={0};		//stores the status of the process(active/executed)
int pidnum[1000]; 		//stores the pid of the process
int pidcnt=0;			//pid count
int mainpid;

void update();
void redirection(char in[]);
void processpipe(char in[]);
void displaypid(char db[][100],int size);
void display() {
	char shelldisplay[1000];
	gethostname(shelldisplay,1000);
	update();
	printf("<%s@%s:%s> ",getenv("USER"),shelldisplay,curdir);
}

void sighandle(int signal) {
	printf("\n");
	display();
	fflush(stdout);
}

void sigchild(int signal) {
	int i;
	int ret=waitpid(-1,&i,WNOHANG);
	if (ret>0) {
		int j;
		for (j=0;j<pidcnt;j++) {
			if (ret==pidnum[j]) {
				printf("\ncommand %s pid:%d exitted normally\n",pidname[j],ret);
				pidstat[j]=0;
				break;
			}
		}
		display();
	}
	fflush(stdout);
}

int split(char input[],char db[][100]) {
	char copy[100];
	strcpy(copy,input);
	int size=0;
	char *temp;
	temp=strtok(input," \t	");
	while (temp != NULL) {
		strcpy(db[size],temp);
		size++;
		temp=strtok(NULL," \t	");
	}
	strcpy(input,copy);
	return size;
}

void changedir(char input[],char db[][100],int size) {
	char *temp;
	temp=strstr(input,"~");
	if (size==1) {
		chdir(homedir);
		curdir[0]='~';
		curdir[1]='\0';
	}
	else if (temp==NULL) {
		int ret=chdir(db[1]);
		if (ret==-1)
			printf("%s:No such file or directory\n",db[1]);
		else
			getcwd(curdir,sizeof(curdir));
	}
	else {
		int index=0;
		char another[1000];
		int val=0;
		while (db[1][index]!='~') {
			another[val]=db[1][index];
			val++;index++;
		}
		int cp=index+1;index=0;
		while (homedir[index]!='\0') {
			another[val]=homedir[index];
			val++;
			index++;
		}
		while (db[1][cp]!='\0') {
			another[val]=db[1][cp];
			val++;cp++;
		}
		another[val]='\0';
		chdir(another);
	}
	update();
}
void update() {
	getcwd(curdir,sizeof(curdir));
	if (strstr(curdir,homedir)) {
		int index=0;
		char another[1000]="~";
		int in=1;
		while (curdir[index]==homedir[index] && homedir[index]!='\0')
			index++;
		while (curdir[index]!='\0') {
			another[in]=curdir[index];
			in++;index++;
		}
		another[in]='\0';
		strcpy(curdir,another);
	}
}

void runcommand(char db[][100],int size) {
	char *st[100];
	int i;
	for (i=0;i<size;i++) 
		st[i]=db[i];
	st[i]=NULL;
	int pid=fork();
	strcpy(pidname[pidcnt],st[0]);
	pidstat[pidcnt]=0;
	pidnum[pidcnt]=pid;
	pidcnt++;
	if (pid==0) {
		int err=execvp(st[0],st);
		if (err==-1)  {
			printf("Command Not Found\n");
			char tty[100]="";
			strcpy(pidname[pidcnt-1],tty);
			pidcnt--;
		}
		exit(0);
	}
	else
		wait(NULL);
}

void runcommandback(char db[][100],int size) {
	int i;
	for (i=0;i<size;i++) {
		if (db[i][strlen(db[i])-1]=='&')
			db[i][strlen(db[i])-1]='\0';
	}
	char *st[100];
	for (i=0;i<size;i++) {
		st[i]=db[i];
	}
	if (st[i-1][0]=='\0')
		st[1]=NULL;
	st[i]=NULL;
	int pid=fork();
	strcpy(pidname[pidcnt],st[0]);
	pidstat[pidcnt]=1;
	pidnum[pidcnt]=pid;
	pidcnt++;
	if (pid==0) {
		int err=execvp(st[0],st);
		if (err==-1) {
			printf("Command Not Found\n");
			char tty[100]="";
			strcpy(pidname[pidcnt-1],tty);
			pidstat[pidcnt-1]=0;
			pidcnt--;
		}
		exit(0);
	}	
	else {
		printf("command name: %s process id: %d\n",pidname[pidcnt-1],pidnum[pidcnt-1]);
	}
}

void historyupdate() {
	int i;
	for (i=0;i<=998;i++) 
		strcpy(history[i],history[i+1]);
}

void displayhistory(char in[]) {
	if (strcmp(in,"hist")==0) {
		int i;
		for (i=0;i<hind-1;i++) 
			printf("%d: %s\n",i+1,history[i]);
	}
	else if (in[0]!='!') {
		char temp[100];
		int tempind=0;
		int hlen=strlen(in);
		int i;
		for (i=4;i<hlen;i++) {
			temp[tempind]=in[i];
			tempind++;
		}
		temp[tempind]='\0';
		int ans=atoi(temp);
		int j;
		for (i=(hind-ans-1>0)?hind-ans-1:0,j=1;i<hind-1;i++,j++) {
			if (strcmp(history[i],"")!=0)
				printf("%d: %s\n",j,history[i]);
		}
	}
	else { 
		int i;
		int hlen=strlen(in);
		char temp[100];
		int tempind=0;
		for (i=5;i<hlen;i++) {
			temp[tempind]=in[i];
			tempind++;
		}
		int ans=atoi(temp);
		char tdar[100][100];
		int yu=split(history[ans-1],tdar);
		if (strstr(history[ans-1],"cd")) 
			changedir(history[ans-1],tdar,yu);
		else if (strstr(history[ans-1],"hist"))
			displayhistory(history[ans-1]);
		else if (strstr(history[ans-1],"|"))
			processpipe(history[ans-1]);
		else if (strstr(history[ans-1],">") || strstr(history[ans-1],"<"))
			redirection(history[ans-1]);
		else if (history[ans-1][0]!='\0' && strstr(history[ans-1],"&")) {
			char db[100][100];
			int size=split(history[ans-1],db);
			runcommandback(db,size);
		}
		else if (strstr(history[ans-1],"pid")) {
			char db[100][100];
			int size=split(history[ans-1],db);
			displaypid(db,size);
		}
		else
			runcommand(tdar,yu);
	}
}

void redirection(char in[]) {
	int localcnt=0;
	char *a1=strstr(in,"<<");
	char *a2=strstr(in,">>");
	char *c1=strstr(in,"<");
	char *c2=strstr(in,">");
	if (a1)  localcnt++;
	if (a2)  localcnt++;
	if (c1&&(c1!=a1 && c1!=a1+1))  localcnt++;
	if (c2&&(c2!=a2 && c2!=a2+1))  localcnt++;
	char command[100];
	int cin=0;
	int i=0,len=strlen(in);
	for (i=0;i<len;i++) {
		if (in[i]=='>' || in[i]=='<')
			break;
		command[cin]=in[i];
		cin++;
	}
	command[cin]='\0';
	char *st[100];
	int tempind=0;
	char fcmd[100][100];
	int lengt=split(command,fcmd);
	int j;
	for (j=0,tempind=0;j<lengt;j++,tempind++) {
		st[tempind]=fcmd[j];
	}
	st[tempind]=NULL;
	if (((a1&&!a2) || (!a1&&a2))&& (localcnt==1)) {
		char file[100];
		cin=0;
		for (++i;i<len;i++) {
			if (in[i]!=' '&&in[i]!='>'&& in[i]!='<') {
				file[cin]=in[i];
				cin++;
			}
		}
		file[cin]='\0';
		if (a1) {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0) {
				int inbuf=open(file,O_CREAT|O_APPEND|O_WRONLY,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(inbuf,0);
				close(inbuf);
				int a=1;
				if(strstr(st[0],"hist"))
					displayhistory(st[0]);
				else
					a=execvp(st[0],st);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
		else {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0) {
				int defout=dup(1);
				int outbuf=open(file,O_CREAT|O_APPEND|O_WRONLY,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(outbuf,1);
				close(outbuf);
				int a=1;
				if (strstr(st[0],"hist"))
					displayhistory(st[0]);
				else
					a=execvp(st[0],st);
				dup2(defout,1);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
	}
	else if (((c1&&!c2) || (!c1&&c2))&&(localcnt==1)) {
		char file[100];
		cin=0;
		for (++i;i<len;i++) {
			if (in[i]!=' ') {
				file[cin]=in[i];
				cin++;
			}
		}
		file[cin]='\0';
		if (c1) {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0)  {
				int inbuf=open(file,O_RDONLY,S_IRUSR|S_IWUSR);
				dup2(inbuf,0);
				close(inbuf);
				int a=1;
				if (strstr(st[0],"hist"))
					displayhistory(st[0]);
				else
					a=execvp(st[0],st);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
		else {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0) {
				int defout=dup(1);
				int outbuf=open(file,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(outbuf,1);
				close(outbuf);
				int a=1;
				if (strstr(st[0],"hist"))
					displayhistory(st[0]);
				else
					a=execvp(st[0],st);
				dup2(defout,1);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
	}
	else {
		char file[100];
		int cin=0;
		for (++i;i<len;i++,cin++)
			file[cin]=in[i];
		file[cin]='\0';
		char file1[100];
		char file2[100];
		int size=0;
		char *temp1;
		temp1=strtok(file," 	\t><");
		while (temp1 != NULL) {
			if (size==0)
				strcpy(file1,temp1);
			else
				strcpy(file2,temp1);
			size++;
			temp1=strtok(NULL," 	\t><");
		}
		if (strstr(in,"<")&&strstr(in,">>")) {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0) {
				int inbuf,outbuf;
				int defout=dup(1);
				inbuf=open(file1,O_RDONLY,S_IRUSR|S_IWUSR);
				outbuf=open(file2,O_CREAT|O_APPEND|O_WRONLY,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(inbuf,0);
				dup2(outbuf,1);
				close(inbuf);
				close(outbuf);
				int a=1;
				if (strstr(st[0],"hist"))
					displayhistory(st[0]);
				else
					a=execvp(st[0],st);
				dup2(defout,1);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
		else if (strstr(in,"<")&&strstr(in,">")) {
			int pid=fork();
			strcpy(pidname[pidcnt],st[0]);
			pidnum[pidcnt]=pid;
			pidcnt++;
			if (pid==0) {
				int inbuf,outbuf;
				inbuf=open(file1,O_RDONLY,S_IRUSR|S_IWUSR);
				outbuf=open(file2,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(inbuf,0);
				dup2(outbuf,1);
				close(inbuf);
				close(outbuf);
				int a=execvp(st[0],st);
				if (a==-1) {
					printf("Command Not Found\n");
					char tty[100]="";
					strcpy(pidname[pidcnt-1],tty);
					pidcnt--;
				}
				exit(0);
			}
			else
				wait(NULL);
		}
	}
}

int createprocess(char *temp1, int fd_in, int fd_out) {
	char cmd1[100][100];
	char copy[100];
	strcpy(copy,temp1);
	int size=0;
	char *temp;
	temp=strtok(temp1," \t	");
	while (temp != NULL) {
		strcpy(cmd1[size],temp);
		size++;
		temp=strtok(NULL," \t	");
	}
	strcpy(temp1,copy);
	char *cmd[100];
	int i;
	for (i=0;i<size;i++) {
		cmd[i]=cmd1[i];
	}
	cmd[i]=NULL;
	int pid=fork();
	strcpy(pidname[pidcnt],cmd[0]);
	pidnum[pidcnt]=pid;
	pidcnt++;
	if (pid==0) {
		if (fd_in != -1 && fd_in != 0) {
			dup2(fd_in, 0);
			close(fd_in);
		}
		if (fd_out != -1 && fd_in != 1) {
			dup2(fd_out, 1);
			close(fd_out);
		}
		int a=execvp(cmd[0],cmd);
		if (a==-1) {
			printf("Command Not Found\n");
			char tty[100]="";
			strcpy(pidname[pidcnt-1],tty);
			pidcnt--;
		}
		exit(-1);
	}
	else
		wait(NULL);
	return pid;
}

void processpipe(char in[]) {
	int pids[100];
	int i;
	char command[100][100];
	int cind=0;
	char copy[100];
	strcpy(copy,in);
	char *temp;
	temp=strtok(in,"|");
	while (temp != NULL) {
		strcpy(command[cind],temp);
		cind++;
		temp=strtok(NULL,"|");
	}
	strcpy(in,copy);
	int fdi=-1;
	int fdo;
	for (i=0;i<cind;i++) {
		char cmd2[100][100];
		int ci2=0;
		strcpy(copy,command[i]);
		temp=strtok(command[i]," 	\t");
		while (temp!=NULL) {
			strcpy(cmd2[ci2],temp);
			ci2++;
			temp=strtok(NULL," 	\t");
		}
		strcpy(command[i],copy);
		int pfd[2];
		if (i+1<cind) {
			pipe(pfd); fdo=pfd[1];
		}
		else
			fdo=-1;
		if (strstr(command[i],">")) {
			int ptmp=fork();
			if (ptmp==0) {
				dup2(pfd[0],0);
				redirection(command[i]);
				exit(0);
			}
			else
				wait(NULL);
		}
		else if (strstr(command[i],"<")) {
			int ptmp=fork();
			if (ptmp==0) {
				dup2(pfd[1],1);
				redirection(command[i]);
				exit(0);
			}
			else
				wait(NULL);
		}
		else if (strstr(cmd2[0],"hist") && ci2==1) {
			int ptmp=fork();
			if (ptmp==0) {
				dup2(pfd[1],1);
				displayhistory(cmd2[0]);
				exit(0);
			}
			else
				wait(NULL);
		}
		else
			pids[i]=createprocess(command[i],fdi,fdo);
		close(fdi);
		close(fdo);
		fdi=pfd[0];	//stdin for next command
	}
}

void displaypid(char db[][100],int size) {
	if (size==1 && strcmp(db[0],"pid")==0)
		printf("command name: ./a.out process id: %d\n",mainpid);
	else if (strcmp(db[1],"current")==0) {
		int i;
		for (i=0;i<pidcnt;i++) {
			if (pidstat[i]==1)
				printf("command name: %s process id: %d\n",pidname[i],pidnum[i]);
		}
	}
	else if (strcmp(db[1],"all")==0) {
		int i;
		for (i=0;i<pidcnt;i++) {
			printf("command name: %s process id: %d\n",pidname[i],pidnum[i]);
		}
	}
}

int main() {
	mainpid=getpid();
	signal(SIGINT,sighandle);      //for Ctrl+C
	signal(SIGTSTP,sighandle);      //for Ctrl+Z
	signal(SIGCHLD,sigchild);
	char input[1000]={'\0'};
	char waste;
	getcwd(homedir,sizeof(homedir));
	getcwd(curdir,sizeof(curdir));
	display();
	while (1) {
		scanf("%[^\n]",input);
		if (hind>=1000) {
			historyupdate();
			--hind;
		}
		char db[100][100];
		int size=split(input,db);
		scanf("%c",&waste);
		if (strcmp(db[0],"")!=0)  {
			strcpy(history[hind],input);
			hind++;
			if (strcmp(db[0],"quit")==0)
				break;
			if (strcmp(db[0],"cd")==0) 
				changedir(input,db,size);
			else if (strstr(input,"|"))
				processpipe(input);
			else if (strstr(db[0],"hist")&& !strstr(input,">") && !strstr(input,"|")) 
				displayhistory(db[0]);
			else if (strstr(input,">") || strstr(input,"<"))
				redirection(input);
			else if (input[0]!='\0' && strstr(input,"&")) 
				runcommandback(db,size);
			else if (strstr(input,"pid"))
				displaypid(db,size);
			else if (input[0]!='\0')
				runcommand(db,size);
		}
		memset(db,'\0',sizeof(db[0][0])*100*100);
		memset(input,'\0',sizeof(input[0])*1000);
		display();
	}
	return 0;
}
