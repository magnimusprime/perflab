/*********************************************************
 * config.h - Configuration data for the driver.c program.
 *********************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 
 * CPEs for the baseline (naive) version of the rotate function that
 * was handed out to the students. Rd is the measured CPE for a dxd
 * image. Run the driver.c program on your system to get these
 * numbers.  
 */

#define R1536 13.14263
#define R1568 5.16275
#define R1792 11.34319
#define R2016 5.19230
#define R2048 18.74872
#define R2080 5.10622

/* 
 * CPEs for the baseline (naive) version of the smooth function that
 * was handed out to the students. Sd is the measure CPE for a dxd
 * image. Run the driver.c program on your system to get these
 * numbers.  
 */

#define S32 49.83789
#define S64 50.45068
#define S128 50.32690
#define S256 50.34412
#define S576 50.48233
#define S800 49.99465

#endif /* _CONFIG_H_ */
