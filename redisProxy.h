/*
 * redisProxy.h
 *
 *  Created on: 2017年5月10日
 *      Author: Administrator
 */

#ifndef REDISPROXY_H_
#define REDISPROXY_H_

#include <hiredis/hiredis.h> //将libhiredis.so放到/usr/local/lib目录下，使用lhireids动态链接
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void redisServerInit(char *metaPath);
void playTrace(char *tracePath);

#endif /* REDISPROXY_H_ */

