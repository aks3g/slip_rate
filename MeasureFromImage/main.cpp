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
#define PI 3.14159265359
namespace {
	const double coeff_pix2mm = 540.0f / 404.0f;

	enum Mini4wdPos
	{
		Mini4wdInUnknown,
		Mini4wdInStraight,
		Mini4wdInRightSideCorner,
		Mini4wdInLeftSideCorner
	};

	void _collectLedIndicator(cv::Mat& img, std::vector<cv::Point> &points, const uint8_t thr_b, const uint8_t thr_g, const uint8_t thr_r)
	{
		int cnt = 0;

		//J Green
		for (size_t y = 0; y < img.size().height; ++y) {
			for (size_t x = 0; x < img.size().width; ++x) {
				cv::Vec3b p = img.at<cv::Vec3b>(cv::Point(x, y));
//				cv::Vec3b m = mask.at<cv::Vec3b>(cv::Point((int)x, (int)y));
//				if (p[1] > thr_g && p[0] < thr_b && p[2] < thr_r) {
				if (p[1] > thr_g && p[2] < thr_r) {
					points.push_back(cv::Point(x,y));
#if 1
					img.at<cv::Vec3b>(cv::Point(x, y)) = cv::Vec3b(0, 0, 255);
#endif
				}
			}
		}

		return;
	}

	enum Mini4wdPos _check_mini4wd_pos(cv::Mat &img, std::vector<cv::Point>& points)
	{
//		cv::Mat mask = cv::imread("mask.png",1);
		_collectLedIndicator(img, points, 50, 50, 50);

		cv::line(img, cv::Point(img.cols / 2 - 404 / 2, 0), cv::Point(img.cols / 2 - 404 / 2, img.rows), cv::Scalar(0, 0, 255), 1);
		cv::line(img, cv::Point(img.cols / 2 + 404 / 2, 0), cv::Point(img.cols / 2 + 404 / 2, img.rows), cv::Scalar(0, 0, 255), 1);
		cv::line(img, cv::Point(0, img.rows/2), cv::Point(img.cols, img.rows / 2), cv::Scalar(0, 0, 255), 1);

		int straight_count = 0;
		int r_corner_count = 0;
		int l_corner_count = 0;

		for (const auto& e : points) {
			if ((img.cols / 2 - 404 / 2) < e.x && e.x < img.cols / 2 + 404 / 2) {
				straight_count++;
			}
			else if (e.x <= (img.cols / 2 - 404 / 2)){
				l_corner_count++;
			}
			else {
				r_corner_count++;
			}
		}

		if (r_corner_count == 0 && l_corner_count == 0 && straight_count != 0) {
			return Mini4wdInStraight;
		}
		else if (r_corner_count == 0 && l_corner_count != 0 && straight_count == 0) {
			return Mini4wdInLeftSideCorner;
		}
		else if (r_corner_count != 0 && l_corner_count == 0 && straight_count == 0) {
			return Mini4wdInRightSideCorner;
		}
		else {
			return Mini4wdInUnknown;
		}
	}

	int _measure_straight_in_pix(std::vector<cv::Point> points)
	{
		//J LED‚Ì‰æ‘œã‚Å‚ÌƒTƒCƒY‚ğ3x3‚Æ‰¼’è‚·‚é 
		int r = 3;

		int min_x=1000000;
		int max_x=-1;
		for (const auto& e : points) {
			if (e.x > max_x) max_x = e.x;
			if (e.x < min_x) min_x = e.x;
		}

		return max_x - min_x - 2*r;
	}

	bool _meature_corner(std::vector<cv::Point> points, cv::Point center, cv::Mat& img, double& theta, int& radius) {

		int r = 0;
		for (const auto& e : points) {
			cv::Point vec = e - center;
			r += (int)sqrt((double)(vec.x * vec.x + vec.y * vec.y));
		}
		r /= points.size();
		double offset = (atan2(3, r) * 2) * 360.0 / (2 * PI);

		//J „’è‚³‚ê‚½‰ñ“]‰~ã‚Ì“]‚¾‚¯‚ğ’Šo
		//J 12‚Ì•ûŒü‚ğ0‚Æ‚µ‚½Še“_‚ÌŠp“x‚ğŒŸo‚·‚é
		double theta_min = 1000;
		double theta_max = -1000;

		for (const auto& e : points) {
			cv::Point vec = e - center;
			cv::Point org = cv::Point(0, -r);
			if ((int)sqrt((double)(vec.x * vec.x + vec.y * vec.y)) == r) {
				img.at<cv::Vec3b>(e) = cv::Vec3b(255, 0, 0);

				double _cos = (vec.x * org.x + vec.y * org.y) / (sqrt((double)(vec.x * vec.x + vec.y * vec.y)) * sqrt((double)(org.x * org.x + org.y * org.y)));
				double theta_degree = acos(_cos) * 360.0 / (2 * PI);
				if (theta_degree > theta_max) theta_max = theta_degree;
				if (theta_degree < theta_min) theta_min = theta_degree;
			}
		}

		theta = theta_max - theta_min - offset;
		radius = r;
#if 0
		cv::circle(img, center, r, cv::Scalar(0, 0, 255), 1);

		char info[200];
		snprintf(info, sizeof(info), "Radius = %d[pix], Theta = %lf[degree]", radius, theta);

		cv::putText(img, info, cv::Point(100, 980), cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

		cv::imshow("test", img);
		cv::waitKey(0);
#endif
		return 0;
	}


	bool _meature_right_side_corner(std::vector<cv::Point> points, cv::Mat& img, double& theta, int& radius)
	{
		//J ‰ñ“]’†S‚©‚ç‚Ì‹——£‚Ì•½‹Ï‚ğZo‚·‚é
		cv::Point center = cv::Point(img.size().width / 2 + 404 / 2, img.size().height / 2);
		return _meature_corner(points, center, img, theta, radius);
	}

	bool _meature_left_side_corner(std::vector<cv::Point> points, cv::Mat& img, double& theta, int& radius)
	{
		//J ‰ñ“]’†S‚©‚ç‚Ì‹——£‚Ì•½‹Ï‚ğZo‚·‚é
		cv::Point center = cv::Point(img.size().width / 2 - 404 / 2, img.size().height / 2);
		return _meature_corner(points, center, img, theta, radius);
	}

}


int main(int argc, char** argv)
{
	//J “ü—Í“®‰æ‚Æo—Íæ‚Ì€”õ
	cv::Mat img = cv::imread(std::string(argv[1]), 1);

	std::vector<cv::Point> points;
	enum Mini4wdPos pos =_check_mini4wd_pos(img, points);
	if (pos == Mini4wdInUnknown) {
		std::cout << argv[1] << '\t' << 'u' << std::endl;
	}
	else if (pos == Mini4wdInStraight) {
		int width = _measure_straight_in_pix(points);
		std::cout << argv[1] << '\t' << 's' << '\t' << width << std::endl;
	}
	else if (pos == Mini4wdInLeftSideCorner) {
		double theta=0;
		int radius = 0;
		_meature_left_side_corner(points, img, theta, radius);
		std::cout << argv[1] << '\t' << 'c' << '\t' << radius << '\t' << theta << std::endl;
	}
	else if (pos == Mini4wdInRightSideCorner) {
		double theta = 0;
		int radius = 0;
		_meature_right_side_corner(points, img, theta, radius);
		std::cout << argv[1] << '\t' << 'c' << '\t' << radius << '\t' << theta << std::endl;
	}

	return 0;
}
