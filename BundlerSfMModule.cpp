
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <algorithm>

#include <uipf/logging.hpp>
#include <uipf/data/opencv.hpp>
#include <uipf/util.hpp>

#include <uipf-sfm/data/KeyPointList.hpp>
#include <uipf-sfm/data/Image.hpp>
#include <uipf-sfm/data/ImageGraph.hpp>

#include "BundlerFunctions.hpp"

#define UIPF_MODULE_NAME "Bundler SfM"
#define UIPF_MODULE_ID "cebe.sfm.bundler.sfm"
#define UIPF_MODULE_CLASS BundlerSfMModule
#define UIPF_MODULE_CATEGORY "sfm"

#define UIPF_MODULE_INPUTS \
		{"imageGraph", uipf::DataDescription(uipfsfm::data::ImageGraph::id(), "the image graph with matching image pairs.")}

#define UIPF_MODULE_OUTPUTS \
		{"imageGraph", uipf::DataDescription(uipfsfm::data::ImageGraph::id(), "the image graph with matching image pairs. (now added P matrixes)")}, \
		{"images", uipf::DataDescription(uipf::data::List::id(/* TODO typed list */), "all images. (now added P matrixes)")}

// TODO workdir could be just a temporary directory
#define UIPF_MODULE_PARAMS \
		{"workdir", uipf::ParamDescription("working directory where key files will be placed.")}

// TODO param $MATCH_WINDOW_RADIUS

#include <uipf/Module.hpp>


// TODO configure this path with cmake
#define BUNDLER_BINARY "uipf-sfm-bundler"

using namespace uipf;
using namespace uipf::data;
using namespace uipfsfm::data;

void BundlerSfMModule::run() {

	using namespace std;
	namespace fs = boost::filesystem;

	ImageGraph::ptr imageGraph = getInputData<ImageGraph>("imageGraph");

	fs::path workdir = fs::absolute(fs::path(getParam<string>("workdir", ".")));
	if (!fs::exists(workdir)) {
		UIPF_LOG_INFO("Creating directory ", workdir.string());
		fs::create_directories(workdir);
	}

	string imageListFileName = (workdir / fs::path("image_list.txt")).string();
	ofstream imageListFile(imageListFileName);
	uipf_cforeach(i, imageGraph->images) {
		imageListFile << (fs::absolute(fs::path(i->second->getContent()))).string() << "\n";
	}
	imageListFile.close();

	// generate options file
	string optionsFileName = (workdir / fs::path("options.txt")).string();
	ofstream optionsFile(optionsFileName);
	fs::path bundleDir = workdir / fs::path("bundle");
	if (!fs::exists(bundleDir)) {
		UIPF_LOG_INFO("Creating directory ", bundleDir.string());
		fs::create_directories(bundleDir);
	}

	// TODO create matches.init.txt from imagegraph
	string matchesFileName = (workdir / fs::path("matches.init.txt")).string();

	// write matches file
	// format:
	// image pair: i j \n
	// # of matches: d \n
	// indexes of matching points: ka kb
	ofstream matchesFile(matchesFileName);
	vector<ImagePair::ptr>& ipairs = imageGraph->getContent();
	UIPF_LOG_DEBUG("writing ", matchesFileName);
	for(auto it = ipairs.begin(); it != ipairs.end(); ++it) {

		//assert((*it)->hasKeyPointMatches);
		Image::ptr imageA = (*it)->getContent().first;
		Image::ptr imageB = (*it)->getContent().second;

		int iidA = -1;
		for(auto im: imageGraph->images) {
			if (im.second == imageA) {
				iidA = im.first;
				break;
			}
		}
		int iidB = -1;
		for(auto im: imageGraph->images) {
			if (im.second == imageB) {
				iidB = im.first;
				break;
			}
		}
//		UIPF_LOG_DEBUG("writing pair ", iidA, " ", iidB);
		matchesFile << iidA << " " << iidB << "\n";
		matchesFile << (*it)->keyPointMatches.size() << "\n";
		for(auto matchIt = (*it)->keyPointMatches.begin(); matchIt != (*it)->keyPointMatches.end(); ++matchIt) {
			matchesFile << matchIt->first << " " << matchIt->second << "\n";
		}
	}
	matchesFile.close();
	UIPF_LOG_DEBUG("done writing");

	string bundleFileName = (workdir / fs::path("bundle/bundle.out")).string();

	// Generate the options file for running bundler
	optionsFile << "--match_table " << matchesFileName << "\n"; // TODO configurable
	optionsFile << "--output bundle.out" << "\n"; // TODO configurable
	optionsFile << "--output_all bundle_" << "\n"; // TODO configurable
	optionsFile << "--output_dir " << bundleDir.string() << "\n"; // TODO configurable
//	optionsFile << "--variable_focal_length" << "\n"; // TODO configurable set to fix for single camera
//	optionsFile << "--use_focal_estimate" << "\n"; // TODO configurable set to fix for single camera
	optionsFile << "--constrain_focal" << "\n"; // TODO configurable set to fix for single camera
	optionsFile << "--constrain_focal_weight 0.0001" << "\n"; // TODO configurable set to fix for single camera

	// TODO:
//if [ "$TRUST_FOCAL" != "" ]
//then
//		echo "--trust_focal" >> options.txt
//		fi
//
//echo "--estimate_distortion" >> options.txt
//		echo "--ray_angle_threshold $RAY_ANGLE_THRESHOLD" >> options.txt

//	optionsFile << "--estimate_distortion" << "\n";
	optionsFile << "--ray_angle_threshold 2.0" << "\n";

//
//if [ "$NUM_MATCHES_ADD_CAMERA" != "" ]
//then
//		echo "--num_matches_add_camera $NUM_MATCHES_ADD_CAMERA" >> options.txt
//		fi
//
//if [ "$USE_CERES" != "" ]
//then
//		echo "--use_ceres" >> options.txt
//		fi
//
//echo "--run_bundle" >> options.txt
	optionsFile << "--run_bundle" << "\n"; // TODO configurable set to fix for single camera

	optionsFile.close();

//
//# Run Bundler!
//		echo "[- Running Bundler -]"
//rm -f constraints.txt
//		rm -f pairwise_scores.txt
//		$BUNDLER $IMAGE_LIST --options_file options.txt > bundle/bundle.log
//
//		echo "[- Done -]"

	// TODO logfile
	fs::path oldcwd = fs::absolute(fs::current_path());
	chdir(workdir.c_str());
	int ret = system((string(BUNDLER_BINARY) + string(" ") + imageListFileName + string(" --options_file ") + optionsFileName).c_str());
	chdir(oldcwd.string().c_str());
	if (ret != 0) {
		throw ErrorException("Bundler exited non-zero.");
	}


	// read P matrix from images
	// https://github.com/snavely/bundler_sfm#output-format

	std::vector<camera_params_t> cameras;
	std::vector<point_t> points;
	double bundle_version;
	if (!ReadBundleFile(bundleFileName.c_str(), cameras, points, bundle_version)) {
		throw ErrorException("error reading bundle file!");
	}
	int cid = 0;
	for(camera_params_t c: cameras) {
		Image::ptr& image = imageGraph->images.at(cid);
		image->camera.R = cv::Matx33d(c.R);
		image->camera.t = cv::Vec3d(c.t);
		image->camera.f = c.f;

		assert(image->width > 0 && image->height > 0);

//		double K[9] =
		cv::Matx33d K =
				{ -c.f, 0.0, 0.5 * image->width - 0.5,
				  0.0, c.f, 0.5 * image->height - 0.5,
				  0.0, 0.0, 1.0 };
		image->camera.K = K;
		image->hasCameraParameters = true;

//		double Ptmp[12] =
		cv::Matx34d Ptmp =
				{ c.R[0], c.R[1], c.R[2], c.t[0],
				  c.R[3], c.R[4], c.R[5], c.t[1],
				  c.R[6], c.R[7], c.R[8], c.t[2] };

		cv::Matx34d P = -(K * Ptmp);
//		matrix_product(3, 3, 3, 4, K, Ptmp, P);
//		matrix_scale(3, 4, P, -1.0, P);

		image->P = P;
		image->hasProjectionMatrix = true;

//		image->data.insert(pair<string, Data::ptr>("bundler_k1", uipf::data::Double::ptr(new Double(c.k[0]))));
//		image->data.insert(pair<string, Data::ptr>("bundler_k2", uipf::data::Double::ptr(new Double(c.k[1]))));

		cid++;
	}

	// TODO read 3D points

	List::ptr imageList(new List());
	for(auto image: imageGraph->images) {
		imageList->getContent().push_back(image.second);
	}

	setOutputData<ImageGraph>("imageGraph", imageGraph);
	setOutputData<List>("images", imageList);
}
