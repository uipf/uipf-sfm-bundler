
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

#define UIPF_MODULE_NAME "Bundler SfM"
#define UIPF_MODULE_ID "cebe.sfm.bundler.sfm"
#define UIPF_MODULE_CLASS BundlerSfMModule
#define UIPF_MODULE_CATEGORY "sfm"

#define UIPF_MODULE_INPUTS \
		{"imageGraph", uipf::DataDescription(uipfsfm::data::ImageGraph::id(), "the image graph with matching image pairs.")}

//#define UIPF_MODULE_OUTPUTS \
//		{"imageGraph", uipf::DataDescription(uipfsfm::data::ImageGraph::id(), "the image graph with matching image pairs.")}

// TODO workdir could be just a temporary directory
#define UIPF_MODULE_PARAMS \
		{"workdir", uipf::ParamDescription("working directory where key files will be placed.")}

// TODO param $MATCH_WINDOW_RADIUS

#include <uipf/Module.hpp>


// TODO configure this path with cmake
#define BUNDLER_BINARY "uipf-sfm-bundler"

using namespace uipf;
using namespace uipf::data;
using namespace uipf::util;
using namespace uipfsfm::data;

void BundlerSfMModule::run() {

	using namespace std;
	namespace fs = boost::filesystem;

	ImageGraph::ptr imageGraph = getInputData<ImageGraph>("imageGraph");

	// generate image list
	vector<string> imageList;
	uipf_foreach(imagePair, imageGraph->getContent()) {

		Image::ptr imageA = (*imagePair)->getContent().first;
		Image::ptr imageB = (*imagePair)->getContent().second;

		imageList.push_back(imageA->getContent());
		imageList.push_back(imageB->getContent());
	}
	unique(imageList.begin(), imageList.end());

	string imageListFileName = (fs::absolute(fs::path(getParam<string>("workdir", ".")) / fs::path("image_list.txt"))).string();
	ofstream imageListFile(imageListFileName);
	uipf_cforeach(i, imageList) {
		imageListFile << *i << "\n";
	}
	imageListFile.close();

	// generate options file
	string optionsFileName = (fs::absolute(fs::path(getParam<string>("workdir", ".")) / fs::path("options.txt"))).string();
	ofstream optionsFile(optionsFileName);
	fs::path bundleDir = fs::path(getParam<string>("workdir", ".")) / fs::path("bundle");
	if (!fs::exists(bundleDir)) {
		UIPF_LOG_INFO("Creating directory ", bundleDir.string());
		fs::create_directories(bundleDir);
	}

	// TODO create matches.init.txt from imagegraph
	string matchesFileName = fs::absolute(fs::path(getParam<string>("workdir", ".")) / fs::path("matches.init.txt")).string();

	// Generate the options file for running bundler
	optionsFile << "--match_table " << matchesFileName << "\n"; // TODO configurable
	optionsFile << "--output bundle.out" << "\n"; // TODO configurable
	optionsFile << "--output_all bundle_" << "\n"; // TODO configurable
	optionsFile << "--output_dir bundle" << "\n"; // TODO configurable
//	optionsFile << "--variable_focal_length" << "\n"; // TODO configurable set to fix for single camera
	optionsFile << "--use_focal_estimate" << "\n"; // TODO configurable set to fix for single camera
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
	chdir(getParam<string>("workdir", ".").c_str());
	system((string(BUNDLER_BINARY) + string(" ") + imageListFileName + string(" --options_file ") + optionsFileName).c_str());
	chdir(oldcwd.string().c_str());


}








