nginx共享内存
==================

根据配置文件来动态的添加共享内存。

```

ngx_shm_dict
 共享内存核心模块(红黑树，队列)

ngx_shm_dict_manager
 添加定时器事件，定时的清除共享内存中过期的key
 添加读事件，支持redis协议，通过redis-cli get,set,del,ttl

ngx_shm_dict_view
 共享内存查看
 
```

## Install

### Linux 

```
git clone https://github.com/lidaohang/ngx_shm_dict
git clone https://github.com/lidaohang/ngx_shm_manager
git clone https://github.com/lidaohang/ngx_shm_dict_view

patch -p1 < ngx_shm_manager/nginx-1.4.1-1.58-proc-module.patch

./configure --add-module=ngx_shm_dict --add-module=ngx_shm_dict_view --add-module=ngx_shm_dict_manager
make && make install
```

### 相关模块

```
git clone https://github.com/lidaohang/ngx_shm_dict   			核心模块
git clone https://github.com/lidaohang/ngx_shm_manager			管理以及主动过期策略,支持redis协议
git clone https://github.com/lidaohang/ngx_shm_dict_view		查看共享内存
```


## Example

```config

processes {

	process shm_dict_manager {
	    ah_shm_dict_name test;
	    interval 3s;
	
	    delay_start 300ms;
	    listen 8010;
	}
}


    ngx_shm_dict_zone zone=test max_size=2048m;
    ngx_shm_dict_zone zone=test1 max_size=2048m;
    
    ngx_shm_dict_zone zone=test2 max_size=2048m;
    ngx_shm_dict_zone zone=test3 max_size=2048m;


    server {
        listen       8011;
        server_name  localhost;

		location / {
            ngx_shm_dict_view;
		}
    }
```

## Test

```
curl "http://127.0.0.1:8011/set?zone=test&key=abc&value=123&exptime=100"
curl "http://127.0.0.1:8011/get?zone=test&key=abc"
curl "http://127.0.0.1:8011/del?zone=test&key=abc"


redis-cli get abc
redis-cli set abc 123
redis-cli del abc
redis-cli ttl abc
	
```

