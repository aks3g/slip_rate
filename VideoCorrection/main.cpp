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

}


int main(int argc, char** argv)
{
	//J 入力動画と出力先の準備
	cv::VideoCapture cap;
	cap.open(argv[1]);

	cv::VideoWriter rec;

	cv::Mat work;
	cv::Mat rsvd;

	// -----------------------------------------------------------------------------
	//J コースの既知点から、画面回転角度を特定する
	int key = 0;
	cap >> rsvd;
	rec.open(std::string(argv[1]) + "_result.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 59.94, rsvd.size(), true);

	//J コース上の既知点4か所を選択
	do{
		rsvd.copyTo(work);
		for (size_t i = 0; i < ::s_mouseCtx.points.size(); ++i) {
			cv::circle(work, s_mouseCtx.points[i], 2, cv::Scalar(0, 255, 255), 2);
		}

		cv::imshow("work", work);
		cv::setMouseCallback("work", _mouse_callback, NULL);
	} while (cv::waitKey(1) != 27);


	if (s_mouseCtx.points.size() != 4) return -1;

	std::vector<cv::Point2f> original;
	std::vector<cv::Point2f> transform;

	// 540mm x 1442mm サイズのコースを 1920x1080に縦をフィットさせて変形
	// 404px x 1080pxにする
	transform.push_back(cv::Point2f(s_mouseCtx.points[0].x, s_mouseCtx.points[0].y));
	transform.push_back(cv::Point2f(s_mouseCtx.points[1].x, s_mouseCtx.points[1].y));
	transform.push_back(cv::Point2f(s_mouseCtx.points[2].x, s_mouseCtx.points[2].y));
	transform.push_back(cv::Point2f(s_mouseCtx.points[3].x, s_mouseCtx.points[3].y));

	original.push_back(cv::Point2f(work.cols / 2 - 202, work.rows / 2 - 1080 / 2));
	original.push_back(cv::Point2f(work.cols / 2 - 202, work.rows / 2 + 1080 / 2));
	original.push_back(cv::Point2f(work.cols / 2 + 202, work.rows / 2 + 1080 / 2));
	original.push_back(cv::Point2f(work.cols / 2 + 202, work.rows / 2 - 1080 / 2));

	cv::Mat img;
	cv::Mat perspective_mat = cv::getPerspectiveTransform(transform, original);
	cv::warpPerspective(work, img, perspective_mat, cv::Size(work.cols, work.rows));

	cv::imshow("work", img);
	key = cv::waitKey(0);
	if (key==0x1b) return 0;

	int cnt = 1;
	while (cap.read(work)) {
		std::cout<< cnt++ <<std::endl;
		cv::warpPerspective(work, work, perspective_mat, cv::Size(work.cols, work.rows));
		rec << work;
	}

	return 0;
}


