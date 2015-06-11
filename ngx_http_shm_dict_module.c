#include "ngx_http_shm_dict_module.h"

static void *ngx_http_shm_dict_create_main_conf(ngx_conf_t *cf);
static char*  ngx_http_ah_shm_zone(ngx_conf_t *cf,ngx_command_t *cmd, void *conf);


static ngx_command_t  ngx_http_shm_dict_commands[] = {

      { ngx_string("ngx_shm_dict_zone"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3,
      ngx_http_ah_shm_zone,
      0,
      0,
      NULL },
    
    ngx_null_command
};


static ngx_http_module_t  ngx_http_shm_dict_module_ctx = {
    NULL,                                   /* preconfiguration */
    NULL,                                   /* postconfiguration */
    ngx_http_shm_dict_create_main_conf,     /* create main configuration */
    NULL,         							/* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};


ngx_module_t  ngx_http_shm_dict_module = {
    NGX_MODULE_V1,
    &ngx_http_shm_dict_module_ctx,          /* module context */
    ngx_http_shm_dict_commands,             /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,          							/* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_shm_dict_create_main_conf(ngx_conf_t *cf) {
    ngx_http_shm_dict_main_conf_t *conf;
    
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_shm_dict_main_conf_t)); 
    if(conf == NULL) {
        return NGX_CONF_ERROR;
    }

    return conf;
}


static char * 
ngx_http_ah_shm_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
	ngx_http_shm_dict_main_conf_t *conf_t = (ngx_http_shm_dict_main_conf_t*)conf;
	ssize_t                    size;
	ngx_str_t                  *value, name, s;
	ngx_uint_t                 i;
	ngx_shm_zone_t 			   **zone_t;
	
	value = cf->args->elts;
	size = 0;
	name.len = 0;
	
	cf->cycle->conf_ctx[ngx_http_shm_dict_module.index] = (void***)conf_t;

	for (i = 1; i < cf->args->nelts; i++) {

		if (ngx_strncmp(value[i].data, "zone=", 5) == 0) {
		
			name.data = value[i].data + 5;
			name.len =  value[i].len - 5;
			continue;
		}

		if (ngx_strncmp(value[i].data, "max_size=", 9) == 0) {
		
			s.data = value[i].data + 9;
			s.len =  value[i].len - 9;
			size = ngx_parse_size(&s);
			
			if (size == NGX_ERROR) {
			
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] invalid zone size \"%V\"", &value[i]);
				return NGX_CONF_ERROR;
			}

			if (size < (ssize_t) (8 * ngx_pagesize)) {
			
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] zone \"%V\" is too small", &value[i]);
				return NGX_CONF_ERROR;
			}
			
			continue;
		}

		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] invalid parameter \"%V\"", &value[i]);
		return NGX_CONF_ERROR;
	}

	if (name.len == 0) {
	
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] \"%V\" must have \"zone\" parameter",
						   &cmd->name);
		return NGX_CONF_ERROR;
	}

	//init shm_dict_list
	if (conf_t->shm_dict_list == NULL) {
	
		conf_t->shm_dict_list = ngx_palloc(cf->pool,sizeof(ngx_array_t));
		if(conf_t->shm_dict_list == NULL) {
			return NGX_CONF_ERROR;
		}

		if(ngx_array_init(conf_t->shm_dict_list,cf->pool, 2,
				sizeof(ngx_shm_zone_t *)) != NGX_OK) {
			return NGX_CONF_ERROR;
		}
	}
	
	//repeat loading
	zone_t = conf_t->shm_dict_list->elts;
	if (zone_t != NULL){
		
		for (i = 0; i < conf_t->shm_dict_list->nelts; i++) {
		
			if ( ngx_strcmp(name.data,zone_t[i]->shm.name.data) == 0 ) {
			
				ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] shm_dict_name repeat loading %s ",name);
				return NGX_CONF_OK;
			}	
		}
		
	}	

	//push shm_dict_list
	ngx_shm_zone_t* zone = ngx_shm_dict_init(cf,&name, size,&ngx_http_shm_dict_module);
	if(zone == NULL){
	
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] ngx_pcalloc ngx_shm_dict_init is error");
		return NGX_CONF_ERROR;
	}

	ngx_shm_zone_t **zp = ngx_array_push(conf_t->shm_dict_list);
	if (zp == NULL) {
		return NGX_CONF_ERROR;
	}
	*zp = zone;

	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"[ah_shm_zone] array shm_dict_list is ok");
	
    return NGX_CONF_OK;
}

