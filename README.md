# 2022_INP

## Homework Description

### HW1: 1A2B server
Implement a 1A2B server with several commands using TCP and UDP. 

### HW2: 1A2B server (advanced version)
Implement additional commands base on HW1 1A2B server. 

### HW3: 1A2B server on AWS EC2
Implement a multi-server for 1A2B game using AWS service. 
Implementation detail please refer to [HW3/README.md](https://github.com/Mars3397/2022_INP/tree/main/hw3)

---

## Docker Usage

Check docker version: 
```
$ docker -v
```

Create docker container:
```
$ docker pull jyunkai/nplinux_x86:v1
$ docker run -it -v `pwd`:/home/hw1 jyunkai/nplinux_x86:v1  bin/bash
```

Connect termianl to docker containter:
```
$ docker exec -it "container name" bash   
```
