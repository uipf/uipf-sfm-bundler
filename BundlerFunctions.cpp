#include "BundlerFunctions.hpp"

#include <stdio.h>
#include <string>
#include <string.h>
#include <locale.h>

/*
 * These functions are extracted from bundler source code:
 * https://github.com/snavely/bundler_sfm
 * Adjusted to parse files independently from the current locale (uselocale).
 * Also adjusted to return information about which point has been seen in which view
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

bool ReadBundleFile(const char *bundle_file,
                    std::vector<camera_params_t> &cameras,
                    std::vector<point_t> &points,
                    std::vector<BundlerPointRef> &ptref,
                    double &bundle_version)
{
	FILE *f = fopen(bundle_file, "r");
	if (f == NULL) {
		printf("Error opening file %s for reading\n", bundle_file);
		return false;
	}

	int num_images, num_points;

	locale_t new_locale = newlocale(LC_ALL, "C", 0);  ;
	locale_t old_locale = uselocale(new_locale);

	char first_line[256];
	fgets(first_line, 256, f);
	if (first_line[0] == '#') {
		double version;
		sscanf(first_line, "# Bundle file v%lf", &version);

		bundle_version = version;
		printf("[ReadBundleFile] Bundle version: %0.3f\n", version);

		fscanf(f, "%d %d\n", &num_images, &num_points);
	} else if (first_line[0] == 'v') {
		double version;
		sscanf(first_line, "v%lf", &version);
		bundle_version = version;
		printf("[ReadBundleFile] Bundle version: %0.3f\n", version);

		fscanf(f, "%d %d\n", &num_images, &num_points);
	} else {
		bundle_version = 0.1;
		sscanf(first_line, "%d %d\n", &num_images, &num_points);
	}

	printf("[ReadBundleFile] Reading %d images and %d points...\n",
	       num_images, num_points);

	/* Read cameras */
	for (int i = 0; i < num_images; i++) {
		double focal_length, k0, k1;
		double R[9];
		double t[3];

		if (bundle_version < 0.2) {
			/* Focal length */
			fscanf(f, "%lf\n", &focal_length);
		} else {
			fscanf(f, "%lf %lf %lf\n", &focal_length, &k0, &k1);
		}

		/* Rotation */
		fscanf(f, "%lf %lf %lf\n%lf %lf %lf\n%lf %lf %lf\n",
		       R+0, R+1, R+2, R+3, R+4, R+5, R+6, R+7, R+8);
		/* Translation */
		fscanf(f, "%lf %lf %lf\n", t+0, t+1, t+2);

		// if (focal_length == 0.0)
		//     continue;

		camera_params_t cam;

		cam.f = focal_length;
		memcpy(cam.R, R, sizeof(double) * 9);
		memcpy(cam.t, t, sizeof(double) * 3);

		cameras.push_back(cam);
	}

	/* Read points */
	for (int i = 0; i < num_points; i++) {
		point_t pt;

		/* Position */
		fscanf(f, "%lf %lf %lf\n",
		       pt.pos + 0, pt.pos + 1, pt.pos + 2);

		/* Color */
		fscanf(f, "%lf %lf %lf\n",
		       pt.color + 0, pt.color + 1, pt.color + 2);

		int num_visible;
		fscanf(f, "%d", &num_visible);

		for (int j = 0; j < num_visible; j++) {
			int view, key;
			fscanf(f, "%d %d", &view, &key);

			BundlerPointRef ref;
			ref.pointId = (unsigned long) i;
			ref.viewId = (unsigned long) view;
			ref.featureId = (unsigned long) key;
			ptref.push_back(ref);

			double x, y;
			if (bundle_version >= 0.3)
				fscanf(f, "%lf %lf", &x, &y);
		}

		if (num_visible > 0) {
			points.push_back(pt);
		}
	}

	uselocale(old_locale);

	fclose(f);
	return true;
}