/*******************************************************************
 * 
 * driver.c - Driver program for CS:APP Performance Lab
 * 
 * In kernels.c, students generate an arbitrary number of rotate and
 * smooth test functions, which they then register with the driver
 * program using the add_rotate_function() and add_smooth_function()
 * functions.
 * 
 * The driver program runs and measures the registered test functions
 * and reports their performance.
 * 
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights
 * reserved.  May not be used, modified, or copied without permission.
 *
 ********************************************************************/

#define _GNU_SOURCE
#include <ctype.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include "fcyc.h"
#include "defs.h"
#include "config.h"
#include "baselines.h"

#ifndef GRADER
#define STUDENT
#endif

#ifndef STUDENT
// FIXME: isolate from stdout
static FILE *bench_out;
#endif

/* Team structure that identifies the students */
extern team_t team; 

/* Keep track of a number of different test functions */
#define MAX_BENCHMARKS 100
#define DIM_CNT 6

#define BASELINE_FACTOR 1.01

/* Misc constants */
#define BSIZE 32     /* cache block size in bytes */     
#define MAX_DIM (3900) 
#define ODD_DIM 96   /* not a power of 2 */

/* fast versions of min and max */
#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

/* This struct characterizes the results for one benchmark test */
typedef struct {
	lab_test_func tfunct; /* The test function */
	double cpes[DIM_CNT]; /* One CPE result for each dimension */
	char *description;    /* ASCII description of the test function */
	unsigned short valid; /* The function is tested if this is non zero */
} bench_t;

/* The range of image dimensions that we will be testing */
//static int test_dim_rotate[] = {1536, 1568, 1792, 2016, 2048, 2080};
// static int test_dim_rotate[] = {1536, 1568, 1792, 2016, 2048, 2080};
static int test_dim_rotate[] = {1536, 1792, 2048, 2080, 2528, 3104};
static int test_dim_smooth[] = {32, 64, 128, 256, 576, 800};

/* Baseline CPEs (see config.h) */
static double rotate_baseline_cpes[] = {R1536, R1568, R1792, R2016, R2048, R2080};
static double smooth_baseline_cpes[] = {S32, S64, S128, S256, S576, S800};

/* These hold the results for all benchmarks */
static bench_t benchmarks_rotate[MAX_BENCHMARKS];
static bench_t benchmarks_smooth[MAX_BENCHMARKS];

/* These give the sizes of the above lists */
static int rotate_benchmark_count = 0;
static int smooth_benchmark_count = 0;

/* 
 * An image is a dimxdim matrix of pixels stored in a 1D array.  The
 * data array holds three images (the input original, a copy of the original, 
 * and the output result array. There is also an additional BSIZE bytes
 * of padding for alignment to cache block boundaries.
 */
static pixel data[(3*MAX_DIM*MAX_DIM) + (BSIZE/sizeof(pixel))];

/* Various image pointers */
static pixel *orig = NULL;         /* original image */
static pixel *copy_of_orig = NULL; /* copy of original for checking result */
static pixel *result = NULL;       /* result image */

/* Keep track of the best rotate and smooth score for grading */
double rotate_maxmean = 0.0;
double rotate_minmean = 0.0;
char *rotate_maxmean_desc = NULL;

double smooth_maxmean = 0.0;
double smooth_minmean = 0.0;
char *smooth_maxmean_desc = NULL;


/******************** Functions begin *************************/

static char *clean_description(char *description) {
    char *result = malloc(strlen(description) + 1);
    strcpy(result, description);
    char *p = result;
    while (*p) {
        if (!isalnum(*p) && *p != '_' && *p != '-' && *p != ':') {
            *p = ' ';
        }
        ++p;
    }
    return result;
}

void add_smooth_function(lab_test_func f, char *description) 
{
	benchmarks_smooth[smooth_benchmark_count].tfunct = f;
	benchmarks_smooth[smooth_benchmark_count].description = clean_description(description);
	benchmarks_smooth[smooth_benchmark_count].valid = 0;  
	smooth_benchmark_count++;
}


void add_rotate_function(lab_test_func f, char *description) 
{
	benchmarks_rotate[rotate_benchmark_count].tfunct = f;
	benchmarks_rotate[rotate_benchmark_count].description = clean_description(description);
	benchmarks_rotate[rotate_benchmark_count].valid = 0;
	rotate_benchmark_count++;
}

/* 
 * random_in_interval - Returns random integer in interval [low, high) 
 */
static int random_in_interval(int low, int high) 
{
	int size = high - low;
	return (rand()% size) + low;
}

/*
 * create - creates a dimxdim image aligned to a BSIZE byte boundary
 */
static void create(int dim)
{
	int i, j;

	/* Align the images to BSIZE byte boundaries */
	orig = data;
	while ((unsigned long)orig % BSIZE)
		orig = (pixel*)(((char *)orig)+1);
	result = orig + dim*dim;
	copy_of_orig = result + dim*dim;

	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			/* Original image initialized to random colors */
			orig[RIDX(i,j,dim)].red = random_in_interval(0, 65536);
			orig[RIDX(i,j,dim)].green = random_in_interval(0, 65536);
			orig[RIDX(i,j,dim)].blue = random_in_interval(0, 65536);
			orig[RIDX(i,j,dim)].alpha = random_in_interval(0, 65536);

			/* Copy of original image for checking result */
			copy_of_orig[RIDX(i,j,dim)].red = orig[RIDX(i,j,dim)].red;
			copy_of_orig[RIDX(i,j,dim)].green = orig[RIDX(i,j,dim)].green;
			copy_of_orig[RIDX(i,j,dim)].blue = orig[RIDX(i,j,dim)].blue;
			copy_of_orig[RIDX(i,j,dim)].alpha = orig[RIDX(i,j,dim)].alpha;

			/* Result image initialized to all black */
			result[RIDX(i,j,dim)].red = 0;
			result[RIDX(i,j,dim)].green = 0;
			result[RIDX(i,j,dim)].blue = 0;
			result[RIDX(i,j,dim)].alpha = 0;
		}
	}

	return;
}


/* 
 * compare_pixels - Returns 1 if the two arguments don't have same RGB
 *    values, 0 o.w.  
 */
static int compare_pixels(pixel p1, pixel p2) 
{
	return 
		(p1.red != p2.red) || 
		(p1.green != p2.green) || 
		(p1.blue != p2.blue) ||
                (p1.alpha != p2.alpha);
}


/* Make sure the orig array is unchanged */
static int check_orig(int dim) 
{
	int i, j;

	for (i = 0; i < dim; i++) 
		for (j = 0; j < dim; j++) 
			if (compare_pixels(orig[RIDX(i,j,dim)], copy_of_orig[RIDX(i,j,dim)])) {
//				printf("\n");
				// printf("Error: Original image has been changed!\n");
				return 1;
			}

	return 0;
}

/* 
 * check_rotate - Make sure the rotate actually works. 
 * The orig array should not  have been tampered with! 
 */
static int check_rotate(int dim) 
{
	int err = 0;
	int i, j;
#ifdef STUDENT
	int badi = 0;
	int badj = 0;
	pixel orig_bad, res_bad;
#endif

	/* return 1 if the original image has been  changed */
	if (check_orig(dim)) 
		return 1; 

	for (i = 0; i < dim; i++) 
		for (j = 0; j < dim; j++) 
			if (compare_pixels(orig[RIDX(i,j,dim)], 
							   result[RIDX(dim-1-j,i,dim)])) {
				err++;
#ifdef STUDENT
				badi = i;
				badj = j;
				orig_bad = orig[RIDX(i,j,dim)];
				res_bad = result[RIDX(dim-1-j,i,dim)];
#endif
			}

	if (err) {
#ifdef STUDENT
		printf("\n");
		printf("ERROR: Dimension=%d, %d errors\n", dim, err);    
		printf("E.g., The following two pixels should have equal value:\n");
		printf("src[%d][%d].{red,green,blue,alpha} = {%d,%d,%d,%d}\n",
			   badi, badj, orig_bad.red, orig_bad.green, orig_bad.blue, orig_bad.alpha);
		printf("dst[%d][%d].{red,green,blue} = {%d,%d,%d,%d}\n",
			   (dim-1-badj), badi, res_bad.red, res_bad.green, res_bad.blue, res_bad.alpha);
#endif
	}

	return err;
}

static pixel check_average(int dim, int i, int j, pixel *src) {
	pixel result;
	int num = 0;
	int ii, jj;
	int sum0, sum1, sum2, sum3;
	int top_left_i, top_left_j;
	int bottom_right_i, bottom_right_j;

	top_left_i = max(i-1, 0);
	top_left_j = max(j-1, 0);
	bottom_right_i = min(i+1, dim-1); 
	bottom_right_j = min(j+1, dim-1);

	sum0 = sum1 = sum2 = sum3 = 0;
	for(ii=top_left_i; ii <= bottom_right_i; ii++) {
		for(jj=top_left_j; jj <= bottom_right_j; jj++) {
			num++;
			sum0 += (int) src[RIDX(ii,jj,dim)].red;
			sum1 += (int) src[RIDX(ii,jj,dim)].green;
			sum2 += (int) src[RIDX(ii,jj,dim)].blue;
			sum3 += (int) src[RIDX(ii,jj,dim)].alpha;
		}
	}
	result.red = (unsigned short) (sum0/num);
	result.green = (unsigned short) (sum1/num);
	result.blue = (unsigned short) (sum2/num);
	result.alpha = (unsigned short) (sum3/num);
 
	return result;
}


/* 
 * check_smooth - Make sure the smooth function actually works.  The
 * orig array should not have been tampered with!  
 */
static int check_smooth(int dim) {
	int err = 0;
	int i, j;
#ifdef STUDENT
	int badi = 0;
	int badj = 0;
	pixel right, wrong;
#endif

	/* return 1 if original image has been changed */
	if (check_orig(dim)) 
		return 1; 

	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			pixel smoothed = check_average(dim, i, j, orig);
			if (compare_pixels(result[RIDX(i,j,dim)], smoothed)) {
				err++;
#ifdef STUDENT
				badi = i;
				badj = j;
				wrong = result[RIDX(i,j,dim)];
				right = smoothed;
#endif
			}
		}
	}

	if (err) {
#ifdef STUDENT
		printf("\n");
	        printf("ERROR: Dimension=%d, %d errors\n", dim, err);    
		printf("E.g., \n");
		printf("You have dst[%d][%d].{red,green,blue,alpha} = {%d,%d,%d,%d}\n",
			   badi, badj, wrong.red, wrong.green, wrong.blue, wrong.alpha);
		printf("It should be dst[%d][%d].{red,green,blue} = {%d,%d,%d,%d}\n",
			   badi, badj, right.red, right.green, right.blue, right.alpha);
#endif
	}

	return err;
}


void func_wrapper(void *arglist[]) 
{
	pixel *src, *dst;
	int mydim;
	lab_test_func f;

	f = (lab_test_func) arglist[0];
	mydim = *((int *) arglist[1]);
	src = (pixel *) arglist[2];
	dst = (pixel *) arglist[3];

	(*f)(mydim, src, dst);

	return;
}

void run_rotate_benchmark(int idx, int dim) 
{
	benchmarks_rotate[idx].tfunct(dim, orig, result);
}

void test_rotate(int bench_index) 
{
	int i;
	int test_num;
  
	for (test_num = 0; test_num < DIM_CNT; test_num++) {
		int dim;

		/* Check for odd dimension */
		create(ODD_DIM);
		run_rotate_benchmark(bench_index, ODD_DIM);
		if (check_rotate(ODD_DIM)) {
#ifdef STUDENT
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   benchmarks_rotate[bench_index].description, ODD_DIM);
#else
                        fprintf(bench_out, "rotate_error:%s,%d\n", 
                            benchmarks_rotate[bench_index].description, ODD_DIM);
#endif
			return;
		}

		/* Create a test image of the required dimension */
		dim = test_dim_rotate[test_num];
		create(dim);
#ifdef DEBUG
		printf("DEBUG: Running benchmark \"%s\"\n", benchmarks_rotate[bench_index].description);
#endif

		/* Check that the code works */
		run_rotate_benchmark(bench_index, dim);
		if (check_rotate(dim)) {
#ifdef STUDENT
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   benchmarks_rotate[bench_index].description, dim);
#else
                        fprintf(bench_out, "rotate_error:%s,%d\n", 
                            benchmarks_rotate[bench_index].description, dim);
#endif
			return;
		}

		/* Measure CPE */
		{
			double num_cycles, cpe;
			int tmpdim = dim;
			void *arglist[4];
			double dimension = (double) dim;
			double work = dimension*dimension;
#ifdef DEBUG
			printf("DEBUG: dimension=%.2f\n",dimension);
			printf("DEBUG: work=%.2f\n",work);
#endif
			arglist[0] = (void *) benchmarks_rotate[bench_index].tfunct;
			arglist[1] = (void *) &tmpdim;
			arglist[2] = (void *) orig;
			arglist[3] = (void *) result;

			create(dim);
			num_cycles = fcyc_v((test_funct_v)&func_wrapper, arglist); 
			cpe = num_cycles/work;
			benchmarks_rotate[bench_index].cpes[test_num] = cpe;
		}
	}

#ifdef STUDENT
	/* 
	 * Print results as a table 
	 */
	printf("Rotate: Version = %s:\n", benchmarks_rotate[bench_index].description);
	printf("Dim\t");
	for (i = 0; i < DIM_CNT; i++)
		printf("\t%d", test_dim_rotate[i]);
	printf("\tMean\n");
  
	printf("Your CPEs");
	for (i = 0; i < DIM_CNT; i++) {
		printf("\t%.2f", benchmarks_rotate[bench_index].cpes[i]);
	}
	printf("\n");

	printf("Baseline CPEs");
	for (i = 0; i < DIM_CNT; i++) {
		printf("\t%.2f", rotate_baseline_cpes[i]);
	}
	printf("\n");
#endif

	/* Compute Speedup */
	{
#ifndef STUDENT
                fprintf(bench_out, "rotate_result:%s,", benchmarks_rotate[bench_index].description);
#endif
		double prod, ratio, mean;
		prod = 1.0; /* Geometric mean */
#ifdef STUDENT
		printf("Speedup\t");
#endif
		for (i = 0; i < DIM_CNT; i++) {
                        double cpe = benchmarks_rotate[bench_index].cpes[i];
#ifndef STUDENT
                        fprintf(bench_out, "%d,%.2f,", test_dim_rotate[i], cpe);
#endif
			if (cpe > 0.0) {
				ratio = rotate_baseline_cpes[i]/
					benchmarks_rotate[bench_index].cpes[i];
			}
			else {
				printf("Fatal Error: Non-positive CPE value...\n");
				exit(EXIT_FAILURE);
			}
#ifndef STUDENT
                        fprintf(bench_out, "%.2f,", ratio);
#endif
			prod *= ratio;
#ifdef STUDENT
			printf("\t%.2f", ratio);
#endif
		}

		/* Geometric mean */
		mean = pow(prod, 1.0/(double) DIM_CNT);
#ifndef STUDENT
                fprintf(bench_out, "%.2f\n", mean);
#endif
#ifdef STUDENT
		printf("\t%.2f", mean);
		printf("\n\n");
#endif
		if (mean > rotate_maxmean) {
			rotate_maxmean = mean;
			rotate_maxmean_desc = benchmarks_rotate[bench_index].description;
		}
		if (mean < rotate_minmean) {
			rotate_minmean = mean;
		}
	}


#ifdef DEBUG
	fflush(stdout);
#endif
	return;  
}

void run_smooth_benchmark(int idx, int dim) 
{
	benchmarks_smooth[idx].tfunct(dim, orig, result);
}

void test_smooth(int bench_index) 
{
	int i;
	int test_num;

        /* Check correctness for odd (non power of two dimensions */
        create(ODD_DIM);
        run_smooth_benchmark(bench_index, ODD_DIM);
        if (check_smooth(ODD_DIM)) {
#ifdef STUDENT
                printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
                           benchmarks_smooth[bench_index].description, ODD_DIM);
#else
                fprintf(bench_out, "smooth_error:%s,%d\n", 
                    benchmarks_smooth[bench_index].description, ODD_DIM);
#endif
                return;
        }
  
	for(test_num=0; test_num < DIM_CNT; test_num++) {
		int dim;

		/* Create a test image of the required dimension */
		dim = test_dim_smooth[test_num];
		create(dim);

#ifdef DEBUG
		printf("DEBUG: Running benchmark \"%s\"\n", benchmarks_smooth[bench_index].description);
#endif
		/* Check that the code works */
		run_smooth_benchmark(bench_index, dim);
		if (check_smooth(dim)) {
#ifdef STUDENT
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   benchmarks_smooth[bench_index].description, dim);
#else
                        fprintf(bench_out, "smooth_error:%s,%d\n", 
                            benchmarks_smooth[bench_index].description, dim);
#endif
			return;
		}

		/* Measure CPE */
		{
			double num_cycles, cpe;
			int tmpdim = dim;
			void *arglist[4];
			double dimension = (double) dim;
			double work = dimension*dimension;
#ifdef DEBUG
			printf("DEBUG: dimension=%.2f\n",dimension);
			printf("DEBUG: work=%.2f\n",work);
#endif
			arglist[0] = (void *) benchmarks_smooth[bench_index].tfunct;
			arglist[1] = (void *) &tmpdim;
			arglist[2] = (void *) orig;
			arglist[3] = (void *) result;
		
			create(dim);
			num_cycles = fcyc_v((test_funct_v)&func_wrapper, arglist); 
			cpe = num_cycles/work;
			benchmarks_smooth[bench_index].cpes[test_num] = cpe;
		}
	}

	/* Print results as a table */
#ifdef STUDENT
	printf("Smooth: Version = %s:\n", benchmarks_smooth[bench_index].description);
	printf("Dim\t");
	for (i = 0; i < DIM_CNT; i++)
		printf("\t%d", test_dim_smooth[i]);
	printf("\tMean\n");
  
	printf("Your CPEs");
	for (i = 0; i < DIM_CNT; i++) {
		printf("\t%.2f", benchmarks_smooth[bench_index].cpes[i]);
	}
	printf("\n");

	printf("Baseline CPEs");
	for (i = 0; i < DIM_CNT; i++) {
		printf("\t%.2f", smooth_baseline_cpes[i]);
	}
	printf("\n");
#endif

	/* Compute speedup */
	{
#ifndef STUDENT
                fprintf(bench_out, "smooth_result:%s,", benchmarks_smooth[bench_index].description);
#endif
		double prod, ratio, mean;
		prod = 1.0; /* Geometric mean */
#ifdef STUDENT
		printf("Speedup\t");
#endif
		for (i = 0; i < DIM_CNT; i++) {
                        double cpe = benchmarks_smooth[bench_index].cpes[i];
#ifndef STUDENT
                        fprintf(bench_out, "%d,%.2f,", test_dim_smooth[i], cpe);
#endif
			if (cpe > 0.0) {
				ratio = smooth_baseline_cpes[i]/
					benchmarks_smooth[bench_index].cpes[i];
			}
			else {
				printf("Fatal Error: Non-positive CPE value...\n");
				exit(EXIT_FAILURE);
			}
#ifndef STUDENT
                        fprintf(bench_out, "%.2f,", ratio);
#endif
			prod *= ratio;
#ifdef STUDENT
			printf("\t%.2f", ratio);
#endif
		}
		/* Geometric mean */
		mean = pow(prod, 1.0/(double) DIM_CNT);
#ifndef STUDENT
                fprintf(bench_out, "%.2f\n", mean);
#else
		printf("\t%.2f", mean);
		printf("\n\n");
#endif
		if (mean > smooth_maxmean) {
			smooth_maxmean = mean;
			smooth_maxmean_desc = benchmarks_smooth[bench_index].description;
		}
		if (mean > smooth_minmean) {
			smooth_minmean = mean;
		}
	}

	return;  
}


void usage(char *progname) 
{
	fprintf(stderr, "Usage: %s [-hqg] [-f <func_file>] [-d <dump_file>]\n", progname);    
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -h         Print this message\n");
	fprintf(stderr, "  -q         Quit after dumping (use with -d )\n");
	fprintf(stderr, "  -g         Autograder mode: checks only rotate() and smooth()\n");
	fprintf(stderr, "  -f <file>  Get test function names from dump file <file>\n");
	fprintf(stderr, "  -d <file>  Emit a dump file <file> for later use with -f\n");
	exit(EXIT_FAILURE);
}

void baseline_benchmark_rotate() 
{
	int test_num;
	char *description = "baseline rotate";
  
	for (test_num = 0; test_num < DIM_CNT; test_num++) {
		int dim;

		/* Check for odd dimension */
		create(ODD_DIM);
		baseline_naive_rotate(ODD_DIM, orig, result);
		if (check_rotate(ODD_DIM)) {
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   description, ODD_DIM);
			return;
		}

		/* Create a test image of the required dimension */
		dim = test_dim_rotate[test_num];
		create(dim);

		/* Check that the code works */
		baseline_naive_rotate(dim, orig, result);
		if (check_rotate(dim)) {
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   description, dim);
			return;
		}

		/* Measure CPE */
		{
			double num_cycles, cpe;
			int tmpdim = dim;
			void *arglist[4];
			double dimension = (double) dim;
			double work = dimension*dimension;

			arglist[0] = (void *) baseline_naive_rotate;
			arglist[1] = (void *) &tmpdim;
			arglist[2] = (void *) orig;
			arglist[3] = (void *) result;

			create(dim);
			num_cycles = fcyc_v((test_funct_v)&func_wrapper, arglist); 
			cpe = num_cycles/work;
			rotate_baseline_cpes[test_num] = cpe;
		}
	}

	return;  
}
void baseline_benchmark_smooth()
{
	int test_num;
	char *description = "baseline smooth";
  
	for(test_num=0; test_num < DIM_CNT; test_num++) {
		int dim;

		/* Check correctness for odd (non power of two dimensions */
		create(ODD_DIM);
		baseline_naive_smooth(ODD_DIM, orig, result);
		if (check_smooth(ODD_DIM)) {
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   description, ODD_DIM);
			return;
		}

		/* Create a test image of the required dimension */
		dim = test_dim_smooth[test_num];
		create(dim);

		/* Check that the code works */
		baseline_naive_smooth(dim, orig, result);
		if (check_smooth(dim)) {
			printf("Benchmark \"%s\" failed correctness check for dimension %d.\n",
				   description, dim);
			return;
		}

		/* Measure CPE */
		{
			double num_cycles, cpe;
			int tmpdim = dim;
			void *arglist[4];
			double dimension = (double) dim;
			double work = dimension*dimension;

			arglist[0] = (void *) &baseline_naive_smooth;
			arglist[1] = (void *) &tmpdim;
			arglist[2] = (void *) orig;
			arglist[3] = (void *) result;
		
			create(dim);
			num_cycles = fcyc_v((test_funct_v)&func_wrapper, arglist); 
			cpe = num_cycles/work;
			smooth_baseline_cpes[test_num] = cpe;
		}
	}

	return;  
}


void set_baselines()
{
	baseline_benchmark_smooth();
	baseline_benchmark_rotate();
}

double graderotate() {
	if (rotate_maxmean < 1.0) return 0.0;
	if (rotate_maxmean < 1.5) return 24.0 * (rotate_maxmean - 1.0) / (1.5 - 1.0);
	if (rotate_maxmean < 2.5) return 24.0 + 8.0 * (rotate_maxmean - 1.5) / (2.5 - 1.5);
	return 32.0;
}
double gradesmooth() {
	if (smooth_maxmean < 1.0) return 0.0;
	if (smooth_maxmean < 2.2) return 24.0 * (smooth_maxmean - 1.0) / (2.2 - 1.0);
	if (smooth_maxmean < 3.8) return 24.0 + 8.0 * (smooth_maxmean - 2.2) / (3.8 - 2.0);
	return 32.0;
}



void grades() {
	printf("  Rotate: %3.2f (%s)\n", rotate_maxmean, rotate_maxmean_desc);
	printf("  Smooth: %3.2f (%s)\n", smooth_maxmean, smooth_maxmean_desc);
}

void htmlsafe(char *name) {
	int i;
	for(i = 0; name[i]; i+=1) {
		if (i > 64) {name[i] = '\0'; break;}
		if (name[i] == ' ' || name[i] == '-' || name[i] == '.' || name[i] == '@' || name[i] == ':') continue;
                if (name[i] == '(' || name[i] == ')') continue;
		if (name[i] >= '0' && name[i] <= '9') continue;
		if (name[i] >= 'a' && name[i] <= 'z') continue;
		if (name[i] >= 'A' && name[i] <= 'Z') continue;
		name[i] = ' ';
	}
}

int main(int argc, char *argv[])
{
	int i,j;
	int quit_after_dump = 0;
	int skip_teamname_check = 0;
	int autograder = 0;
	int seed = 1729;
        int print_baselines = 0;
	char c = '0';
	char *bench_func_file = NULL;
	char *func_dump_file = NULL;

#ifndef STUDENT
        int new_stdout = dup(1);
        bench_out = fdopen(new_stdout, "w");
        setlinebuf(bench_out);

        fflush(stdout);
        dup2(2, 1);
        fflush(stdout);
#endif

	/* register all the defined functions */
	register_rotate_functions();
	register_smooth_functions();

	/* parse command line args */
	while ((c = getopt(argc, argv, "btgqf:d:s:h")) != -1)
		switch (c) {

		case 't': /* skip team name check (hidden flag) */
			skip_teamname_check = 1;
			break;

		case 's': /* seed for random number generator (hidden flag) */
			seed = atoi(optarg);
			break;

		case 'g': /* autograder mode (checks only rotate() and smooth()) */
			autograder = 1;
			break;

		case 'q':
			quit_after_dump = 1;
			break;

		case 'f': /* get names of benchmark functions from this file */
			bench_func_file = strdup(optarg);
			break;

		case 'd': /* dump names of benchmark functions to this file */
			func_dump_file = strdup(optarg);
			{
				int i;
				FILE *fp = fopen(func_dump_file, "w");  

				if (fp == NULL) {
					printf("Can't open file %s\n",func_dump_file);
					exit(-5);
				}

				for(i = 0; i < rotate_benchmark_count; i++) {
					fprintf(fp, "R:%s\n", benchmarks_rotate[i].description); 
				}
				for(i = 0; i < smooth_benchmark_count; i++) {
					fprintf(fp, "S:%s\n", benchmarks_smooth[i].description); 
				}
				fclose(fp);
			}
			break;
                case 'b':
                        print_baselines = 1;
                        break;

		case 'h': /* print help message */
			usage(argv[0]);
                        break;


		default: /* unrecognized argument */
			usage(argv[0]);
		}

	if (quit_after_dump) 
		exit(EXIT_SUCCESS);

	/* Print team info */
	if (!skip_teamname_check) {
		if (strcmp("bovik", team.team) == 0) {
			printf("%s: Please fill in the team struct in kernels.c.\n", argv[0]);
			exit(1);
		}
#ifdef STUDENT
		printf("Teamname: %s\n", team.team);
		printf("Member 1: %s (%s)\n", team.name1, team.email1);
		if (*team.name2 || *team.email2) {
			printf("Member 2: %s (%s)\n", team.name2, team.email2);
		} else {
			printf("\n");
		}
#endif
	}

	srand(seed);

	/* 
	 * If we are running in autograder mode, we will only test
	 * the rotate() and bench() functions.
	 */
	if (autograder) {
		rotate_benchmark_count = 1;
		smooth_benchmark_count = 1;

		benchmarks_rotate[0].tfunct = rotate;
		benchmarks_rotate[0].description = "rotate() function";
		benchmarks_rotate[0].valid = 1;

		benchmarks_smooth[0].tfunct = smooth;
		benchmarks_smooth[0].description = "smooth() function";
		benchmarks_smooth[0].valid = 1;
	}

	/* 
	 * If the user specified a file name using -f, then use
	 * the file to determine the versions of rotate and smooth to test
	 */
	else if (bench_func_file != NULL) {
		char flag;
		char func_line[256];
		FILE *fp = fopen(bench_func_file, "r");

		if (fp == NULL) {
			printf("Can't open file %s\n",bench_func_file);
			exit(-5);
		}
	
		while(func_line == fgets(func_line, 256, fp)) {
			char *func_name = func_line;
			char **strptr = &func_name;
			char *token = strsep(strptr, ":");
			flag = token[0];
			func_name = strsep(strptr, "\n");
#ifdef DEBUG
			printf("Function Description is %s\n",func_name);
#endif

			if (flag == 'R') {
				for(i=0; i<rotate_benchmark_count; i++) {
					if (strcmp(benchmarks_rotate[i].description, func_name) == 0)
						benchmarks_rotate[i].valid = 1;
				}
			}
			else if (flag == 'S') {
				for(i=0; i<smooth_benchmark_count; i++) {
					if (strcmp(benchmarks_smooth[i].description, func_name) == 0)
						benchmarks_smooth[i].valid = 1;
				}
			}      
		}

		fclose(fp);
	}

	/* 
	 * If the user didn't specify a dump file using -f, then 
	 * test all of the functions
	 */
	else { /* set all valid flags to 1 */
		for (i = 0; i < rotate_benchmark_count; i++)
			benchmarks_rotate[i].valid = 1;
		for (i = 0; i < smooth_benchmark_count; i++)
			benchmarks_smooth[i].valid = 1;
	}


	/* Set measurement (fcyc) parameters */
#ifndef STUDENT
	set_fcyc_cache_size(1 << 24); /* 16 MB cache size */
#endif
	set_fcyc_clear_cache(1); /* clear the cache before each measurement */
       
#ifdef STUDENT
        set_fcyc_k(4);
#else
        set_fcyc_k(30);
	set_baselines();
#endif
	set_baselines();

        if (print_baselines) {
            for (int i = 0; i < DIM_CNT; ++i) {
                printf("#define R%d %.5f\n", test_dim_rotate[i], rotate_baseline_cpes[i]);
            }
            for (int i = 0; i < DIM_CNT; ++i) {
                printf("#define S%d %.5f\n", test_dim_smooth[i], smooth_baseline_cpes[i]);
            }
            return 0;
        }

        

	for(j = 0; j < 3; j += 1) {
		smooth_minmean = 1.0;
		rotate_minmean = 1.0;

	 
                // we can expect less variation on the larger rotate sizes
                set_fcyc_k(10);
		for (i = 0; i < rotate_benchmark_count; i++) {
			if (benchmarks_rotate[i].valid) {
				test_rotate(i);
			}
		}
                set_fcyc_k(30);
		for (i = 0; i < smooth_benchmark_count; i++) {
			if (benchmarks_smooth[i].valid) {
				test_smooth(i);
			}
		}
		
#ifdef STUDENT
                break;
#else
		if (smooth_minmean > 0.99 && rotate_minmean > 0.99) break;
                set_fcyc_k(30);
                set_baselines();
                set_baselines();
                set_fcyc_k(30);
#endif
	}

#ifndef STUDENT
	char *tname = (char*)malloc(strlen(team.team)+1);
	strcpy(tname, team.team);
	htmlsafe(tname);
        char *name1 = strdup(team.name1);
        char *email1 = strdup(team.email1);
        char *name2;
        char *email2;
        if (team.name2 && *team.name2 && team.email2 && *team.email2) {
            name2 = strdup(team.name2);
            email2 = strdup(team.email2);
        } else {
            name2 = strdup("");
            email2 = strdup("");
        }
        htmlsafe(name1); htmlsafe(email1); htmlsafe(name2); htmlsafe(email2);
        fprintf(bench_out, "names:%s,%s,%s,%s,%s\n",
            tname, name1, email1, name2, email2);
        fprintf(bench_out, "overall_result:%3.2f,%3.2f,%.2f,%.2f\n", rotate_maxmean, smooth_maxmean,
            graderotate(), gradesmooth());

        fclose(bench_out);
#else
	printf("Summary of Your Best Scores:\n");
	printf("  Rotate: %3.1f (%s)\n", rotate_maxmean, rotate_maxmean_desc);
	printf("  Smooth: %3.1f (%s)\n", smooth_maxmean, smooth_maxmean_desc);
        printf("Note: Your machine may have different performance than our\n");
        printf("reference machine. Scores above are based on the ratio of\n");
        printf("cycles per element measured on your machine to a\n");
        printf("baseline measured on your machine.\n");
#endif

        return 0;
}
