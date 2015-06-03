# ngx_shm_dict
nginx 共享内存

nginx 配置文件：

user  nobody nobody;
worker_processes 1;


#error_log  logs/error.log;
#error_log  logs/error.log  debug;
error_log   logs/ngx_debug.log  error;

#pid        logs/nginx.pid;

#for debug..
daemon on;
master_process on;
worker_rlimit_core 50000000;
worker_rlimit_nofile 204800;
working_directory /tmp;

events {
	use epoll;
    worker_connections  65535;
}

http {
    default_type  application/octet-stream;
	
	log_format  main  '$time_local'
					'       $request_uri'
					'       $status'
					'       $bytes_sent'
					'       $upstream_cache_status'
					'       $request_time'
					'       $upstream_response_time'
					'       $host'
					'       $remote_addr'
					'       $server_addr'
					'       $upstream_addr'
					'       $http_referer'
					'       $http_user_agent'
					'       $http_X_Forwarded_For'
					'       $http_connection'
					'       $sent_http_connection'
					'       $sent_http_keep_alive';
    
    
    access_log off;
    #access_log  logs/access.log main;

    sendfile        on;
    tcp_nopush     on;
    tcp_nodelay             on;
    server_tokens off;
    reset_timedout_connection on;

    
    client_header_timeout   10;
    client_body_timeout     10;
    send_timeout            15;
    
    keepalive_timeout  65;
    keepalive_requests 10000;

    client_header_buffer_size 32k;
    large_client_header_buffers 4 32k;
    client_max_body_size 12m;   
    client_body_buffer_size 256k;
    
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

}

