
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
		{"cache", uipf::ParamDescription("whether to cache results, implies createKeyFile. Defaults to false.", true)}

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

	List::ptr images = getInputData<List>("images");

	string outFileName = (fs::path(getParam<string>("workdir", ".")) / fs::path("matches.init.txt")).string();

	bool cache = getParam<bool>("cache", false);
	if (cache && fs::exists(fs::path(outFileName))) {
		UIPF_LOG_INFO("Loading matched keypoints from cache.");
	} else {
		UIPF_LOG_INFO("Matching keypoints (this can take a while)");

		string keyListFileName = (fs::path(getParam<string>("workdir", ".")) / fs::path("list_keys.txt")).string();
		ofstream keylistFile(keyListFileName);
		uipf_foreach(data, images->getContent()) {
			Image::ptr image = std::dynamic_pointer_cast<Image>(*data);
			fs::path imageFileName = image->getContent();
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


		// $MATCHKEYS list_keys.txt matches.init.txt $MATCH_WINDOW_RADIUS
		system((string(MATCHER_BINARY) + string(" ") + keyListFileName + string(" ") + outFileName).c_str());
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
		Image::ptr imageI = std::dynamic_pointer_cast<Image>(images->getContent()[i]); // TODO replace this cast with a typechecking function that check data::id
		Image::ptr imageJ = std::dynamic_pointer_cast<Image>(images->getContent()[j]);
		ImagePair::ptr imagePair(new ImagePair(pair<Image::ptr, Image::ptr>(imageI, imageJ)));
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
		imagePairs.push_back(imagePair);
	}


	// TODO rm keylistFile
	// TODO rm key file if created before
	// TODO rm outfile

	UIPF_LOG_INFO("Created image graph with ", imagePairs.size(), " image pairs.");

	setOutputData<ImageGraph>("imageGraph", new ImageGraph(imagePairs));
}