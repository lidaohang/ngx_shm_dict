#ifndef __NGX_HTTP_SHM_DICT_MODULE_H__
#define __NGX_HTTP_SHM_DICT_MODULE_H__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_array_t *shm_dict_list;
} ngx_http_shm_dict_main_conf_t;


#endif
