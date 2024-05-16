#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include<algorithm>
#include <string.h>
#include <iostream>
#include <utility>

#include <stdio.h>

#include <opencv2/opencv.hpp>

namespace {
	enum VideoState {
		VideoStateWaitForHead,
		VideoStateSkip,
		VideoStateSkipFine,
		VideoStateSave,
		VideoStateSaveFine
	};

//	bool _checkLedIndicator(cv::Mat& img, cv::Mat& mask, const uint8_t thr_b, const uint8_t thr_g, const uint8_t thr_r)
	bool _checkLedIndicator(cv::Mat& img, const uint8_t thr_b, const uint8_t thr_g, const uint8_t thr_r)
	{
		int cnt = 0;

		//J Green
		for (size_t y = 0; y < img.size().height; ++y) {
			for (size_t x = 0; x < img.size().width; ++x) {
				cv::Vec3b p = img.at<cv::Vec3b>(cv::Point((int)x, (int)y));
//				cv::Vec3b m = mask.at<cv::Vec3b>(cv::Point((int)x, (int)y));
//				if (p[1] > thr_g && p[0] < thr_b && p[2] < thr_r) {
//				if (m[0] != 255 && p[1] > thr_g && p[2] < thr_r) {
				if (p[1] > thr_g && p[2] < thr_r) {
						cnt++;
#if 0
						cv::circle(img, cv::Point((int)x, (int)y), 20, cv::Scalar(0, 0, 255));
						cv::circle(img, cv::Point((int)x, (int)y), 5, cv::Scalar(0, 0, 255));
//						img.at<cv::Vec3b>(cv::Point(x, y)) = cv::Vec3b(0, 0, 255);
#endif
				}
			}
		}
		return cnt!=0;
	}
}


int main(int argc, char** argv)
{
	//J 入力動画と出力先の準備
	cv::VideoCapture cap;
	cap.open(argv[1]);

	//J LED on : 2sec 走る
	//J        : 6sec 走る
	//J        : 2sec Duty 0
	uint8_t vset_idx=0;
	uint8_t vset[] = { 25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8 };

	//J 頭出し
	cv::Mat img;
	int save_count=0;
	int skip_count = 0;

	std::string rec_name;
	cv::VideoWriter rec;

	// LED Indicatorの場所を見つけるために不要な情報を消す
//	cv::Mat mask = cv::imread("mask.png", 1);

	VideoState state = VideoStateWaitForHead;
	int cnt = 0;
	while (cap.read(img)) {
//		bool led = _checkLedIndicator(img, mask, 220, 243, 220);
		bool led = _checkLedIndicator(img, 150, 200, 150);
		if (cnt == 29145) led = 0;

		if (state == VideoStateWaitForHead) {
			if (led) {
				state = VideoStateSkip;
			}
		}
		else if (state == VideoStateSkip ){
			if (!led) {
				if (vset_idx >= sizeof(vset)) break;

				state = VideoStateSave;
				save_count = 6.1 * 220;
	
				rec_name = std::string(argv[1]) + std::string(".") + std::to_string((int)vset[vset_idx++]) + std::string(".mp4");
				rec.open(rec_name, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 59.94, cv::Size(1920, 1080), true);
			}
		}
		else if (state == VideoStateSave) {
			rec<<img;
			save_count--;
			if (save_count == 0) {
				state = VideoStateWaitForHead;
			}
		}

		std::cout << cnt++ << "\t" << led << "\t" << (int)vset[vset_idx] << std::endl;
//		if (cnt > 29144) {
//			cv::imshow("test", img);
//			cv::waitKey(0);
//		}

	}

	return 0;
}


