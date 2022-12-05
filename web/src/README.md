v0_webserver.c 

  exp1, from teacher's guide(origin)

v1_webserver.c 

  exp2, add fork based v0

v2_webserver.c 

  exp2, add semaphore based v1

v3_webserver.c 

  exp3, test time: 1. total 2. socket read 3. socket write 4. read html 5.log write

v4_webserver.c 

  exp3(pthread), there are some trouble. pthread test time
  
  trouble fixed in 2022/11/11 12:22

v4.2_webserver.c 

  exp3, fix the trouble in v4, good!! 

v5_webserver.c 

  exp3, add mutex lock based on v4

v6_webserver_threadpool.c 

  exp4, add threadpool, pay attention to its compile, "make ptdpl"

v7_webserver.c 

  exp5, business segmentation model, add readmsg threadpool, readfile threadpool, sendmsg threadpool, filename queue, msg queue. "make multiptdpl"

2022/11/10

v8_webserver.c

  bad code, the hash can't work well with write(), "make cache"

v9_webserver.c

  there is file cache, get File By LFU, get File By LRU, "make cache2"

2022/12/5

timeTest.c

  compare my malloc(Nginx) with stdlib malloc. it's makefile in ./myalloc and rename it test.c before compile it.



