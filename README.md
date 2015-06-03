# ngx_shm_dict
<h1>nginx 共享内存</h1>

nginx 配置文件：


  <h1>  
    ah_shm_dict_zone zone=lands max_size=2048m;
    ah_shm_dict_zone zone=click max_size=2048m;
    
    ah_shm_dict_zone zone=lands1 max_size=2048m;
    ah_shm_dict_zone zone=click1 max_size=2048m;
       

    server {
        listen       8012;
        server_name  localhost;
	location / {
            ah_shm_dict_view;
	}
    }
<h1>

