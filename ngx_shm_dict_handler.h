#ifndef NGX_SHM_DICT_HANDLER_H_
#define NGX_SHM_DICT_HANDLER_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stddef.h>


int ngx_shm_dict_handler_get(ngx_shm_zone_t* zone_t,ngx_str_t *key, ngx_str_t *value,uint32_t *exptime);

int ngx_shm_dict_handler_set(ngx_shm_zone_t* zone_t,ngx_str_t *key, ngx_str_t *value,uint32_t exptime);

int ngx_shm_dict_handler_set_exptime(ngx_shm_zone_t* zone_t, ngx_str_t* key, uint32_t exptime);

int ngx_shm_dict_handler_delete(ngx_shm_zone_t* zone_t,ngx_str_t *key);
int ngx_shm_dict_handler_incr_int(ngx_shm_zone_t* zone_t,ngx_str_t *key, int count,uint32_t exptime,int64_t* res);
int ngx_shm_dict_handler_flush_all(ngx_shm_zone_t* zone_t);

int ngx_shm_dict_handler_flush_expired(ngx_shm_zone_t* zone_t,int attempts);
void ngx_str_handler_set_int32(ngx_str_t* key, int32_t* ikey);
void ngx_str_handler_set_int64(ngx_str_t* key, int64_t* ikey);
void ngx_str_handler_set_double(ngx_str_t* key, double* value);
uint32_t ngx_shm_dict_handler_crc32(u_char *p, size_t len);


ngx_shm_zone_t * ngx_http_get_shm_zone(ngx_str_t *shm_name_t);



#endif /* NGX_SHM_DICT_HANDLER_H_ */
