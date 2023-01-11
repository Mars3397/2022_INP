# HW3 
[spec](https://docs.google.com/document/d/1A987DybohteGmvUZvjv0LQXmJcZq10339JQve4dB88M/edit)

## Overview

- ㄧ個 **server** 一個 **instance**

## Implementation 

- Create a **VPC** with IPC4 `11.0.0.0/16` ([官方說明](https://docs.aws.amazon.com/directoryservice/latest/admin-guide/gsg_create_vpc.html) 但他長得不太一樣) → 點 Your VPC → create VPC
                
- Create 3 instance → EC2 → instance → Launch an instance → [create 的同時要用 EFS](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/AmazonEFS.html)  (注意 mount point 三個都要一樣)
- 要填的欄位 `Name: server1 or 2 or 3` `Key pair: vockey` `Network settings: Edit 如下`
- `VPC: 選剛剛創的 VPC` `Subnet: 選現在這個 instance 對應的` `Auto-assign public IP: Enable`
`SecurityGroup: 第一次創選 create new 之後就用同一個就好了`  
    
- [Create internet gateway](https://docs.aws.amazon.com/vpc/latest/userguide/VPC_Internet_Gateway.html): 照著說明做就好了 (create route table 的時候要加 0.0.0.0 的 destination 然後選新創的 gateway)
    - 確定有 attach 到 VPC

- SSH 進創出來的 instance ([說明](https://labs.vocareum.com/web/2303448/1178462.0/ASNLIB/public/docs/lang/en-us/README.html#ssh)) → create server in the instance

- `scp -i labsuser.pem server.cpp ec2-user@44.213.90.128:~` → copy server.cpp in [localhost](http://localhost) to home directory of ec2

## EFS: Share storage (Done)

- 另外要 add network    
- 在共同的 folder（你 mount 的點）裡面創一個文字檔放三個 server 目前各自 login 的人數就好了
- [官方文件](https://docs.aws.amazon.com/zh_tw/AWSEC2/latest/UserGuide/AmazonEFS.html)    

## Install g++

`sudo yum install -y gcc-c++`

