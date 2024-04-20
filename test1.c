/*
    This file tests message queue creation and deletion w.r.t first fit hole method.
*/

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mf.h"
void test_mq_create_and_remove_1();
void test_mq_create_and_remove_2();
void test_mq_create_and_remove_3();

int main(int argc, char **argv)
{
    //test_mq_create_and_remove_1();
    //test_mq_create_and_remove_2();  
    test_mq_create_and_remove_3();

	return 0;
}


void test_mq_create_and_remove_1()
{
    mf_connect();
    //Test1
    int mq1 = mf_create("mq1", 128);
    int mq2 = mf_create("mq2", 128);
    int mq3 = mf_create("mq3", 128);
    int mq4 = mf_create("mq4", 64);
    int mq5 = mf_create("mq5", 32);

    // If creation was successful, then remove
    if(mq1 == 0) { mf_remove("mq1"); }
    if(mq5 == 0) { mf_remove("mq5"); }

    int mq6 = mf_create("mq6", 32);
    int mq7 = mf_create("mq7", 128);

    // If creation was successful, then remove
    if(mq2 == 0) { mf_remove("mq2"); }
    if(mq3 == 0) { mf_remove("mq3"); }
    if(mq4 == 0) { mf_remove("mq4"); }

    if(mq6 == 0) { mf_remove("mq6"); }
    if(mq7 == 0) { mf_remove("mq7"); }

    mf_disconnect();
}

//First fit
void test_mq_create_and_remove_2()
{
    mf_connect();
    //Test2
    int mq1 = mf_create("mq1", 128);
    int mq2 = mf_create("mq2", 128);
    int mq3 = mf_create("mq3", 128);
    int mq4 = mf_create("mq4", 64);
    int mq5 = mf_create("mq5", 32);
    if(mq4 == 0) {mf_remove("mq4");}
    int mq6 = mf_create("mq6", 32);

    //Insufficient space because 2
    int mq7 = mf_create("mq7", 32);

    if(mq1 == 0) {mf_remove("mq1");}
    if(mq2 == 0) {mf_remove("mq2");}
    if(mq3 == 0) {mf_remove("mq3");}
    if(mq5 == 0) {mf_remove("mq5");}
    if(mq6 == 0) {mf_remove("mq6");}
    if(mq7 == 0) {mf_remove("mq7");}

    mf_disconnect();
}

//First fit
void test_mq_create_and_remove_3()
{
    mf_connect();
    //Test3
    int mq1 = mf_create("mq1", 128);
    int mq2 = mf_create("mq2", 128);
    int mq3 = mf_create("mq3", 128);
    int mq4 = mf_create("mq4", 64);
    int mq5 = mf_create("mq5", 32);
    if(mq4 == 0) {mf_remove("mq4");}
    int mq6 = mf_create("mq6", 128);
    if(mq3 == 0) {mf_remove("mq3");}

    //Created because holes are merged
    int mq7 = mf_create("mq7", 128);

    if(mq1 == 0) {mf_remove("mq1");}
    if(mq2 == 0) {mf_remove("mq2");}
    if(mq5 == 0) {mf_remove("mq5");}
    if(mq6 == 0) {mf_remove("mq6");}
    if(mq7 == 0) {mf_remove("mq7");}

    mf_disconnect();
}
