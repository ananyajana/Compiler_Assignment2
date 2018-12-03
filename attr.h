/**********************************************
        CS515  Project 2
        Fall  2018
        Student Version
**********************************************/

#ifndef ATTR_H
#define ATTR_H

typedef union {int num; char *str;} tokentype;

typedef struct {
        int targetRegister;
        } regInfo;

typedef struct {
        int label1;
	int label2;
	int label3;
        } labelsInfo;

typedef struct {
        int label1;
	int label2;
	int label3;
	char* str;
        } labelsStrInfo;
typedef struct {
        int label1;
        } labelInfo;
#endif


  
