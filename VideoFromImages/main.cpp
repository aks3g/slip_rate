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

int main(int argc, char** argv)
{
	std::ifstream list_file = std::ifstream(argv[1]);
	std::string name;
	std::getline(list_file, name);

	cv::Mat img = cv::imread(name);

	cv::VideoWriter rec;
	rec.open(std::string(argv[2]) + ".mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 100.0, img.size(), true);

	do {
		std::cout << name <<std::endl;

		img = cv::imread(name);

		rec<<img;
	} while (std::getline(list_file, name));

	return 0;
}