
#include <sys/debug.h>
#include <sys/types.h>
#ifdef _KERNEL
   #include <sys/systm.h>
#else
   #include <strings.h>
#endif
#include <umem.h>
#include <lzo/lzoconf.h>
#include <lzo/lzo1x.h>
#include <pthread.h>

/* Work-memory needed for compression. Allocate memory in units
*  * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
*   */

#define HEAP_ALLOC(var,size) \
	    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static pthread_mutex_t wrkmem_mtx;
static int alloc=0, used=0;
typedef struct {
	lzo_voidp wrkmem;
	size_t len;
	int used;
} tent;
static tent *ent = NULL;
static int init_done = 0;

void init_lzo()
{
	if (lzo_init() != LZO_E_OK)
	{
		printf("internal error - lzo_init() failed !!!\n");
		printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
		return;
	}
	pthread_mutex_init(&wrkmem_mtx, NULL);
	init_done = 1;
}

static lzo_voidp get_wrkmem(size_t wrk_len)
{
	pthread_mutex_lock(&wrkmem_mtx);
	int n;
	for (n=0; n<used; n++)
		if (!ent[n].used && ent[n].len == wrk_len) {
			ent[n].used = 1;
			pthread_mutex_unlock(&wrkmem_mtx);
			return ent[n].wrkmem;
		}
	if (used == alloc) {
		alloc += 10;
		ent = realloc(ent,sizeof(tent)*alloc);
	}
	lzo_voidp wrkmem = umem_alloc(wrk_len,UMEM_NOFAIL);
	ent[used].wrkmem = wrkmem;
	ent[used].used = 1;
	ent[used].len = wrk_len;
	used++;
	pthread_mutex_unlock(&wrkmem_mtx);
	return wrkmem;
}

static void free_wrkmem(lzo_voidp wrkmem)
{
	int n;
	pthread_mutex_lock(&wrkmem_mtx);
	for (n=0; n<used; n++) {
		if (ent[n].wrkmem == wrkmem) {
			ent[n].used = 0;
			pthread_mutex_unlock(&wrkmem_mtx);
			return;
		}
	}
}

void done_lzo() {
	int n;
	for (n=0; n<used; n++)
		umem_free(ent[n].wrkmem,ent[n].len);
	used = 0;
	free(ent);
	ent = NULL;
	alloc = 0;
	pthread_mutex_destroy(&wrkmem_mtx);
}

static int lz_compress(void *dst, const void *src, size_t srclen,size_t *dstlen, int level) 
{
   int zstat; 

   if (!init_done)
	   return LZO_E_ERROR;
   lzo_voidp wrkmem;
   lzo_uint32 wrk_len = 0;
   if (level == 9)
	   wrk_len = LZO1X_999_MEM_COMPRESS;
   else
	   wrk_len = LZO1X_1_MEM_COMPRESS;
   wrkmem = get_wrkmem(wrk_len);

   lzo_uint cmps = srclen + srclen / 16 + 64 +3;
   lzo_uint cps;
   lzo_bytep cmpb = umem_alloc(cmps, UMEM_NOFAIL);
   /*printf("lzc1:sl=%d,ds=%d\n",srclen,*dstlen); */
   if (level == 9)
	   zstat = lzo1x_999_compress(src,srclen,cmpb,&cps,wrkmem);
   else
	   zstat = lzo1x_1_compress(src,srclen,cmpb,&cps,wrkmem);
   if (cps > *dstlen) {
	   zstat=LZO_E_OUTPUT_OVERRUN;
   } else {
	   memcpy(dst,cmpb,cps);
	   *dstlen=cps;
   }
   umem_free(cmpb, cmps);
   free_wrkmem(wrkmem);
   return zstat; 
} 

int lz_uncompress(void *dst, size_t *dstlen, const void *src, size_t srclen) 
{
   int zstat;

   if (!init_done)
	   return LZO_E_ERROR;

   zstat=lzo1x_decompress((const lzo_bytep)src,(lzo_uint)srclen,(lzo_bytep)dst,(lzo_uintp)dstlen,NULL);
   /* printf("lzu1:sl=%d,ds=%d,zs=%d\n",srclen,*dstlen,zstat); */
   return zstat; 
} 


/*
 */
size_t lzo_compress(void *s_start, void *d_start, size_t s_len, size_t d_len, int level)
{
	ASSERT(d_len <= s_len);
	if (lz_compress(d_start,s_start, s_len,&d_len,level) != LZO_E_OK) {
		if (d_len != s_len)
			return (s_len);

		bcopy(s_start, d_start, s_len);
		return (s_len);
	}
	return (d_len);
}

int lzo_decompress(void *s_start, void *d_start, size_t s_len, size_t d_len, int level)
{
	ASSERT(d_len >= s_len);
	if (lz_uncompress(d_start, &d_len, s_start, s_len) != LZO_E_ERROR)
		return (0);
	return (-1);
}
