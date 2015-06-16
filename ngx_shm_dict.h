
#ifndef __NGX_SHM_DICT_H__
#define __NGX_SHM_DICT_H__

#include <ngx_config.h>
#include <ngx_core.h>
#include <stdint.h>

#define SHM_BINARY 0
#define SHM_INT32 1
#define SHM_INT64 2
#define SHM_DOUBLE 3
#define SHM_STRING 4
#define SHM_NULL 5

#define SHM_OK 0
#define SHM_ERROR -1

#define NGX_SHM_DICT_EXPIRE			1
#define NGX_SHM_DICT_EXPIRE_COUNT	1
#define NGX_SHM_DICT_MAX_EXPIRE_COUNT	30

#pragma pack(push)
#pragma pack(4)

typedef struct {
    u_char                       color;
    u_char                       dummy;
    u_short                      key_len;
    ngx_queue_t                  queue;
    uint64_t                     expires;
    uint32_t                     value_len;
    uint32_t                     user_flags;
    uint8_t                      value_type;
    u_char                       data[1];
} ngx_shm_dict_node_t;
#pragma pack(pop)

typedef struct {
    ngx_rbtree_t                  rbtree;
    ngx_rbtree_node_t             sentinel;
    ngx_queue_t                   queue;
} ngx_shm_dict_shctx_t;


typedef struct {
    ngx_shm_dict_shctx_t  *sh;
    ngx_slab_pool_t              *shpool;
    ngx_str_t                     name;
    ngx_log_t                    *log;
} ngx_shm_dict_ctx_t;

ngx_shm_zone_t* ngx_shm_dict_init(ngx_conf_t *cf, ngx_str_t* name, size_t size, void* module);

int ngx_shm_dict_get(ngx_shm_zone_t* zone, ngx_str_t* key,
		ngx_str_t* data, uint8_t* value_type,uint32_t* exptime,uint32_t* user_flags);


int ngx_shm_dict_set(ngx_shm_zone_t* zone, ngx_str_t* key, ngx_str_t* value,
			uint8_t value_type, uint32_t exptime, uint32_t user_flags);
			
int ngx_shm_dict_set_exptime(ngx_shm_zone_t* zone, ngx_str_t* key,
			uint32_t exptime);
		
int ngx_shm_dict_delete(ngx_shm_zone_t* zone, ngx_str_t* key);

int ngx_shm_dict_inc_int(ngx_shm_zone_t* zone, ngx_str_t* key,int64_t i,uint32_t exptime, int64_t* ret);

typedef void (*foreach_pt)(ngx_shm_dict_node_t* node, void* extarg);

int ngx_shm_dict_foreach(ngx_shm_zone_t* zone, foreach_pt func, void* args);

int ngx_shm_dict_flush_all(ngx_shm_zone_t* zone);

int ngx_shm_dict_flush_expired(ngx_shm_zone_t* zone, int attempts);

void ngx_str_set_int32(ngx_str_t* key, int32_t* ikey);
void ngx_str_set_int64(ngx_str_t* key, int64_t* ikey);
void ngx_str_set_double(ngx_str_t* key, double* value);
uint32_t ngx_shm_dict_crc32(u_char *p, size_t len);
char * ngx_strtok_r(char *s, const char *delim, char **last);

#endif

