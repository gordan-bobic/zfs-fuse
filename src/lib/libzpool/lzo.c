
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

/* Work-memory needed for compression. Allocate memory in units
*  * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
*   */

#define HEAP_ALLOC(var,size) \
	    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static int init_lzo = 0;

static int lz_compress(void *dst, const void *src, size_t srclen,size_t *dstlen, int level) 
{
   int zstat; 

   if (!init_lzo) {
	   if (lzo_init() != LZO_E_OK)
	   {
		   printf("internal error - lzo_init() failed !!!\n");
		   printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
		   return LZO_E_ERROR;
	   }
	   init_lzo = 1;
   }
   lzo_voidp wrkmem;
   lzo_uint32 wrk_len = 0;
   if (level == 9)
	   wrk_len = LZO1X_999_MEM_COMPRESS;
   else
	   wrk_len = LZO1X_1_MEM_COMPRESS;
   wrkmem = (lzo_voidp) umem_alloc(wrk_len, UMEM_NOFAIL);

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
   umem_free(wrkmem,wrk_len);
   return zstat; 
} 

int lz_uncompress(void *dst, size_t *dstlen, const void *src, size_t srclen) 
{
   int zstat;

   if (!init_lzo) {
	   if (lzo_init() != LZO_E_OK)
	   {
		   printf("internal error - lzo_init() failed !!!\n");
		   printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
		   return LZO_E_ERROR;
	   }
	   init_lzo = 1;
   }

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
