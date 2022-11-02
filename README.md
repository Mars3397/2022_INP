# 2022_INP

## Dock


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
