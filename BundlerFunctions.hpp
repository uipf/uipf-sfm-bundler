#ifndef UIPFSFM_BUNDLER_BUNDLER_FUNCTIONS
#define UIPFSFM_BUNDLER_BUNDLER_FUNCTIONS

/*
 * These functions are extracted from bundler source code:
 * https://github.com/snavely/bundler_sfm
 *
 *  Copyright (c) 2008  Noah Snavely (snavely (at) cs.washington.edu)
 *    and the University of Washington
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <vector>


#define NUM_CAMERA_PARAMS 9
#define POLY_INVERSE_DEGREE 6


struct BundlerPointRef {
	unsigned long pointId;
	unsigned long viewId;
	unsigned long featureId;
};

typedef struct {
	double R[9];     /* Rotation */
	double t[3];     /* Translation */
	double f;        /* Focal length */
	double k[2];     /* Undistortion parameters */
	double k_inv[POLY_INVERSE_DEGREE]; /* Inverse undistortion parameters */
	char constrained[NUM_CAMERA_PARAMS];
	double constraints[NUM_CAMERA_PARAMS];  /* Constraints (if used) */
	double weights[NUM_CAMERA_PARAMS];      /* Weights on the constraints */
	double K_known[9];  /* Intrinsics (if known) */
	double k_known[5];  /* Distortion params (if known) */

	char fisheye;            /* Is this a fisheye image? */
	char known_intrinsics;   /* Are the intrinsics known? */
	double f_cx, f_cy;       /* Fisheye center */
	double f_rad, f_angle;   /* Other fisheye parameters */
	double f_focal;          /* Fisheye focal length */

	double f_scale, k_scale; /* Scale on focal length, distortion params */
} camera_params_t;

typedef struct
{
	double pos[3];
	double color[3];

} point_t;

bool ReadBundleFile(const char *bundle_file,
                    std::vector<camera_params_t> &cameras,
                    std::vector<point_t> &points,
                    std::vector<BundlerPointRef> &ptref,
                    double &bundle_version);

#endif // UIPFSFM_BUNDLER_BUNDLER_FUNCTIONS
