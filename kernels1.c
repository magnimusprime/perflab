/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
#include "defs.h"

/* 
 * Please fill in the following team struct 
 */
team_t team = {
    "PerfLab",              /* Team name */

    "Maggie Gates",         /* First member full name */
    "mg5hd@virginia.edu",   /* First member email address */

    "De Ouyang",            /* Second member full name (leave blank if none) */
    "do5xb@virginia.edu"    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

/******************************************************
 * Your different versions of the rotate kernel go here
 ******************************************************/

/* 
 * naive_rotate - The naive baseline version of rotate 
 */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/* 
 * rotate - Another version of rotate
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    int i_1, i_2, i_3, i_4, i_5, i_6, i_7, i_8, i_9, i_10, i_11, i_12, i_13, i_14, i_15; 
    for (i = 0; i < dim; i+=16){
        i_1 = i+1;
        i_2 = i+2;
        i_3 = i+3;
        i_4 = i+4;
        i_5 = i+5;
        i_6 = i+6;
        i_7 = i+7;
        i_8 = i+8;
        i_9 = i+9;
        i_10 = i+10;
        i_11 = i+11;
        i_12 = i+12;
        i_13 = i+13;
        i_14 = i+14;
        i_15 = i+15;
         
    for (j = 0; j < dim; j++){
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
        dst[RIDX(dim-1-j, i_1, dim)] = src[RIDX(i_1, j, dim)];
        dst[RIDX(dim-1-j, i_2, dim)] = src[RIDX(i_2, j, dim)];
        dst[RIDX(dim-1-j, i_3, dim)] = src[RIDX(i_3, j, dim)];
        dst[RIDX(dim-1-j, i_4, dim)] = src[RIDX(i_4, j, dim)];
        dst[RIDX(dim-1-j, i_5, dim)] = src[RIDX(i_5, j, dim)];
        dst[RIDX(dim-1-j, i_6, dim)] = src[RIDX(i_6, j, dim)];
        dst[RIDX(dim-1-j, i_7, dim)] = src[RIDX(i_7, j, dim)];
        dst[RIDX(dim-1-j, i_8, dim)] = src[RIDX(i_8, j, dim)];
        dst[RIDX(dim-1-j, i_9, dim)] = src[RIDX(i_9, j, dim)];
        dst[RIDX(dim-1-j, i_10, dim)] = src[RIDX(i_10, j, dim)];
        dst[RIDX(dim-1-j, i_11, dim)] = src[RIDX(i_11, j, dim)];
        dst[RIDX(dim-1-j, i_12, dim)] = src[RIDX(i_12, j, dim)];
        dst[RIDX(dim-1-j, i_13, dim)] = src[RIDX(i_13, j, dim)];
        dst[RIDX(dim-1-j, i_14, dim)] = src[RIDX(i_14, j, dim)];
        dst[RIDX(dim-1-j, i_15, dim)] = src[RIDX(i_15, j, dim)];
    }
  }
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_rotate_functions() 
{
  /*add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
     ... Register additional test functions here */
}


/***************
 * SMOOTH KERNEL
 **************/

/***************************************************************
 * Various typedefs and helper functions for the smooth function
 * You may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
    int red;
    int green;
    int blue;
    int alpha;
    int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = sum->alpha = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->alpha += (int) p.alpha;
    sum->num++;
    return;
}
static void accumulate_sum_hardcode(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->alpha += (int) p.alpha;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned char) (sum.red/sum.num);
    current_pixel->green = (unsigned char) (sum.green/sum.num);
    current_pixel->blue = (unsigned char) (sum.blue/sum.num);
    current_pixel->alpha = (unsigned char) (sum.alpha/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */

static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
	    accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}
static pixel avgMid(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    //for(ii = i-1; ii <= i+1; ii++) 
    //for(jj = j-1; jj <= j+1; jj++)
    accumulate_sum_hardcode(&sum, src[RIDX(i-1, j-1, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i, j-1, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i+1, j-1, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i-1, j, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i, j, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i+1, j, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i-1, j+1, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i, j+1, dim)]);
    accumulate_sum_hardcode(&sum, src[RIDX(i+1, j+1, dim)]);
    sum.num=9;
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

static pixel avgTopEdge(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = i; ii <= i+1; ii++) 
	for(jj = j-1; jj <= j+1; jj++) 
	    accumulate_sum_hardcode(&sum, src[RIDX(ii, jj, dim)]);
    sum.num=6;
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}
static pixel avgBottomEdge(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = i-1; ii <= i; ii++) 
	for(jj = j-1; jj <= j+1; jj++) 
	    accumulate_sum_hardcode(&sum, src[RIDX(ii, jj, dim)]);
    sum.num=6;
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

static pixel avgLeftEdge(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = i-1; ii <= i+1; ii++) 
	for(jj = j; jj <= j+1; jj++) 
	    accumulate_sum_hardcode(&sum, src[RIDX(ii, jj, dim)]);
    sum.num=6;
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}
static pixel avgRightEdge(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = i-1; ii <= i+1; ii++) 
	for(jj = j-1; jj <= j; jj++) 
	    accumulate_sum_hardcode(&sum, src[RIDX(ii, jj, dim)]);
    sum.num=6;
    assign_sum_to_pixel(&current_pixel, sum);
    return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

/*
 * naive_smooth - The naive baseline version of smooth 
 */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    for (i = 0; i < dim; i++)
	for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

/*
 * smooth - Another version of smooth. 
 */
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
  int i, j,d;
  d=dim-1;
  // top edge  
  for (j=1;j<dim-1;j++)
    dst[RIDX(0, j, dim)] = avgTopEdge(dim, 0, j, src);
  // Bottom Edge
  for (j=1;j<dim-1;j++)
    dst[RIDX(dim-1, j, dim)] = avgBottomEdge(dim, dim-1, j, src);
  // Left edge  
  for (i=1;i<dim-1;i++)
    dst[RIDX(i, 0, dim)] = avgLeftEdge(dim, i, 0, src);
  // Right Edge
  for (i=1;i<dim-1;i++)
    dst[RIDX(i, dim-1, dim)] = avgRightEdge(dim, i, dim-1, src);
 
/*   // top left corner
  pixel sum1;
     sum1.red=(unsigned char)((int)src[RIDX(0, 0, dim)].red+(int)src[RIDX(1, 0, dim)].red+(int)src[RIDX(0, 1, dim)].red+(int)src[RIDX(1, 1, dim)].red)/4;
    sum1.blue=(unsigned char)((int)src[RIDX(0, 0, dim)].blue+(int)src[RIDX(1, 0, dim)].blue+(int)src[RIDX(0, 1, dim)].blue+(int)src[RIDX(1, 1, dim)].blue)/4;
	sum1.green=(unsigned char)((int)src[RIDX(0, 0, dim)].green+(int)src[RIDX(1, 0, dim)].green+(int)src[RIDX(0, 1, dim)].green+(int)src[RIDX(1, 1, dim)].green)/4;
	sum1.alpha=(unsigned char)((int)src[RIDX(0, 0, dim)].alpha+(int)src[RIDX(1, 0, dim)].alpha+(int)src[RIDX(0, 1, dim)].alpha+(int)src[RIDX(1, 1, dim)].alpha)/4;
	dst[0]=sum1; */
/*// top right corner               *
    pixel sum2;
    sum2.red=(unsigned char)((int)src[RIDX(0, d, dim)].red+(int)src[RIDX(1, d, dim)].red+(int)src[RIDX(0, d-1, dim)].red+(int)src[RIDX(1, d-1, dim)].red)/4;
    sum2.blue=(unsigned char)((int)src[RIDX(0, d, dim)].blue+(int)src[RIDX(1, d, dim)].blue+(int)src[RIDX(0, d-1, dim)].blue+(int)src[RIDX(1, d-1, dim)].blue)/4;
	sum2.green=(unsigned char)((int)src[RIDX(0, d, dim)].green+(int)src[RIDX(1, d, dim)].green+(int)src[RIDX(0, d-1, dim)].green+(int)src[RIDX(1, d-1, dim)].green)/4;
	sum2.alpha=(unsigned char)((int)src[RIDX(0, d, dim)].alpha+(int)src[RIDX(1, d, dim)].alpha+(int)src[RIDX(0, d-1, dim)].alpha+(int)src[RIDX(1, d-1, dim)].alpha)/4;
	dst[d]=sum2;*/			      
/*   // bottom left corner
    pixel sum3;
    sum3.red=(unsigned char)((int)src[RIDX(d, 0, dim)].red+(int)src[RIDX(d-1, 0, dim)].red+(int)src[RIDX(d, 1, dim)].red+(int)src[RIDX(d-1, 1, dim)].red)/4;
    sum3.blue=(unsigned char)((int)src[RIDX(d, 0, dim)].blue+(int)src[RIDX(d-1, 0, dim)].blue+(int)src[RIDX(d, 1, dim)].blue+(int)src[RIDX(d-1, 1, dim)].blue)/4;
	sum3.green=(unsigned char)((int)src[RIDX(d, 0, dim)].green+(int)src[RIDX(d-1, 0, dim)].green+(int)src[RIDX(d, 1, dim)].green+(int)src[RIDX(d-1, 1, dim)].green)/4;
    sum3.alpha=(unsigned char)((int)src[RIDX(d, 0, dim)].alpha+(int)src[RIDX(d-1, 0, dim)].alpha+(int)src[RIDX(d, 1, dim)].alpha+(int)src[RIDX(d-1, 1, dim)].alpha)/4;
	dst[RIDX(d,0,dim)]=sum3; */
/*   // bottom right corner 
    pixel sum4;
    sum4.red=(unsigned char)((int)src[RIDX(d, d, dim)].red+(int)src[RIDX(d-1, d, dim)].red+(int)src[RIDX(d, d-1, dim)].red+(int)src[RIDX(d-1, d-1, dim)].red)/4;
    sum4.blue=(unsigned char)((int)src[RIDX(d, d, dim)].blue+(int)src[RIDX(d-1, d, dim)].blue+(int)src[RIDX(d, d-1, dim)].blue+(int)src[RIDX(d-1, d-1, dim)].blue)/4;
    sum4.green=(unsigned char)((int)src[RIDX(d, d, dim)].green+(int)src[RIDX(d-1, d, dim)].green+(int)src[RIDX(d, d-1, dim)].green+(int)src[RIDX(d-1, d-1, dim)].green)/4;
    sum4.alpha=(unsigned char)((int)src[RIDX(d, d, dim)].alpha+(int)src[RIDX(d-1, d, dim)].alpha+(int)src[RIDX(d, d-1, dim)].alpha+(int)src[RIDX(d-1, d-1, dim)].alpha)/4;
    dst[RIDX(d,d,dim)]=sum4;*/			      
// middle
    
  
  for (i=1; i< dim-1;i++)
      for (j=1; j<dim-1;j++)
	    dst[RIDX(i, j, dim)] = avgMid(dim, i, j, src);
    dst[RIDX(d,d,dim)]=avg(dim,d,d,src);
    dst[RIDX(d,0,dim)]=avg(dim,d,0,src);
    dst[RIDX(0,d,dim)]=avg(dim,0,d,src);
    dst[RIDX(0,0,dim)]=avg(dim,0,0,src);
      
}

/********************************************************************* 
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_smooth_functions() {
     add_smooth_function(&smooth, smooth_descr);
      add_smooth_function(&naive_smooth, naive_smooth_descr);
      /* ... Register additional test functions here */

}

