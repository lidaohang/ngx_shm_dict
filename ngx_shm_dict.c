#include "ngx_shm_dict.h"
#include <assert.h>


ngx_int_t ngx_shm_dict_init_zone(ngx_shm_zone_t *shm_zone, void *data);


void ngx_str_set_int32(ngx_str_t* key, int32_t* value)
{
	key->len= sizeof(int32_t);
	key->data = (u_char*)value;
}


void ngx_str_set_int64(ngx_str_t* key, int64_t* value)
{
	key->len= sizeof(int64_t);
	key->data = (u_char*)value;
}


void ngx_str_set_double(ngx_str_t* key, double* value)
{
	key->len= sizeof(double);
	key->data = (u_char*)value;
}


ngx_inline uint32_t ngx_shm_dict_crc32(u_char *p, size_t len)
{
	if(len == sizeof(ngx_int_t)) {
	
		uint32_t* pi = (uint32_t*)p;
		return *pi;	
	}
	else {
		return ngx_crc32_short(p, len);
	}
}


char *
ngx_strtok_r(char *s, const char *delim, char **last)
{
    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = *last) == NULL) {
	
		return NULL;
    }

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0; ) {
	
		if (c == sc) {
			goto cont;
		}
    }
	 /* no non-delimiter characters */
    if (c == 0) {
		*last = NULL;
		return NULL;
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
	
		c = *s++;
		spanp = (char *)delim;
		
		do {
			if ((sc = *spanp++) == c) {
			
				if (c == 0)	{
					s = NULL;
				}
				else {
					char *w = s - 1;
					*w = '\0';
				}
				*last = s;
				return tok;
			}
		}
		while (sc != 0);
    }
    /* NOTREACHED */
}


ngx_shm_zone_t* 
ngx_shm_dict_init(ngx_conf_t *cf, ngx_str_t* name, size_t size, void* module)
{
	ngx_shm_dict_ctx_t* 			ctx;
	ngx_shm_zone_t                  *zone;
	
    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_shm_dict_ctx_t));
    if (ctx == NULL) {
        return NULL; 
    }

    zone = ngx_shared_memory_add(cf, name, size, module);
    if (zone == NULL) {
		ngx_pfree(cf->pool, ctx);
		ctx = NULL;
        return NULL;
    }

    ctx->name = *name;
    ctx->log = &cf->cycle->new_log;

	
    if (zone->data) {
        ctx = zone->data;

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "[ngx_shm_dict] ngx_shm_dict_init \"%V\" is already defined as "
                           "\"%V\"", name, &ctx->name);
        return NULL;
    }

    zone->init = ngx_shm_dict_init_zone;
    zone->data = ctx;

  	return zone;
}


static void
ngx_shm_dict_rbtree_insert_value(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t          **p;
    ngx_shm_dict_node_t   	   *sdn, *sdnt;

    for ( ;; ) {

        if (node->key < temp->key) {

            p = &temp->left;

        } else if (node->key > temp->key) {

            p = &temp->right;

        } else { /* node->key == temp->key */

            sdn = (ngx_shm_dict_node_t *) &node->color;
            sdnt = (ngx_shm_dict_node_t *) &temp->color;

            p = ngx_memn2cmp(sdn->data, sdnt->data, sdn->key_len,
                             sdnt->key_len) < 0 ? &temp->left : &temp->right;
        }

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


ngx_int_t
ngx_shm_dict_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_shm_dict_ctx_t  *octx = data;
	ngx_shm_dict_ctx_t  *ctx;
    size_t              len;
    

    ctx = shm_zone->data;

    if (octx) {
        ctx->sh = octx->sh;
        ctx->shpool = octx->shpool;

        goto done;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;

    if (shm_zone->shm.exists) {
        ctx->sh = ctx->shpool->data;

        goto done;
    }

    ctx->sh = ngx_slab_alloc(ctx->shpool, sizeof(ngx_shm_dict_shctx_t));
    if (ctx->sh == NULL) {
        return NGX_ERROR;
    }

    ctx->shpool->data = ctx->sh;

    ngx_rbtree_init(&ctx->sh->rbtree, &ctx->sh->sentinel,
                    ngx_shm_dict_rbtree_insert_value);

    ngx_queue_init(&ctx->sh->queue);

    len = sizeof(" in ngx_shared_map zone \"\"") + shm_zone->shm.name.len;

    ctx->shpool->log_ctx = ngx_slab_alloc(ctx->shpool, len);
    if (ctx->shpool->log_ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_sprintf(ctx->shpool->log_ctx, " in ngx_shared_map zone \"%V\"%Z",
                &shm_zone->shm.name);

done:

    return NGX_OK;
}



static ngx_int_t
ngx_shm_dict_lookup(ngx_shm_zone_t *shm_zone, ngx_uint_t hash,
    u_char *kdata, size_t klen, ngx_shm_dict_node_t **sdp)
{
    ngx_int_t                    rc;
    ngx_time_t                   *tp;
    uint64_t                     now;
    int64_t                      ms;
    ngx_rbtree_node_t            *node, *sentinel;
    ngx_shm_dict_ctx_t   		 *ctx;
    ngx_shm_dict_node_t  		 *sd;

    ctx = shm_zone->data;

    node = ctx->sh->rbtree.root;
    sentinel = ctx->sh->rbtree.sentinel;

    while (node != sentinel) {

        if (hash < node->key) {
            node = node->left;
            continue;
        }

        if (hash > node->key) {
            node = node->right;
            continue;
        }

        /* hash == node->key */

        sd = (ngx_shm_dict_node_t *) &node->color;

        rc = ngx_memn2cmp(kdata, sd->data, klen, (size_t) sd->key_len);

        if (rc == 0) {
		
            ngx_queue_remove(&sd->queue);
            ngx_queue_insert_head(&ctx->sh->queue, &sd->queue);

            *sdp = sd;

            if (sd->expires != 0) {
                tp = ngx_timeofday();

                now = (uint64_t) tp->sec * 1000 + tp->msec;
                ms = sd->expires - now;

                if (ms < 0) {
                 
                    return NGX_DONE;
                }
            }

            return NGX_OK;
        }

        node = (rc < 0) ? node->left : node->right;
    }

    *sdp = NULL;

    return NGX_DECLINED;
}


ngx_int_t
ngx_shm_dict_count(ngx_shm_zone_t *shm_zone)
{
    ngx_rbtree_node_t           *node, *sentinel;
    ngx_shm_dict_ctx_t   *ctx;
	int count = 0;

    ctx = shm_zone->data;

    node = ctx->sh->rbtree.root;
    sentinel = ctx->sh->rbtree.sentinel;

    while (node != sentinel) {
	
		count++;
		
		if (node->left != sentinel) {
			
			node = node->left;
			continue;
		}

		if (node->right != sentinel) {
			
			node = node->right;
			continue;
		}
	}
	return count;
}


static int
ngx_shm_dict_expire(ngx_shm_dict_ctx_t *ctx, ngx_uint_t n)
{
    ngx_time_t                  *tp;
    uint64_t                    now;
    ngx_queue_t                 *q;
    int64_t                     ms;
    ngx_rbtree_node_t           *node;
    ngx_shm_dict_node_t  		*sd;
    int                         freed = 0;

    tp = ngx_timeofday();

    now = (uint64_t) tp->sec * 1000 + tp->msec;

    /*
     * n == 1 deletes one or two expired entries
     * n == 0 deletes oldest entry by force
     *        and one or two zero rate entries
     */

    while (n < 3) {

        if (ngx_queue_empty(&ctx->sh->queue)) {
            return freed;
        }

        q = ngx_queue_last(&ctx->sh->queue);

        sd = ngx_queue_data(q, ngx_shm_dict_node_t, queue);

        if (n++ != 0) {

            if (sd->expires == 0) {
                return freed;
            }

            ms = sd->expires - now;
            if (ms > 0) {
                return freed;
            }
        }

        ngx_queue_remove(q);

        node = (ngx_rbtree_node_t *)
                   ((u_char *) sd - offsetof(ngx_rbtree_node_t, color));

        ngx_rbtree_delete(&ctx->sh->rbtree, node);

        ngx_slab_free_locked(ctx->shpool, node);

        freed++;
    }

    return freed;
}


int
ngx_shm_dict_get(ngx_shm_zone_t* zone, ngx_str_t* key,
		ngx_str_t* data, uint8_t* value_type,uint32_t* exptime,
		uint32_t* user_flags)
{
    uint32_t                     hash;
    ngx_int_t                    rc;
    ngx_shm_dict_ctx_t   		 *ctx;
    ngx_shm_dict_node_t  		 *sd;
	
	assert(zone != NULL);
	assert(key != NULL);
	assert(data != NULL);
	
    ctx = zone->data;

	//todo MAX_KEY_LEN
    if (key->len == 0 || key->len > 65535) {
        return SHM_ERROR;
    }

    hash = ngx_shm_dict_crc32(key->data, key->len);

    ngx_shmtx_lock(&ctx->shpool->mutex);

#if NGX_SHM_DICT_EXPIRE
    ngx_shm_dict_expire(ctx, NGX_SHM_DICT_EXPIRE_COUNT);
#endif
 
    rc = ngx_shm_dict_lookup(zone, hash, key->data, key->len, &sd);

    if (rc == NGX_DECLINED || rc == NGX_DONE) {
	
        ngx_shmtx_unlock(&ctx->shpool->mutex);
        return SHM_ERROR;
    }

    /* rc == NGX_OK */

    data->data = sd->data + sd->key_len;
    data->len = (size_t) sd->value_len;

	if(value_type) *value_type = sd->value_type;
    if(user_flags) *user_flags = sd->user_flags;
	if(exptime){
	
		if(sd->expires == 0) {
			*exptime = 0;
		}else {
			ngx_time_t* tp = ngx_timeofday();
			*exptime = (sd->expires-((uint64_t) tp->sec * 1000 + tp->msec))/1000;
		}
	}
	
    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return SHM_OK;
}


int
ngx_shm_dict_delete(ngx_shm_zone_t* zone, ngx_str_t* key)
{
    uint32_t                     hash;
    ngx_int_t                    rc;
    ngx_shm_dict_ctx_t   		 *ctx;
    ngx_shm_dict_node_t  		 *sd;
    ngx_rbtree_node_t            *node;

	assert(zone != NULL);
	assert(key != NULL);

    ctx = zone->data;

    hash = ngx_shm_dict_crc32(key->data, key->len);

    ngx_shmtx_lock(&ctx->shpool->mutex);
	
#if NGX_SHM_DICT_EXPIRE
    ngx_shm_dict_expire(ctx, NGX_SHM_DICT_EXPIRE_COUNT);
#endif

    rc = ngx_shm_dict_lookup(zone, hash, key->data, key->len, &sd);

	if (rc == NGX_DECLINED || rc == NGX_DONE) {
		ngx_shmtx_unlock(&ctx->shpool->mutex);
		//not exists
		return SHM_OK;
	}

	ngx_queue_remove(&sd->queue);

	node = (ngx_rbtree_node_t *)
			   ((u_char *) sd - offsetof(ngx_rbtree_node_t, color));

	ngx_rbtree_delete(&ctx->sh->rbtree, node);
	ngx_slab_free_locked(ctx->shpool, node);
	
	ngx_shmtx_unlock(&ctx->shpool->mutex);
	
	return SHM_OK;
}
 

int
ngx_shm_dict_flush_expired(ngx_shm_zone_t* zone, int attempts)
{
    ngx_queue_t                 *q, *prev;
    ngx_shm_dict_node_t  		*sd;
    ngx_shm_dict_ctx_t   		*ctx;
    ngx_time_t                  *tp;
    int                         freed = 0;
    ngx_rbtree_node_t           *node;
    uint64_t                    now;
	
	assert(zone != NULL);

    ctx = zone->data;

    ngx_shmtx_lock(&ctx->shpool->mutex);

    if (ngx_queue_empty(&ctx->sh->queue)) {
    	ngx_shmtx_unlock(&ctx->shpool->mutex);
        return SHM_OK;
    }

    tp = ngx_timeofday();

    now = (uint64_t) tp->sec * 1000 + tp->msec;

    q = ngx_queue_last(&ctx->sh->queue);

    while (q != ngx_queue_sentinel(&ctx->sh->queue)) {
        prev = ngx_queue_prev(q);

        sd = ngx_queue_data(q, ngx_shm_dict_node_t, queue);

        if (sd->expires != 0 && sd->expires <= now) {
		
            ngx_queue_remove(q);

            node = (ngx_rbtree_node_t *)
                ((u_char *) sd - offsetof(ngx_rbtree_node_t, color));

            ngx_rbtree_delete(&ctx->sh->rbtree, node);
            ngx_slab_free_locked(ctx->shpool, node);
            freed++;

            if (attempts && freed == attempts) {
                break;
            }
        }

        q = prev;
    }

    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return freed;
}

int
ngx_shm_dict_insert(ngx_shm_dict_ctx_t *ctx, ngx_str_t* key, ngx_str_t* value,
			uint8_t value_type, uint32_t exptime, uint32_t user_flags,
			uint32_t  hash) {

	int                          i, size;
	ngx_rbtree_node_t            *node;
	ngx_shm_dict_node_t  		 *sd;
	int						  	 b = 0;
	ngx_time_t                   *tp;
	u_char                       *p;
	
			
	size = offsetof(ngx_rbtree_node_t, color)
        + offsetof(ngx_shm_dict_node_t, data)
        + key->len
        + value->len;

    node = ngx_slab_alloc_locked(ctx->shpool, size);
    if (node == NULL) {

		for (i = 0; i < NGX_SHM_DICT_MAX_EXPIRE_COUNT; i++) {
            if (ngx_shm_dict_expire(ctx, 0) == 0) {
                break;
            }

            node = ngx_slab_alloc_locked(ctx->shpool, size);
            if (node != NULL) {
                b = 1;
                break;
            }
        }

		if(!b) {
			return SHM_ERROR;
		}
    }

	sd = (ngx_shm_dict_node_t *) &node->color;
 
	node->key = hash;
	sd->key_len = key->len;

	if (exptime > 0) {
		tp = ngx_timeofday();
		sd->expires = (uint64_t) tp->sec * 1000 + tp->msec
					  + exptime * 1000;

	} else {
		sd->expires = 0;
	}

	sd->user_flags = user_flags;
	sd->value_len = (uint32_t) value->len;
	sd->value_type = value_type;

	p = ngx_copy(sd->data, key->data, key->len);
	ngx_memcpy(p, value->data, value->len);

	ngx_rbtree_insert(&ctx->sh->rbtree, node);
	ngx_queue_insert_head(&ctx->sh->queue, &sd->queue);
	
    return SHM_OK;
}


int ngx_shm_dict_set_exptime(ngx_shm_zone_t* zone, ngx_str_t* key, uint32_t exptime) {
	
    uint32_t                     hash;
    ngx_int_t                    rc;
    ngx_shm_dict_ctx_t   		 *ctx;
    ngx_shm_dict_node_t  		 *sd;
    ngx_time_t                   *tp;
	
	assert(zone != NULL);
	assert(key != NULL);
	
	ctx = zone->data;
 

    if (key->len == 0 || key->len > 65535) {
        return SHM_ERROR;
    }

    hash = ngx_shm_dict_crc32(key->data, key->len);
	
    ngx_shmtx_lock(&ctx->shpool->mutex);

#if NGX_SHM_DICT_EXPIRE
    ngx_shm_dict_expire(ctx, NGX_SHM_DICT_EXPIRE_COUNT);
#endif

    rc = ngx_shm_dict_lookup(zone, hash, key->data, key->len, &sd);
	
	if (rc == NGX_DECLINED || rc == NGX_DONE) {
		
		ngx_shmtx_unlock(&ctx->shpool->mutex);
		
		return SHM_ERROR;
	}
	
	ngx_queue_remove(&sd->queue);
	ngx_queue_insert_head(&ctx->sh->queue, &sd->queue);

	if (exptime > 0) {
	
		tp = ngx_timeofday();
		sd->expires = (uint64_t) tp->sec * 1000 + tp->msec
					  + exptime * 1000;
	} else {
		sd->expires = 0;
	}

	ngx_shmtx_unlock(&ctx->shpool->mutex);

	return SHM_OK;
}

 
int
ngx_shm_dict_set(ngx_shm_zone_t* zone, ngx_str_t* key, ngx_str_t* value,
			uint8_t value_type, uint32_t exptime, uint32_t user_flags)
{
	
    uint32_t                     hash;
    ngx_int_t                    rc;
    ngx_shm_dict_ctx_t   		 *ctx;
    ngx_shm_dict_node_t  		 *sd;
    u_char                       *p;
    ngx_rbtree_node_t            *node;
    ngx_time_t                   *tp;
	
	assert(zone != NULL);
	assert(key != NULL);
	assert(value != NULL);
	
    ctx = zone->data;
 

    if (key->len == 0 || key->len > 65535) {
        return SHM_ERROR;
    }

    hash = ngx_shm_dict_crc32(key->data, key->len);

    ngx_shmtx_lock(&ctx->shpool->mutex);

#if NGX_SHM_DICT_EXPIRE
    ngx_shm_dict_expire(ctx, NGX_SHM_DICT_EXPIRE_COUNT);
#endif

    rc = ngx_shm_dict_lookup(zone, hash, key->data, key->len, &sd);
	
	//如果不存在，则插入新的
	if (rc == NGX_DECLINED || rc == NGX_DONE) {
		//not exists
		
		//insert
		rc = ngx_shm_dict_insert(ctx, key, value, value_type, exptime, user_flags,
				hash);
		
		ngx_shmtx_unlock(&ctx->shpool->mutex);

		return rc;
	}

//update
	if (value->data && value->len == (size_t) sd->value_len) {
		
		ngx_queue_remove(&sd->queue);
		ngx_queue_insert_head(&ctx->sh->queue, &sd->queue);

		sd->key_len = key->len;

		if (exptime > 0) {
		
			tp = ngx_timeofday();
			sd->expires = (uint64_t) tp->sec * 1000 + tp->msec
						  + exptime * 1000;
		} else {
			sd->expires = 0;
		}
	
		sd->user_flags = user_flags;
		sd->value_len = (uint32_t) value->len;
		sd->value_type = value_type;

		p = ngx_copy(sd->data, key->data, key->len);
		ngx_memcpy(p, value->data, value->len);

		ngx_shmtx_unlock(&ctx->shpool->mutex);

		return SHM_OK;
	}
	
	//remove
	ngx_queue_remove(&sd->queue);

	node = (ngx_rbtree_node_t *)
			   ((u_char *) sd - offsetof(ngx_rbtree_node_t, color));

	ngx_rbtree_delete(&ctx->sh->rbtree, node);
	ngx_slab_free_locked(ctx->shpool, node);	
	
	//insert
	rc = ngx_shm_dict_insert(ctx, key, value, value_type, exptime, user_flags,
			hash);
	
    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return rc;
}


int 
ngx_shm_dict_inc_int(ngx_shm_zone_t* zone, ngx_str_t* key,int64_t i,uint32_t exptime, int64_t* ret)
{
	ngx_int_t rc = 0;
	ngx_str_t data = ngx_null_string;
	uint8_t value_type = SHM_INT64;

	assert(zone != NULL);
	assert(key != NULL);
	assert(ret != NULL);
	
	rc = ngx_shm_dict_get(zone, key, &data, &value_type, NULL,NULL);
	if(rc == SHM_OK) {
	
		if(value_type != SHM_INT64) {
			return SHM_ERROR;
		}
		int64_t* p = (int64_t*)data.data;
		*ret = __sync_add_and_fetch(p, i);
	}
	else {
		//不存在，插入新的
		ngx_str_set_int64(&data, &i);

		rc =ngx_shm_dict_set(zone, key, &data, value_type, exptime,0);
		if(rc == SHM_OK) {
			*ret = i;
		}
	}
	
	return rc;
}


int 
ngx_shm_dict_foreach(ngx_shm_zone_t* zone, foreach_pt func, void* args)
{
    ngx_queue_t                 *q;
    ngx_shm_dict_node_t  		*sd;
    ngx_shm_dict_ctx_t   		*ctx;
	
	assert(zone != NULL);

    ctx = zone->data;

    int locked = ngx_shmtx_trylock(&ctx->shpool->mutex);
	if (!locked){
		return SHM_ERROR;
	}
	
    for (q = ngx_queue_head(&ctx->sh->queue);
         q != ngx_queue_sentinel(&ctx->sh->queue);
         q = ngx_queue_next(q)) 
	{	 
        sd = ngx_queue_data(q, ngx_shm_dict_node_t, queue);
   		func(sd, args);
    }

    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return SHM_OK;
}


int
ngx_shm_dict_flush_all(ngx_shm_zone_t* zone)
{
    ngx_queue_t                 *q;
    ngx_shm_dict_node_t  *sd;
    ngx_shm_dict_ctx_t   *ctx;
	assert(zone != NULL);

    ctx = zone->data;

    ngx_shmtx_lock(&ctx->shpool->mutex);

    for (q = ngx_queue_head(&ctx->sh->queue);
         q != ngx_queue_sentinel(&ctx->sh->queue);
         q = ngx_queue_next(q))
    {
        sd = ngx_queue_data(q, ngx_shm_dict_node_t, queue);
        sd->expires = 1;
    }

    ngx_shm_dict_expire(ctx, 0);

    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return SHM_OK;
}
