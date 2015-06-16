#include "ngx_shm_dict_handler.h"
#include "ngx_http_shm_dict_module.h"


int
ngx_shm_dict_handler_set(ngx_shm_zone_t* zone_t,ngx_str_t *key, ngx_str_t *value,
		uint32_t exptime) {

	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	ctx = zone_t->data;

	rc = ngx_shm_dict_set(zone_t, key,value,SHM_STRING,exptime,0);

	ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] key=[%V] "
	            "value=[%V] exptime=[%d] rc=[%d]\n",ngx_getpid(),"set",
	            &zone_t->shm.name,key,value,exptime,rc);
	
	return rc;
}

int ngx_shm_dict_handler_set_exptime(ngx_shm_zone_t* zone_t, ngx_str_t* key,
		uint32_t exptime) {

	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	ctx = zone_t->data;

	rc = ngx_shm_dict_set_exptime(zone_t, key,exptime);

	ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] key=[%V] "
	            "exptime=[%d] rc=[%d]\n",ngx_getpid(),"set",
	            &zone_t->shm.name,key,exptime,rc);
	
	return rc;
}


int
ngx_shm_dict_handler_get(ngx_shm_zone_t *zone_t,ngx_str_t *key,ngx_str_t *value,
		uint32_t *exptime) {

	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	uint8_t 				value_type = SHM_STRING;
	ctx = zone_t->data;

	rc = ngx_shm_dict_get(zone_t, key,value,&value_type,exptime,0);
	
	ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] key=[%V] "
	            "value=[%V] exptime=[%d] rc=[%d]\n",ngx_getpid(),"get",
	            &zone_t->shm.name,key,value,*exptime,rc);


	return rc;
}

int
ngx_shm_dict_handler_delete(ngx_shm_zone_t* zone_t, ngx_str_t *key) {
	
	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	ctx = zone_t->data;

	rc = ngx_shm_dict_delete(zone_t,key);
	
	ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] key=[%V] "
	            "rc=[%d]\n",ngx_getpid(),"del",&zone_t->shm.name,key,rc);

	return rc;
}

int
ngx_shm_dict_handler_incr_int(ngx_shm_zone_t* zone_t, ngx_str_t *key, int count,
		uint32_t exptime,int64_t* res) {

	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	ctx = zone_t->data;

	rc = ngx_shm_dict_inc_int(zone_t, key, count,0, res);
	
	 ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] key=[%V] "
	            "count=[%d] exptime=[%d] rc=[%d]\n",ngx_getpid(),"incr",
	            &zone_t->shm.name,key,count,exptime,rc);


	return rc;
}

int
ngx_shm_dict_handler_flush_all(ngx_shm_zone_t* zone_t) {
	
	int 					rc;
	ngx_shm_dict_ctx_t   	*ctx;
	ctx = zone_t->data;

	rc = ngx_shm_dict_flush_all(zone_t);
	
	ngx_log_error(NGX_LOG_DEBUG, ctx->log, 0,
	            "[ah_shm_zone] process=[%d] operator=[%s] zone=[%V] rc=[%d]\n",
	            ngx_getpid(),"flush",&zone_t->shm.name,rc);


	return rc;
}

ngx_shm_zone_t * 
ngx_http_get_shm_zone(ngx_str_t *shm_name_t) {
	
	ngx_shm_zone_t 		**zone;
    size_t 				i;
    zone = g_shm_dict_list->elts;

    if (g_shm_dict_list == NULL || zone == NULL) {
    	return NULL;
    }

	for (i = 0; i < g_shm_dict_list->nelts; i++) {
	
		if( shm_name_t->len == 0 && i == 0) {

			ngx_log_error(NGX_LOG_DEBUG, ((ngx_shm_dict_ctx_t *)(zone[i]->data))->log, 0,
			                    "[ah_shm_zone] process=[%d] get_shm_zone default name is  %V \n",
			                    ngx_getpid(),&zone[i]->shm.name);

			return zone[i];
		}
        
		if ( ngx_strcmp(shm_name_t->data, zone[i]->shm.name.data) == 0) {

			ngx_log_error(NGX_LOG_DEBUG, ((ngx_shm_dict_ctx_t *)(zone[i]->data))->log, 0,
			                    "[ah_shm_zone] process=[%d] get_shm_zone name is  %V \n",
			                    ngx_getpid(),shm_name_t);

            return zone[i];
		}
    }
	
	return NULL;
}

void
ngx_str_handler_set_int32(ngx_str_t* key, int32_t* ikey) {
	ngx_str_set_int32(key,ikey);
}

void 
ngx_str_handler_set_int64(ngx_str_t* key, int64_t* ikey) {
	ngx_str_set_int64(key,ikey);
}

void 
ngx_str_handler_set_double(ngx_str_t* key, double* value) {
	ngx_str_set_double(key,value);
}

uint32_t
ngx_shm_dict_handler_crc32(u_char *p, size_t len) {
	return ngx_shm_dict_crc32(p,len);
}
