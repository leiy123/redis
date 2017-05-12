#include "redisProxy.h"

char *metaPath = "spread.txt";
char *tracePath = "trace.txt";

redisContext *c = NULL;

//insert <blkNo, nodeIP@memflag> to redis server
void redisServerInit(char *metaPath){
	redisReply *reply;
	const char *hostname = "127.0.0.1";
	const defaultPort = 6379;

	c = redisConnect(hostname, defaultPort);
	if(c == NULL || c->err){
		printf("connection error: %s\n", c->errstr);
		exit(1);
	}
	FILE *fp = fopen(metaPath, "r");
	if(fp == NULL){
		printf("metaPath open fail\n");
		exit(1);
	}
	char val[40] = {'\0'}, key[10] = {'\0'};
	int flag;
	while(!feof(fp) && fscanf(fp, "%s%s ", key, val) != EOF){ //<D11 192.168.0.54,1>
		//字符串处理问题'\0'==NULL(ascii:0)
		reply = redisCommand(c, "set %s %s", key, val);
		//printf("init reply %s\n", reply->str);
		freeReplyObject(reply);
	}
}

//从redis中请求块号
void playTrace(char *tracePath){
	FILE *fp = fopen(tracePath, "r");
	if(fp == NULL){
		printf("trace fail\n");
		exit(-1);
	}
	char rec[10], val[40]={'\0'};
	redisReply *reply;
	while(!feof(fp) && fscanf(fp, "%s", rec) != EOF){
		reply = redisCommand(c, "get %s", rec);
		strcpy(val, reply->str);
		char ip[20], blkNo[10], newVal[40];
		char *delim = strchr(val, ',');
		int delimDex = (delim-val)/sizeof(char);
		strncpy(ip, val, delimDex);
		ip[delimDex] = '\0';
		strncpy(blkNo, val+delimDex+1, 10);
		sprintf(newVal, "%s,%d", ip, atoi(blkNo) + 1); //构造新值，块访问次数增加1
		reply = redisCommand(c, "set %s %s", rec, newVal);
		freeReplyObject(reply);
	}
	fclose(fp);
	reply = redisCommand(c, "flushall");
	freeReplyObject(reply);
}

int main(void){
	redisServerInit(metaPath);
	playTrace(tracePath);
}



