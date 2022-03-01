#include <math.h>
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
#include <vector>
#include <cmath>

#define FPS_PERCENTAGE 1 // use this option to scale the frame rate of your video playback. Must be between 0 and 1.
#define END_TIME  90 // use this option to set the desired end time for your video
#define START_TIME 30 // use this option to set the desired start time for your video
#define VIDEO_FILE "../images/badapple.mp4"

static const char* s_listen_on = "ws://localhost:8000";

std::vector<mg_connection*> webSockets;
std::queue<std::string> imgQueu;
std::mutex imgMutex;
std::mutex wsMutex;

std::shared_ptr<cv::VideoWriter> outputVideo = NULL;

int screenshotNum = 0;

#define VIDEO_FILE_OUPUT "../images/screenrecord2.avi"
//this is output fps of our video file
double outFPS = 0;

void sendMessage(std::string message) {
	wsMutex.lock();
	for (auto& item : webSockets) {
		mg_ws_send(item, message.c_str(), message.size(), WEBSOCKET_OP_TEXT);
	}
	wsMutex.unlock();
}

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
//   /api/rest - respond with JSON string {"result": 123}
//   any other URI serves static files from s_web_root
static void fn(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
	if (ev == MG_EV_OPEN) {
		// c->is_hexdumping = 1;
	}
	else if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		if (mg_http_match_uri(hm, "/websocket")) {
			// Upgrade to websocket. From now on, a connection is a full-duplex
			// Websocket connection, which will receive MG_EV_WS_MSG events.
			printf("websocket opened\n");
			wsMutex.lock();
			mg_ws_upgrade(c, hm, NULL);
			webSockets.push_back(c);
			wsMutex.unlock();
		}
		else if (mg_http_match_uri(hm, "/rest")) {
			// Serve REST response
			mg_http_reply(c, 200, "", "{\"result\": %d}\n", 123);
		}
		else {
			// Serve static files

			struct mg_http_serve_opts opts = {};
			std::filesystem::path webFilesRel("../WebFiles");
			std::filesystem::path webFilesAbs = std::filesystem::absolute(webFilesRel);
			std::string absString = webFilesAbs.generic_string();
			opts.root_dir = absString.c_str(); // Serve local dir
			mg_http_serve_dir(c, (mg_http_message*)ev_data, &opts);
		}
	}
	else if (ev == MG_EV_WS_MSG) {

		struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;

		//client has requested a new photo
		if ((strcmp(wm->data.ptr, "photo") == 0)) {
			if (imgQueu.empty()) {

				printf("we are finished playing our video!");

				//send terminated packet
				sendMessage("1");

				//if we were writing a video, close it!
				if(outputVideo)
					outputVideo->release();


			}
			else {
				imgMutex.lock();
				sendMessage(imgQueu.front());
				imgQueu.pop();
				imgMutex.unlock();
			}
		}
		else { // binary blob, try saving image

			//std::ofstream image("../images/screenshot"+ std::to_string(screenshotNum++) + ".png", std::ios::binary);

			//opencv demands a vector for imdecode function for some reason
			std::vector<char> charVec;
			charVec.assign(wm->data.ptr, wm->data.ptr + wm->data.len);

			cv::Mat imMat = cv::imdecode(charVec, cv::ImreadModes::IMREAD_COLOR);

			//if this is our first frame, open video writer
			if(outputVideo==NULL)
				outputVideo = std::make_shared<cv::VideoWriter>(VIDEO_FILE_OUPUT, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), outFPS, cv::Size(imMat.cols, imMat.rows));
			outputVideo->write(imMat);

			//image.write(wm->data.ptr, wm->data.len);
			//image.close();
		}
	}
	(void)fn_data;
}


//warning, must free bitmaps allocated here
//this function converts cv mat to potrace bitmap, see http://potrace.sourceforge.net/potracelib.pdf for potrace format
potrace_bitmap_t* matToBitMap(cv::Mat& mat) {

	//allocate bitmap, this must be freed after usage!
	potrace_bitmap_t* bitmap = bm_new(mat.cols, mat.rows);

	bitmap->w = mat.cols;
	bitmap->h = mat.rows;

	//in bits
	const int wordLength = sizeof(potrace_word) * 8;

	//round up division for number of words per scaline
	const int dy = (mat.cols - 1 + wordLength) / wordLength;

	//this is word position that we are on in map
	int arrPos = 0;

	//loop through mat from bottom left to top right
	for (int i = mat.rows - 1; i >= 0; i--) {
		potrace_word word = 0x00000000;

		//writing position in our word, starts at most significant bit
		int bitpos = wordLength - 1;

		for (int j = 0; j < mat.cols; j++) {
			auto val = mat.at<uchar>(i, j);

			if (val == 0) {
				word |= 1UL << bitpos;
			}

			bitpos--;

			if (bitpos == -1) {
				bitmap->map[arrPos++] = word;
				bitpos = 31;
				word = 0x00000000;
			}
		}

		//if we just wrote a word, don't write the current one
		//TODO: test this
		if (bitpos != 31) {
			bitmap->map[arrPos++] = word;
			bitpos = wordLength - 1;
			word = 0x00000000;
		}
	}

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

	if (FPS_PERCENTAGE > 1 || FPS_PERCENTAGE <= 0) {
		printf("Error. Video percentage must be between 1 and 0\n");
		return -1;
	}



	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, s_listen_on, fn, NULL);     // Create listening connection
	printf("Starting WS listener on %s/websocket\n", s_listen_on);


	std::thread t([&]() {
		for (;;) mg_mgr_poll(&mgr, 1000);                   // Block forever
		});


	auto begin = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds timeWorking(0);




	cv::VideoCapture cap(VIDEO_FILE);
	double fps = cap.get(cv::CAP_PROP_FPS);
	auto frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);
	auto duration = frame_count / fps;

	outFPS = fps * FPS_PERCENTAGE;

	printf("fps = %f\n", fps);
	printf("Number of frames = %f\n", frame_count);
	printf("Duration (s) = %f\n", duration);
	printf("Start time: %d\n", START_TIME);
	printf("End time: %d\n", END_TIME);
	printf("FPS percentage: %f\n", FPS_PERCENTAGE);



	if (START_TIME > duration || START_TIME < 0) {
		printf("Error. Illegal range for start time. \n");
		return -1;
	}

	if (END_TIME > duration || END_TIME < 0) {
		printf("Error. Illegal range for end time. \n");
		return -1;
	}





	// Check if camera opened successfully
	if (!cap.isOpened()) {
		std::cout << "Error opening video stream or file" << std::endl;
		return -1;
	}

	auto settings = potrace_param_default();

	double desiredFPS = fps * FPS_PERCENTAGE;


	//choose a double as close as possible to our FPS_PERCENTAGE value
	double x = ((2 * std::floor(FPS_PERCENTAGE * pow(2, 31)) - 1) / (pow(2, 32)));

	int numSkip = 0, numFrame = 0;


	int currentFrameNum = -1;
	while (1) {

		currentFrameNum++;


		cv::Mat frame, gray;
		// Capture frame-by-frame
		cap >> frame;
		if (frame.empty()) {
			break;
		}

		//skip some ratio of frames because x and fps_percent are coprime
		if (!((std::fmod(x * currentFrameNum, 1)) <= FPS_PERCENTAGE)) {
			numSkip++;
			continue;
		}

		numFrame++;

		double time = double(currentFrameNum) / fps;

		//we don't want to show these frames yet
		if (time < START_TIME) {
			continue;
		}

		//we don't care about these frames
		if (time > END_TIME) {
			break;
		}


		auto workBegin = std::chrono::high_resolution_clock::now();

		cv::Canny(frame, gray, 400, 500);

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

		//build packet delimted by '$', because i really don't wanna use json!
		std::string outputStr = "";
		outputStr += "0$";
		outputStr += std::to_string(time)+"$";
		outputStr += std::to_string(currentFrameNum)+"/"+std::to_string(int(frame_count)) + "$";
		imgMutex.lock();
		outputStr += shapeListToString(shapelist);
		imgQueu.push(outputStr);
		imgMutex.unlock();

		potrace_state_free(output);
		free(shapelist.shapes);
		bm_free(bitmap);

		auto workEnd = std::chrono::high_resolution_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(workEnd - workBegin);
		timeWorking += diff;


	}

	printf("Frame ratio: %f\n", float(numFrame) / float(numFrame + numSkip));

	potrace_param_free(settings);

	// When everything done, release the video capture object
	cap.release();

	// Stop measuring time and calculate the elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);

	printf("Time measured: %.3f seconds. Time working: %.3f seconds.\n", elapsed.count() * 1e-9, timeWorking.count() * 1e-9);

	t.join();
	mg_mgr_free(&mgr);


	return 0;
}
