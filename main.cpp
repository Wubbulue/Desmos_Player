#include <iostream>
#include "opencv2/opencv.hpp"
#include <filesystem>
#include "potracelib.h"
#include "bitmap.h"
#include "opencv2/imgcodecs.hpp"
#include "backend_svg.h"
#include "backend_latex.h"
#include <fstream>
#include "main_potrace.h"
#include <sstream>
#include "opencv2/core/utils/logger.hpp"
#include <chrono>
#include "mongoose.h"


static void fn(struct mg_connection* c, int ev, void *ev_data, void* fn_data) {
	
	struct mg_http_serve_opts opts = {};   
	std::filesystem::path webFilesRel("../WebFiles");
	std::filesystem::path webFilesAbs = std::filesystem::absolute(webFilesRel);
	std::string absString = webFilesAbs.generic_string();
	opts.root_dir = absString.c_str(); // Serve local dir
	if (ev == MG_EV_HTTP_MSG) mg_http_serve_dir(c, (mg_http_message *)ev_data, &opts);
}


//warning, must free bitmaps allocated here
//this function converts cv mat to potrace bitmap, see http://potrace.sourceforge.net/potracelib.pdf for potrace format
potrace_bitmap_t *matToBitMap(cv::Mat& mat) {

	//allocate bitmap, this must be freed after usage!
    potrace_bitmap_t *bitmap = bm_new(mat.cols, mat.rows);
	
    bitmap->w = mat.cols;
    bitmap->h = mat.rows;

	//in bits
	const int wordLength = sizeof(potrace_word) * 8;

	//round up division for number of words per scaline
	const int dy = (mat.cols-1+wordLength) / wordLength;

	//this is word position that we are on in map
	int arrPos = 0;

	//loop through mat from bottom left to top right
	for (int i = mat.rows-1; i >=0; i--) {
		potrace_word word = 0x00000000;

		//writing position in our word, starts at most significant bit
		int bitpos = wordLength-1;

		for (int j = 0; j<mat.cols; j++) {
			auto val = mat.at<uchar>(i, j);

			if (val == 0) {
				word |= 1UL << bitpos;
			}

			bitpos--;

			if (bitpos == -1) {
				(*bitmap).map[arrPos++] = word;
				bitpos = 31;
				word = 0x00000000;
			}
			//printf("Pixel at: %d,%d has value %d\n", j, i, val);
		}

		//if we just wrote a word, don't write the current one
		//TODO: test this
		if (bitpos != 31) {
			(*bitmap).map[arrPos++] = word;
			bitpos = wordLength-1;
			word = 0x00000000;
		}
	}

	//for (int i = 0; i < arrPos; i++) {
	//	printf("Postion %d val:%X\n", i, bitmap->map[i]);
	//}

    return bitmap;

}

std::string shapeListToString(shapeList shapelist) {
	std::stringstream ss;
	char buffer[1000];
	for (int i = 0; i < shapelist.currentIndex; i++) {
		struct shape* s = shapelist.shapes + i;
		if (s->isLine) {
			sprintf(buffer, "\\left(\\left(1-t\\right)%d+t%d,\\left(1-t\\right)%d+t%d\\right)\n", s->line[0].x, s->line[1].x, s->line[0].y, s->line[1].y);
			ss << buffer;
		}
		else {
			sprintf(buffer, "\\left(\\left(1-t\\right)\\left(\\left(1-t\\right)\\left(\\left(1-t\\right)%d+t%d\\right)+t\\left(\\left(1-t\\right)%d+t%d\\right)\\right)+t\\left(\\left(1-t\\right)\\left(\\left(1-t\\right)%d+t%d\\right)+t\\left(\\left(1-t\\right)%d+t%d\\right)\\right),\\left(1-t\\right)\\left(\\left(1-t\\right)\\left(\\left(1-t\\right)%d+t%d\\right)+t\\left(\\left(1-t\\right)%d+t%d\\right)\\right)+t\\left(\\left(1-t\\right)\\left(\\left(1-t\\right)%d+t%d\\right)+t\\left(\\left(1-t\\right)%d+t%d\\right)\\right)\\right)\n", s->curve[0].x, s->curve[1].x, s->curve[1].x, s->curve[2].x, s->curve[1].x, s->curve[2].x, s->curve[2].x, s->curve[3].x, s->curve[0].y, s->curve[1].y, s->curve[1].y, s->curve[2].y, s->curve[1].y, s->curve[2].y, s->curve[2].y, s->curve[3].y);
			ss << buffer;
		}
	}

	auto str = ss.str();
	return str;
}

int main() {

	cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);


	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, "0.0.0.0:8000", fn, NULL);     // Create listening connection
	
	std::thread t([&]() {
		for (;;) mg_mgr_poll(&mgr, 1000);                   // Block forever
	});


	auto begin = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds timeWorking(0);




	cv::VideoCapture cap("../images/nyan.mp4");

	// Check if camera opened successfully
	if (!cap.isOpened()) {
		std::cout << "Error opening video stream or file" << std::endl;
		return -1;
	}

	auto settings = potrace_param_default();


	for (int i = 0; i < 1000; i++) {
		cv::Mat frame, gray;
		// Capture frame-by-frame
		cap >> frame;
		if ((500 < i) && (i < 550)) {

			auto workBegin = std::chrono::high_resolution_clock::now();

			cv::Canny(frame, gray, 100, 200);

			auto bitmap = matToBitMap(gray);

			trans_s trans;
			trans.bb[0] = bitmap->w;
			trans.bb[1] = bitmap->h;
			trans.orig[0] = trans.orig[1] = 0;
			trans.x[0] = 1;
			trans.x[1] = 0;
			trans.y[0] = 0;
			trans.y[1] = 1;
			trans.scalex = 1;
			trans.scaley = 1;
			imginfo_t imgInfo{ bitmap->w,bitmap->h,bitmap->w / 4,bitmap->h / 4,0,0,0,0,trans };

			auto output = potrace_trace(settings, bitmap);

			auto shapelist = pageLatex(output->plist, &imgInfo);
			auto str = shapeListToString(shapelist);

			potrace_state_free(output);
			free(shapelist.shapes);
			bm_free(bitmap);

			auto workEnd = std::chrono::high_resolution_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(workEnd - workBegin);
			timeWorking += diff;


		}
	}

	potrace_param_free(settings);

	// When everything done, release the video capture object
	cap.release();

	// Stop measuring time and calculate the elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);

	printf("Time measured: %.3f seconds. Time working: %.3f seconds.\n", elapsed.count() * 1e-9, timeWorking.count() * 1e-9);

	t.join();

	return 0;
}