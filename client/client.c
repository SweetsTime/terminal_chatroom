#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <curses.h>             //ncurses头文件

#define FUN 11
typedef struct sockaddr SA;
char name[30];
int sockfd; //客户端socket
struct sockaddr_in addr;
char IP[15],PORT[10]; //服务器IP、服务端口
char *fun[FUN] = {"查看功能命令:\t\t\\fun","退出聊天室:\t\t\\exit","发送表情:\t\t\\emoji","查看聊天室成员:\t\t\\member",
                "私聊:\t\t\t\\send_to","上传文件:\t\t\\send_file","下载文件:\t\t\\download",
                "查看聊天记录:\t\t\\message","写博客:\t\t\t\\write_blog","读博客:\t\t\t\\read_blog","Snake:\t\t\t\\snake"};
pthread_t threadSend, threadReceive;

char title[100];
char content[5000];
void print_box(char* title, char* content);

/********************Snake**************/
struct Snake{                   //用结构体来表示每个节点
	int hang;
	int lie;
	struct Snake *next;
};

struct Snake food;
struct Snake *head = NULL;             
struct Snake *tail = NULL;
int dir;
int score = 0;
#define UP 1
#define DOWN -1
#define RIGHT 2
#define LEFT -2

/* ***********************************函数************************************* */
//功能部分
void Send_file(void *Socket);//用于发送文件
int Download(void *Socket, char *buf);//用于receive线程下载文件
void Send_To(void *Socket);
void write_blog(void *Socket);
void read_blog(void *Socket);
//基础部分
void* Send(void* Socket); //用于发送信息的进程
void* Receive(void* Socket); //用于接收信息的进程
void init(); //初始化
void start(); //开始服务
int snake(void *Socket);

/* ***********************************主函数************************************* */
int main(){
    //输入服务器ip
    printf("输入服务器IP（默认IP:127.0.0.1）");
    fgets(IP, sizeof(IP), stdin);
    if(IP[0] == '\n'){
        strcpy(IP, "127.0.0.1\n");
    }
    IP[strlen(IP) - 1] = '\0';

    //输入端口
    printf("输入端口号（默认端口:10222号端口）");
    fgets(PORT, sizeof(PORT), stdin);
    if(PORT[0] == '\n'){
        strcpy(PORT, "10222\n");
    }
    PORT[strlen(PORT) - 1] = '\0';

    start();
    pthread_join(threadSend, NULL);
    pthread_join(threadReceive, NULL);
    close(sockfd);
    return 0;
}

/* *********************************功能部分函数具体实现************************************* */



/* *********************************基础部分函数具体实现************************************* */
void* Receive(void* Socket)
{
    int *SocketCopy = Socket;
    char Receiver[1024];

    while(1){
        memset(Receiver, 0, sizeof(Receiver)); //初始化接收缓冲区

        int receiverEnd = 0;
        receiverEnd  = recv (*SocketCopy, Receiver, sizeof(Receiver), 0);
        if (receiverEnd < 0) {
            perror("接收出错");
            exit(-1);
        } else if (receiverEnd == 0) {
            perror("服务器已关闭，断开连接");
            exit(-1);
        } else if (Receiver[0] == 'D' && Receiver[1] == 'L'){//下载标识符
            Download(SocketCopy, Receiver);
        }
        else {
            Receiver[receiverEnd] = '\0';
            fputs(Receiver, stdout);
        }
    }
}

void* Send(void* Socket)
{
    char Sender[100];
    int *SocketCopy = Socket;
    while(1)
    {
        memset(Sender, 0, sizeof(Sender)); //初始化发送缓冲区
        fgets(Sender, sizeof(Sender), stdin); //读取需要发送的数据

        if (send(*SocketCopy, Sender, sizeof(Sender), 0) < 0){ //防止发送失败
            perror("发送出错！再试一次！");
            continue;
        }
        else {
            if(strcmp(Sender, "\\exit\n") == 0) //检测到退出聊天室命令
                exit(0);
        }
        if (strcmp(Sender, "\\send_file\n") == 0){
            Send_file(SocketCopy);//调用发送文件的函数
        } else if(strcmp(Sender, "\\send_to\n") == 0){
            Send_To(SocketCopy);
        } else if(strcmp(Sender, "\\write_blog\n") == 0){
            write_blog(SocketCopy);
        } else if(strcmp(Sender, "\\read_blog\n") == 0){
            read_blog(SocketCopy);
        } else if(strcmp(Sender, "\\snake\n") == 0){
            snake(SocketCopy);
        }

    }
}

void read_blog(void *Socket){
    char Sender[5],title[50],like[10];
    int *SocketCopy = Socket;
    memset(Sender, 0, sizeof(Sender));
    memset(title, 0, sizeof(title));
    memset(like, 0, sizeof(like));
    fgets(Sender, sizeof(Sender), stdin); //读取需要发送的数据
    send(*SocketCopy, Sender, strlen(Sender), 0);

    fgets(title, sizeof(title), stdin);
    send(*SocketCopy, title, strlen(title), 0);

    fgets(like, sizeof(like), stdin);
    send(*SocketCopy, like, strlen(like), 0);

}

void write_blog(void * Socket){
    char title[50], context[200], choice[5];
    int *SocketCopy = Socket;
    int fd = *SocketCopy;
    int flag = 1;
    memset(title, 0, sizeof(title));
    memset(choice, 0, sizeof(choice));
    fgets(title, sizeof(title), stdin); //读取需要发送的数据
    send(fd, title, strlen(title), 0);
    fgets(choice, sizeof(choice), stdin); //读取需要发送的数据
    send(fd, choice, strlen(choice), 0);
    while(flag){
        memset(context, 0, sizeof(context));
        fgets(context, sizeof(context), stdin);
        send(fd, context, strlen(context), 0);
        if (strcmp(context, "\\finish\n") == 0) flag = 0;
    }

}

void Send_To(void *Socket){
    char Sender[5];
    int *SocketCopy = Socket;

    memset(Sender, 0, sizeof(Sender));
    fgets(Sender, sizeof(Sender), stdin); //读取需要发送的数据
    send(*SocketCopy, Sender, strlen(Sender), 0);


}


void Send_file(void *Socket){
    char file[100] = {0};
    char buf[100] = {0};
    int *SocketCopy = Socket;
    int read_len;
    int send_len;
    int file_len = 0;//记录文件长度
    FILE *fp;

    fgets(file, sizeof(file), stdin); 
    file[strlen(file) - 1] = '\0';
    if ((fp = fopen(file,"r")) == NULL) {//打开文件
        perror("打开文件失败\n");
        send(*SocketCopy, "FAIL", strlen("FAIL"), 0);
        return;
    }

    send(*SocketCopy, file, 100, 0);//发送文件名
    bzero(buf, 100);

    while ((read_len = fread(buf, sizeof(char), 100, fp)) > 0 ) {
        send_len = send(*SocketCopy, buf, read_len, 0);
        if ( send_len < 0 ) {
            perror("发送文件失败\n");
            return;
        }
        bzero(buf, 100);
    }
    fclose(fp);
    send(*SocketCopy, "OK!", strlen("OK!"), 0);//传输完成的标志
    printf("文件%s已发送完成\n",file);
}

//改动点
int Download(void *Socket, char Receiver[100]){
    char file[100] = {0};//记录文件名
    int *SocketCopy = Socket;
    int write_len;
    int recv_len;
    FILE *fp;

    //改动点
    send(*SocketCopy, "OK", strlen("OK"), 0);//不send一个信息过去，DL和文件内容会阻塞到一起，无法进入下面的while循环

    recv(*SocketCopy, file, 100, 0);

    if ((fp = fopen(file,"w")) == NULL) {//打开文件
        perror("文件创建失败\n");
        return 0;
    }
    while (recv_len = recv(*SocketCopy, Receiver, 100, 0)) {
        if(recv_len < 0) {
            printf("文件下载失败\n");
            break;
        }
        //debug
        printf("#");
        //改动点
        int i = 0;
        for(i = 0; i < recv_len; i++)   //暴力检查传输完成标志
            if(strcmp(&Receiver[i], "OK!") == 0)
                break;

        write_len = fwrite(Receiver, sizeof(char), (i < recv_len)? i : recv_len, fp);
        if(i < recv_len){
            printf("\n文件%s下载完成\n", file);
            fclose(fp);
            return 1;
        }
    

        if (write_len < recv_len) {
            printf("文件拷贝失败\n");
            return 0;
        }
        bzero(Receiver, 100);
    }
    return 1;
}


void init(){

    //步骤1：创建socket并填入地址信息
    sockfd = socket(PF_INET,SOCK_STREAM,0);
    if (sockfd < 0) {
        perror("套接字创建失败！");
        exit(-1);
    }
    addr.sin_family = PF_INET;
    addr.sin_port = htons(atoi(PORT));
    addr.sin_addr.s_addr = inet_addr(IP);

    //步骤2：连接服务器
    if (connect(sockfd,(SA*)&addr,sizeof(addr)) < 0){
        perror("无法连接到服务器!");
        exit(-1);
    }
}


void start(){
    char nameInfo[100]; //用来接收输入名字后服务器的反馈信息
    int flag = 1;
    int i;
    printf("我们提供的功能有：\n");
    sprintf(title, "欢迎来到聊天室");
    strcat(content, "查看命令:      【 \\fun        】\n");
    strcat(content, "发送表情:      【 \\emoji      】\n");
    strcat(content, "聊天成员:      【 \\member     】\n");
    strcat(content, "发起私聊:      【 \\send_to    】\n");
    strcat(content, "上传文件:      【 \\send_file  】\n");
    strcat(content, "下载文件:      【 \\download   】\n");
    strcat(content, "聊天记录:      【 \\message    】\n");
    strcat(content, "博客写作:      【 \\write_blog 】\n");
    strcat(content, "阅览博客:      【 \\read_blog  】\n");
    strcat(content, "贪食之蛇:      【 \\snake      】\n");
    strcat(content, "退出聊天:      【 \\exit       】\n");
    print_box(title, content);
    //for (i = 0; i < FUN; i++) puts(fun[i]);//输出功能列表
    printf("\n");
    //步骤1：输入客户端的名字
    memset(nameInfo, 0, sizeof(nameInfo));
    //输入名字并发送给服务器
    while(flag){
        printf("请输入您的名字：");
        fgets(name, sizeof(name), stdin);
        name[strlen(name) - 1] = '\0'; //fgets函数比gets函数安全，同样支持空格，但会把换行符读进去。

        //步骤2：建立连接并发送
        init();
        send(sockfd, name, strlen(name), 0);

        //步骤3：接收服务器的反馈信息到rec
        recv(sockfd, nameInfo, sizeof(nameInfo), 0);
        nameInfo[strlen(nameInfo)] = '\0';

        //判断命名是否成功，若得到refuse，则name清零，继续循环操作输入名字，直到
        if (strcmp(nameInfo, "change") == 0) {
            printf("该名字已存在，尝试换一个吧！\n");
            memset(name, 0 , sizeof(name));
        } else {
            flag = 0;
            fputs(nameInfo, stdout);
            //步骤4：启动收发函数线程
            pthread_create(&threadSend, 0, Send, &sockfd); //专门用于发送信息的线程
            pthread_create(&threadReceive, 0, Receive, &sockfd); //专门用于接收信息的线程
        }
    }
}

int get_display_length(char* str) {
    int length = 0;
    while (*str) {
        if (*str == '\t') {  // 制表符
            length += 4;  // 假设一个制表符等于4个空格
            str += 1;
        }
        else if ((*str & 0x80) == 0) {  // 单字节UTF-8字符
            length += 1;
            str += 1;
        }
        else if ((*str & 0xE0) == 0xC0) {  // 双字节UTF-8字符
            length += 2;
            str += 2;
        }
        else if ((*str & 0xF0) == 0xE0) {  // 三字节UTF-8字符
            length += 2;
            str += 3;
        }
    }
    return length;
}


//然后，使用 print_with_padding 和 print_with_padding2 函数来打印内容和标题。
// 这两个函数会根据文本框的宽度和当前行的长度，计算出需要添加的空格数量，以确保文本在文本框中居中显示。
void print_with_padding(char* str, int padding) {
    int length = get_display_length(str);
    printf("* %s", str);
    for (int i = 0; i < padding - length + 3; i++) {  //可修改这个+3改变内容右侧*的缩进
        printf(" ");
    }
    printf("*\n");
}

void print_with_padding2(char* str, int padding) {
    int length = get_display_length(str);
    printf("* ");
    for (int i = 0; i < (padding - length) / 2 + 1; i++) {
        printf(" ");
    }
    printf("%s", str);
    for (int i = 0; i < (padding - length + 1) / 2 + 2; i++) {  //可修改这个+2改变标题右侧*的缩进
        printf(" ");
    }
    printf("*\n");
}

//在 print_box 函数中，首先计算标题的长度，然后遍历内容中的每一行，找出最长的一行。这样就可以得到文本框的宽度。
//最后，print_box 函数会打印出上边框、标题、分隔线、内容和下边框，从而形成一个完整的文本框。
// 这个文本框的宽度会自动适应标题和内容中最长的一行，实现了自适应的效果。
void print_box(char* title, char* content) {
    int max_line_length = get_display_length(title);
    char* content_copy = strdup(content);  // 创建内容的副本
    char* line = strtok(content_copy, "\n");

    // 计算最长行的长度
    while (line) {
        int line_length = get_display_length(line);
        if (line_length > max_line_length) {
            max_line_length = line_length;
        }
        line = strtok(NULL, "\n");
    }

    free(content_copy);  // 释放副本的内存
    content_copy = strdup(content);  // 创建一个新的副本
    line = strtok(content_copy, "\n");

    // 打印上边框
    for (int i = 0; i < max_line_length + 6; i++) {
        printf("*");
    }
    printf("\n");

    // 打印标题
    print_with_padding2(title, max_line_length);  // 使用print_with_padding函数打印标题

    // 打印分隔线
    for (int i = 0; i < max_line_length + 6; i++) {
        printf("=");
    }
    printf("\n");

    // 打印内容
    while (line) {
        print_with_padding(line, max_line_length);
        line = strtok(NULL, "\n");
    }

    // 打印下边框
    for (int i = 0; i < max_line_length + 6; i++) {
        printf("*");
    }
    printf("\n");

    free(content_copy);  // 释放副本的内存
}

void initNcurses(){             //初始化ncurses界面
	initscr();
	noecho();
	keypad(stdscr,1);       //键盘输入标准
}

void initFood(){

	int x = rand()%30;
	int y = rand()%40;
	if(x == 0){
		x = x+1;
	}
	if(x == 30){
		x = x-1;
	}
	if(y == 0){
		y = y+1;
	}
	if(y == 40){
		y = y-1;
	}
	food.hang = x;
	food.lie = y;
}

int hasFood(int i,int j){
	if(food.hang == i && food.lie == j){
		return 1;
	}
	return 0;
}

int snakeNode(int i,int j){     //判断遍历到的行列是否为节点行列
	struct Snake *p;
	p = head;
	while(p != NULL){
		if(p->hang ==i&&p->lie == j){
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void addNode(){                  //尾部加节点
	struct Snake *new = (struct Snake *)malloc(sizeof(struct Snake));
	switch(dir){
		case UP:
			new->hang = tail->hang-1;
			new->lie = tail->lie;
			break;
		case DOWN:
			new->hang = tail->hang+1;
			new->lie = tail->lie;
			break;
		case LEFT:
			new->hang = tail->hang;
			new->lie = tail->lie-1;
			break;
		case RIGHT:
			new->hang = tail->hang;
			new->lie = tail->lie+1;
			break;
	}
	new->next = NULL;
	tail->next = new;
	tail = new;
}

void initSnake(){                //初始化头节点
	struct Snake *p;
	dir = RIGHT;
	while(head != NULL){
		p = head;
		head = head->next;
		free(p);
	}
	initFood();
	head = (struct Snake *)malloc(sizeof(struct Snake));
	head->hang = 1;
	head->lie = 1;
	head->next = NULL;
	tail = head;

	addNode();
	addNode();
}

void gamePic(){                  //显示界面
	int hang;
	int lie;
	move(0,0);               //光标坐标

	for(hang = 0;hang<=30;hang++){
		if(hang ==0){
			for(lie = 0;lie<40;lie++){
				printw("--");
			}
			printw("\n");
		}
		else if(hang >= 0 && hang < 30){
			for(lie = 0;lie <= 40;lie++){
				if(lie == 0 || lie == 40){
					printw("|");
				}
				else if(snakeNode(hang,lie)){
					printw("[]");
				}
				else if(hasFood(hang,lie)){     
					printw("##");
				}
				else{
					printw("  ");
				}
			}
			printw("\n");
		}
		else{
			for(lie = 0;lie<40;lie++){
				printw("--");
			}
		}
	}
	printw("\nSNAKE GAME");
	printw("\nSCORE = %d\n",score);
}
void deleNode(){                                                       //从头部删除节点
	struct Snake *p;
	p = head;
	head = head->next;
	free(p);
}

int ifSnakeDie(){
	struct Snake *p;
	p = head;
	if(tail->hang == 0 || tail->lie == 0 || tail->hang == 30 || tail->lie == 40){
		return 1;
	}
	while(p->next != NULL){
		if(p->hang == tail->hang && p->lie == tail->lie){
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void moveSnake(){                                                      //蛇向右移动
        if(tail->hang == food.hang && tail->lie == food.lie){
		score++;
		initFood();
	}else{
		deleNode();                                                    //头部删除节点
	}
	addNode();                                                     //尾部增加节点
	if(ifSnakeDie()){
		initSnake();
		score = 0;
	}
	gamePic();                                                     //刷新显示界面
}

void *refreshPic(){                                                    //向右自动移动线程
	while(1){
			moveSnake();
			refresh();
			usleep(100000);		                       //移动速度
	}
}

void turn(int direction){
	if(abs(dir) != abs(direction)){
		dir = direction;
	}
}

void *getDir(){                                                        //不断获取键盘输入线程
	int key;
	while(1){
		key = getch();
		switch(key){
			case KEY_DOWN:
				turn(DOWN);
				break;
			case KEY_UP:
				turn(UP);
				break;
			case KEY_LEFT:
				turn(LEFT);
				break;
			case KEY_RIGHT:
				turn(RIGHT);
				break;
		}
	}
}

int snake(void *Socket){
    initNcurses();
	initSnake();
	gamePic();

	pthread_t t1;                                     //线程的定义
	pthread_t t2;
	pthread_create(&t1,NULL,refreshPic,NULL);         //线程的创建
	pthread_create(&t2,NULL,getDir,NULL);

	while(1){}
	return 0;
};