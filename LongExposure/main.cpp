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
	typedef struct MouseControlContext_t
	{
		int x;
		int y;

		std::vector<cv::Point> points;
	} MouseControlContext;

	static MouseControlContext s_mouseCtx = { -1, -1 };

	static void _mouse_lbutton_down(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
		ctx->points.push_back(cv::Point(ctx->x, ctx->y));
	}

	static void _mouse_lbutton_up(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
	}

	static void _mouse_rbutton_down(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
		ctx->points.clear();
	}

	static void _mouse_rbutton_up(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
	}

	static void _mouse_rbutton_double_click(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
	}

	static void _mosue_move(MouseControlContext* ctx, cv::Mat& in_img, cv::Mat& out_img)
	{
	}

	static void _mouse_callback(int event, int x, int y, int flags, void* userdata)
	{
		std::pair<cv::Mat*, cv::Mat*>* image_pair = (std::pair<cv::Mat*, cv::Mat*>*)userdata;

		cv::Mat* input_image = image_pair->first;
		cv::Mat* output_image = image_pair->second;

		s_mouseCtx.x = x;
		s_mouseCtx.y = y;

		switch (event) {
		case cv::EVENT_MOUSEMOVE:
			_mosue_move(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_LBUTTONDOWN:
			_mouse_lbutton_down(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_LBUTTONUP:
			_mouse_lbutton_up(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_RBUTTONDOWN:
			_mouse_rbutton_down(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_RBUTTONUP:
			_mouse_rbutton_up(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_LBUTTONDBLCLK:
			break;
		case cv::EVENT_RBUTTONDBLCLK:
			_mouse_rbutton_double_click(&s_mouseCtx, *input_image, *output_image);
			break;
		case cv::EVENT_MOUSEWHEEL:
			break;
		}

		return;
	}

	int _checkLedIndicator(cv::Mat& img, const uint8_t thr_b, const uint8_t thr_g, const uint8_t thr_r)
	{
		int cnt = 0;

		//J Green
		for (size_t y = 0; y < img.size().height; ++y) {
			for (size_t x = 0; x < img.size().width; ++x) {
				cv::Vec3b p = img.at<cv::Vec3b>(cv::Point(x,y));
//				if (p[1] > thr_g && p[0] < thr_b && p[2] < thr_r) {
				if (p[1] > thr_g && p[2] < thr_r) {
					cnt++;
#if 0
					img.at<cv::Vec3b>(cv::Point(x, y)) = cv::Vec3b(0,0,255);
#endif
				}
			}
		}
		return cnt;
	}

	void _mixture(cv::Mat& work, cv::Mat img) {
		for (size_t y = 0; y < img.size().height; ++y) {
			for (size_t x = 0; x < img.size().width; ++x) {
				cv::Vec3b pi = img.at<cv::Vec3b>(cv::Point(x, y));
				cv::Vec3b pw = work.at<cv::Vec3b>(cv::Point(x, y));

				if (pi[1] > pw[1]) {
					work.at<cv::Vec3b>(cv::Point(x, y)) = pi;
				}
			}
		}
	}

	enum CapState {
		CapStateWaitFallingEdge,
		CapStateWaitForLED,
		CapStateOnGoing
	};
}


int main(int argc, char** argv)
{
	int fps = 220;

	//J 入力動画と出力先の準備
	cv::VideoCapture cap;
	cap.open(argv[1]);

	cv::Mat img;
	cv::Mat work;

	cap.read(img);

	int straight_width = 0;
	if (argc == 2) {
		img.copyTo(work);

		//J コース上の既知点4か所を選択
		while (cv::waitKey(1) != 27) {
			cv::imshow("work", work);
			cv::setMouseCallback("work", _mouse_callback, NULL);

			img.copyTo(work);
			for (size_t i = 0; i < ::s_mouseCtx.points.size(); ++i) {
				cv::circle(work, s_mouseCtx.points[i], 2, cv::Scalar(0, 255, 255), 2);
			}
		}

		if (s_mouseCtx.points.size() != 2) return -1;

		straight_width = abs(s_mouseCtx.points[0].x - s_mouseCtx.points[1].x);
		std::cout << "straight width = " << straight_width << std::endl;
	}
	else {
		straight_width = atoi(argv[2]);
	}

	cv::Size frameSize = img.size();
	int width100mmInImage = (int)((float)straight_width * (100.0/540.0) + 0.5f);

	int cnt=0;
	int id = 0;
	int sof,eof;
	enum CapState state = CapStateWaitFallingEdge;

	char s_idx[100];
	do {
		bool isLedOn = _checkLedIndicator(img, 50, 50, 50) > 1;
		std::cout << cnt << '\t' << isLedOn <<std::endl;

		if (state == CapStateWaitFallingEdge) {
			if (!isLedOn) {
				state = CapStateWaitForLED;
			}
		}
		else if (state == CapStateWaitForLED) {
			if (isLedOn) {
				sof = cnt;
				img.copyTo(work);
				state = CapStateOnGoing;
			}
		}
		else if (state == CapStateOnGoing) {
			if (!isLedOn) {
				cv::line(work, cv::Point(1700, 1000), cv::Point(1700, 1000) + cv::Point(width100mmInImage, 0), cv::Scalar(255, 255, 255), 2);
				cv::line(work, cv::Point(1700, 1000) + cv::Point(0, -20), cv::Point(1700, 1000) + cv::Point(0, 20), cv::Scalar(255, 255, 255), 2);
				cv::line(work, cv::Point(1700, 1000) + cv::Point(0, -20) + cv::Point(width100mmInImage, 0), cv::Point(1700, 1000) + cv::Point(0, 20) + cv::Point(width100mmInImage, 0), cv::Scalar(255, 255, 255), 2);

				cv::putText(work, "100mm", cv::Point(1700, 980), cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

				snprintf(s_idx, sizeof(s_idx), "%03d", id++);
				cv::imwrite(std::string(argv[1])+"."+s_idx+ "." + std::to_string(sof) + "_"+std::to_string(eof)+".png", work);
				state = CapStateWaitForLED;
			}
			else {
				eof = cnt;
				_mixture(work, img);
			}

		}
#if 0
		cv::imshow("work", img);
		if (cv::waitKey(0) == 0x1b) {
			break;
		}
#endif
		cnt++;

	} while (cap.read(img));

	return 0;
}
