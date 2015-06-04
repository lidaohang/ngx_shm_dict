nginx共享内存
==================

根据配置文件来动态的添加共享内存。


## Install

### Linux 

```
git clone https://github.com/lidaohang/ngx_shm_dict
git clone https://github.com/lidaohang/ngx_shm_dict_view
./configure --add-module=ngx_shm_dict --add-module=ngx_shm_dict_view
make && make install
```

### 相关模块

```
git clone https://github.com/lidaohang/ngx_shm_dict_view
```


## Example

```config
	ngx_shm_dict_zone zone=test max_size=2048m;
    ngx_shm_dict_zone zone=test1 max_size=2048m;
    
    ngx_shm_dict_zone zone=test2 max_size=2048m;
    ngx_shm_dict_zone zone=test3 max_size=2048m;


    server {
        listen       8011;
        server_name  localhost;

		location / {
            ah_shm_dict_view;
		}
    }
```

## Test

```
curl "http://127.0.0.1:8011/set?zone=test&key=abc&value=123&exptime=100"
curl "http://127.0.0.1:8011/get?zone=test&key=abc"
curl "http://127.0.0.1:8011/del?zone=test&key=abc"
	
```

