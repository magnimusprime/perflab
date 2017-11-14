#include "defs.h"


void baseline_naive_rotate(int dim, pixel *src, pixel *dst) 
{
	int i, j;

	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

typedef struct {
    int red;
    int green;
    int blue;
    int alpha;
    int num;
} pixel_sum;

static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = sum->alpha = 0;
    sum->num = 0;
    return;
}

static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->alpha += (int) p.alpha;
    sum->num++;
    return;
}

static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned char) (sum.red/sum.num);
    current_pixel->green = (unsigned char) (sum.green/sum.num);
    current_pixel->blue = (unsigned char) (sum.blue/sum.num);
    current_pixel->alpha = (unsigned char) (sum.alpha/sum.num);
    return;
}

static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii=max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj=max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii,jj,dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
 
    return current_pixel;
}

void baseline_naive_smooth(int dim, pixel *src, pixel *dst) 
{
	int i, j;

	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}
