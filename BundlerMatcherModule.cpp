
#include <map>
#include <string>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include <uipf/logging.hpp>
#include <uipf/data/opencv.hpp>
#include <uipf/util.hpp>

#include <uipf-sfm/data/KeyPointList.hpp>
#include <uipf-sfm/data/Image.hpp>
#include <uipf-sfm/data/ImageGraph.hpp>

#define UIPF_MODULE_NAME "Bundler Matcher"
#define UIPF_MODULE_ID "cebe.sfm.bundler.matcher"
#define UIPF_MODULE_CLASS BundlerMatcherModule
#define UIPF_MODULE_CATEGORY "sfm"

#define UIPF_MODULE_INPUTS \
		{"images", uipf::DataDescription(uipf::data::List::id(), "the input image.")}

#define UIPF_MODULE_OUTPUTS \
		{"imageGraph", uipf::DataDescription(uipfsfm::data::ImageGraph::id(), "the image graph with matching image pairs.")}

// TODO workdir could be just a temporary directory
#define UIPF_MODULE_PARAMS \
		{"workdir", uipf::ParamDescription("working directory where key files will be placed.")}, \
		{"cache", uipf::ParamDescription("whether to cache results, implies createKeyFile. Defaults to false.", true)}, \
		{"windowRadius", uipf::ParamDescription("MATCH_WINDOW_RADIUS the only match images in a sliding window of size 2rad+1. Defaults to -1, i.e. match all images.", true)}

// TODO param $MATCH_WINDOW_RADIUS

#include <uipf/Module.hpp>


// TODO configure this path with cmake
#define MATCHER_BINARY "uipf-sfm-bundler-keymatchfull"

using namespace uipf;
using namespace uipf::data;
using namespace uipf::util;
using namespace uipfsfm::data;

void BundlerMatcherModule::run() {

	using namespace std;
	namespace fs = boost::filesystem;

	List::ptr imageList = getInputData<List>("images");

	fs::path workdir = getParam<string>("workdir", ".");
	if (!fs::exists(workdir)) {
		UIPF_LOG_INFO("Creating directory ", workdir.string());
		fs::create_directories(workdir);
	}

	// create list of images
	map<int, Image::ptr> images;
	int iid = 0;
	for(auto data: imageList->getContent()) {
		Image::ptr image = std::dynamic_pointer_cast<Image>(data);
		images.insert(pair<int, Image::ptr>(iid++, image));
	}

	string outFileName = (workdir / fs::path("matches.init.txt")).string();

	bool cache = getParam<bool>("cache", false);
	if (cache && fs::exists(fs::path(outFileName))) {
		UIPF_LOG_INFO("Loading matched keypoints from cache.");
	} else {
		UIPF_LOG_INFO("Matching keypoints (this can take a while)");

		string keyListFileName = (workdir / fs::path("list_keys.txt")).string();
		ofstream keylistFile(keyListFileName);
		uipf_foreach(image, images) {
			fs::path imageFileName = image->second->getContent();
			fs::path keyFileName =
					imageFileName.parent_path() / fs::path(imageFileName.stem().string() + string(".key"));
			if (!keyFileName.is_absolute()) {
				keyFileName = fs::canonical(fs::absolute(keyFileName));
			}
			UIPF_LOG_DEBUG("adding file: ", keyFileName.string());
			keylistFile << keyFileName.string() << "\n";
			// TODO (later) create key file if it does not exist (non bundler compliant pointmatcher)

		}
		keylistFile.close();

		int windowRadius = getParam("windowRadius", -1);
		// $MATCHKEYS list_keys.txt matches.init.txt $MATCH_WINDOW_RADIUS
		system((string(MATCHER_BINARY) + string(" ")
		      + keyListFileName + string(" ")
		      + outFileName
		      + (windowRadius > 0 ? string(" ") + to_string(windowRadius) : "")).c_str()
		);
	}

	// read matches file
	// format:
	// image pair: i j \n
	// # of matches: d \n
	// indexes of matching points: ka kb
	vector<ImagePair::ptr> imagePairs;
	ifstream outFile(outFileName);
	while(!outFile.eof()) {
		int i;
		int j;
		outFile >> i;
		outFile >> j;
		ImagePair::ptr imagePair(new ImagePair(pair<Image::ptr, Image::ptr>(images[i], images[j])));
		// number of matches
		int d;
		outFile >> d;
		for( ; d > 0; d--) {
			int ka;
			int kb;
			outFile >> ka;
			outFile >> kb;
			imagePair->keyPointMatches.push_back(pair<int, int>(ka, kb));
		}
		imagePair->hasKeyPointMatches = true;
		imagePairs.push_back(imagePair);
	}


	// TODO rm keylistFile
	// TODO rm key file if created before
	// TODO rm outfile

	UIPF_LOG_INFO("Created image graph with ", imagePairs.size(), " image pairs.");

	ImageGraph::ptr imageGraph(new ImageGraph(imagePairs));
	imageGraph->images = images;
	setOutputData<ImageGraph>("imageGraph", imageGraph);
}
