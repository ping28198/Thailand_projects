#include <iostream>
#include <vector>
#include <fstream>

#include <opencv2\opencv.hpp>

#include <ctime>

//#define FILE_SAVE

using namespace cv;
using namespace std;

struct BlockInfo {
	//区块序号
	int index;
	//区块位置
	Point leftTop;
	//区块宽度
	int width;
	//区块高低
	int height;
	//区块角度信息直方图
	long angleHist[8];
	//得分(有效像素占总像素数的比例)
	float score;
	//有效点数量
	long count;
	//区块方向
	int direction;
	//所有点角度
	vector<float > angleSet;
	//
	bool exist;

	BlockInfo() {
		index = 0;
		leftTop.x = 0;
		leftTop.y = 0;
		width = 0;
		height = 0;
		memset(angleHist, 0, sizeof(long) * 8);
		score = 0.0f;
		count = 0;
		direction = -1;
		exist = false;
	}
};

vector<BlockInfo > g_blockSet;

//Mat gradient;
double gradient_minVal, gradient_maxVal;
long hist[8];

struct CmpByValue {
	bool operator()(const pair<int, long>& lhs, const pair<int, long>& rhs) {
		return lhs.second > rhs.second;
	}
};

//#define WHOLE_IMG

vector<Mat>  division(Mat &image, int width, int height) {
	int m, n;
	float fm, fn;
	fm = image.rows*1.0 / height * 1.0;
	fn = image.cols*1.0 / width * 1.0;

	m = (int)fm;
	n = (int)fn;

	if (fm - (float)m > 0) {
		m++;
	}
	if (fn - (float)n > 0) {
		n++;
	}

	int index = 0;

	vector<Mat> imgOut;
	for (int j = 0; j < m; j++) {
		if (j < m - 1) {
			int i;
			for (i = 0; i < n; i++) {
				if (i < n - 1) {
					BlockInfo temBlock;
					temBlock.index = index++;
					Mat temImage(height, width, CV_8U, Scalar(0, 0, 0));
					Mat imageROI = image(Rect(i*width, j*height, temImage.cols, temImage.rows));
					addWeighted(temImage, 1.0, imageROI, 1.0, 0., temImage);

					temBlock.width = width;
					temBlock.height = height;
					temBlock.leftTop = Point(i*width, j*height);

					g_blockSet.push_back(temBlock);
					imgOut.push_back(temImage);
				}
#ifdef WHOLE_IMG
				else {
					Mat temImage(height, width, CV_8U, Scalar(0, 0, 0));
					Mat imageROI = image(Rect(i*width, j*height, image.cols - i * width - 1, temImage.rows));
					for (size_t y = 0; y < imageROI.rows; ++y) {
						unsigned char* ps = imageROI.ptr<unsigned char>(y);
						unsigned char* pp = temImage.ptr<unsigned char>(y);
						for (size_t x = 0; x < imageROI.cols; ++x) {
							pp[x] = ps[x];
						}
					}
					imgOut.push_back(temImage);
				}
#endif
			}
		}
#ifdef WHOLE_IMG
		else {
			int i;
			for (i = 0; i < n; i++) {
				if (i < n - 1) {
					Mat temImage(height, width, CV_8U, Scalar(0, 0, 0));
					Mat imageROI = image(Rect(i*width, j*height, temImage.cols, image.rows - j * height - 1));
					for (size_t y = 0; y < imageROI.rows; ++y) {
						unsigned char* ps = imageROI.ptr<unsigned char>(y);
						unsigned char* pp = temImage.ptr<unsigned char>(y);
						for (size_t x = 0; x < imageROI.cols; ++x) {
							pp[x] = ps[x];
						}
					}
					imgOut.push_back(temImage);
				}
				else {
					Mat temImage(height, width, CV_8U, Scalar(0, 0, 0));
					Mat imageROI = image(Rect(i*width, j*height, image.cols - i * width - 1, image.rows - j * height - 1));
					for (size_t y = 0; y < imageROI.rows; ++y) {
						unsigned char* ps = imageROI.ptr<unsigned char>(y);
						unsigned char* pp = temImage.ptr<unsigned char>(y);
						for (size_t x = 0; x < imageROI.cols; ++x) {
							pp[x] = ps[x];
						}
					}
					imgOut.push_back(temImage);
				}
			}
		}
#endif
	}
	return imgOut;
}


///保存分割后的区块图像
void savedivisionfiles(vector<Mat> imgDiv) {
	int index = 0;
	for(int i=0;i<imgDiv.size();i++)
	{
		string prefilename = "Division - ";
		char str[10];
		sprintf_s(str, "%d", index);
		/*_itoa(index, str, 10);*/
		string indexstr(str);
		string filename = "..//results//cvtest//" + prefilename + indexstr + ".bmp";
		imwrite(filename, imgDiv[i]);
		++index;
	}
}

///绘制直方图
Mat drawHistgram() {
	Mat histgram(300, 400, CV_8U);
	histgram = Mat::zeros(histgram.size(), CV_8U);
	long maxVal = 0;
	for (int i = 0; i < 8; i++) {
		if (hist[i] > maxVal)
			maxVal = hist[i];
	}

	if (maxVal != 0)
		for (int i = 0; i < 8; i++) {
			long val = hist[i];
			long intensity = cvRound((val * 300 / maxVal)*0.9);
			rectangle(histgram, Point(i * 50 + 5, 300 - intensity),
				Point((i * 50) + 50 - 5, 300),
				CV_RGB(255, 255, 255), 3);
		}

	return histgram;
}

///将各个区块计算Sobel,并得到每个像素的方向信息,进行统计存储
void sobelAndCalc(Mat &img) {
	static int idx = 0;

	///不纳入统计的梯度强度阈值比例
	const float GRADIANT_THRESHOLD = 0.4f;

#ifdef FILE_SAVE
	char index[10];
	itoa(i, index, 10);
	string indexstr(index);
#endif

	///纳入统计的像素值的数量
	long pixelCnt = 0;

	Mat gx(img.rows, img.cols, CV_32F);
	Mat gy(img.rows, img.cols, CV_32F);
	Sobel(img, gx, CV_32F, 1, 0, -1);
	Sobel(img, gy, CV_32F, 0, 1, -1);

	///初始化角度直方图
	memset(hist, 0, sizeof(long) * 8);

	Mat magnitude(gx.rows, gx.cols, CV_32F);
	Mat angleMat(gx.rows, gx.cols, CV_32F);
	cartToPolar(gx, gy, magnitude, angleMat);

	///只统计梯度强度大于最大梯度强度40%的像素点,将低于阈值的值置为0
	threshold(magnitude, magnitude, gradient_maxVal*GRADIANT_THRESHOLD, gradient_maxVal, THRESH_TOZERO);

#ifdef FILE_SAVE
	string filename0 = "..//results//cvtest//Magnitude - " + indexstr + ".bmp";
	imwrite(filename0, magnitude);
#endif

	vector<float > angleSet;

	///遍历当前区块所有像素点
	for (size_t y = 0; y < gx.rows; ++y) {
		float* pa = angleMat.ptr<float>(y);
		float* pm = magnitude.ptr<float>(y);
		for (size_t x = 0; x < gx.cols*gx.channels(); ++x) {


			float angle = pa[x];
			///将弧度限制在0~PI的范围内
			angle = angle > CV_PI ? angle - CV_PI : angle;

			///当前像素点的梯度强度
			float curGradient = pm[x];

			///如果之前未被置为0
			if (curGradient > 0) {
				++pixelCnt;
				///将弧度值转换为角度
				float angle_new = 180.0 * angle / CV_PI;
				angleSet.push_back(angle_new);

				if (angle >= 0 && angle < 0.125*CV_PI) {
					hist[0]++;
				}
				else if (angle >= 0.125*CV_PI && angle < 0.25*CV_PI) {
					hist[1]++;
				}
				else if (angle >= 0.25*CV_PI && angle < 0.375*CV_PI) {
					hist[2]++;
				}
				else if (angle >= 0.375*CV_PI && angle < 0.5*CV_PI) {
					hist[3]++;
				}
				else if (angle >= 0.5*CV_PI && angle < 0.625*CV_PI) {
					hist[4]++;
				}
				else if (angle >= 0.625*CV_PI && angle < 0.75*CV_PI) {
					hist[5]++;
				}
				else if (angle >= 0.75*CV_PI && angle < 0.875*CV_PI) {
					hist[6]++;
				}
				else if (angle >= 0.875*CV_PI && angle < CV_PI) {
					hist[7]++;
				}

			}
		}
	}


#ifdef FILE_SAVE
	string filename1 = "..//results//cvtest//Histgram - " + indexstr + ".bmp";
	imwrite(filename1, drawHistgram());
#endif

	///存储此区块信息
	g_blockSet[idx].count = pixelCnt;
	g_blockSet[idx].angleSet = angleSet;
	for (int j = 0; j < 8; ++j) {
		g_blockSet[idx].angleHist[j] = hist[j];
	}
	idx++;

}

///获得直方图中值最高的项,并返回其与相邻项的和
long getMaxWithAdj(long* hist, int &index) {
	long max = 0;
	int maxindex = 0;
	for (int i = 0; i < 8; ++i) {
		if (hist[i] > max) {
			max = hist[i];
			maxindex = i;
		}
	}
	index = maxindex;
	return hist[(maxindex - 1) % 8] + hist[maxindex] + hist[(maxindex + 1) % 8];

	//long *newHist = new long[8];

	//for (int i = 0; i < 8; ++i) {
	//  newHist[i] = hist[(i - 1) % 8] + hist[i] + hist[(i + 1) % 8];
	//}

	//long max = 0;
	//int maxindex = 0;
	//for (int i = 0; i < 8; ++i) {
	//  if (newHist[i] > max) {
	//      max = newHist[i];
	//      maxindex = i;
	//  }
	//}

	//index = maxindex;

	//long val = newHist[maxindex];

	//if (newHist) {
	//  delete newHist;
	//}
	//newHist = NULL;

	//return val;
}

///计算所有区块的分数
void blockSetTravel() {
	///遍历所有区块
	for (vector<BlockInfo >::iterator pb = g_blockSet.begin(); pb < g_blockSet.end(); ++pb) {
		int maxIndex = 0;
		///获得最高的项的和
		long max = getMaxWithAdj(pb->angleHist, maxIndex);
		pb->direction = maxIndex;
		if (pb->count > pb->height*pb->width / 20.0) {
			if (pb->count > 0)
				///该区块的得分
				pb->score = max * 1.0 / pb->count*1.0;
			else
				pb->score = 0;
		}
		else {
			pb->score = 0;
		}
	}
}

///计算所有区块的总体方向,即整个条形码的大概方向范围在PI的哪一个1/8等分中
int getDirection() {
	int direction[8];
	memset(direction, 0, sizeof(int) * 8);
	///认为非干扰区块的阈值
	const float SCORE_THRESHOLD = 0.65;
	for (int i = 0; i < g_blockSet.size();i++) {
		if (g_blockSet[i].score >= SCORE_THRESHOLD) {
			direction[g_blockSet[i].direction]++;
		}
	}

	int max = 0;
	int maxIndex;
	for (int i = 0; i < 8; ++i) {
		if (direction[i] > max) {
			max = direction[i];
			maxIndex = i;
		}
	}

	return maxIndex;
}

/// 得到该图片中最有可能的条码角度(可以认为是getDirection函数的求精版本)
int getAngle() {
	map<int, long> set;
	for(auto block : g_blockSet) {
		if (block.exist) {
			for(auto var : block.angleSet) {
				/// 当图片分辨率较大时,1°将造成条码左右区域多个像素的偏差,故此处以0.5°为步进统计
				if (var - (int)var > 0.5)
					var = (int)var*1.0 + 0.5;
				else
					var = (int)var;
				set[var]++;
			}
		}
	}

	/// 将角度信息按照出现的次数进行排序
	vector<pair<int, long > > vec(set.begin(), set.end());
	sort(vec.begin(), vec.end(), CmpByValue());

	/// 返回出现次数最多的那个角度
	return vec.begin()->first;
}

///将指定方向的区块根据梯度强度强弱或深或浅绘制
Mat drawBlockSet(Size size, int direction) {
	Mat result(size, CV_8U, Scalar(0, 0, 0));
	for (vector<BlockInfo >::iterator pvar = g_blockSet.begin(); pvar < g_blockSet.end(); ++pvar) {
		if (pvar->score >= 0.65 && pvar->direction == direction) {
			pvar->exist = true;
			rectangle(result, Rect(pvar->leftTop.x, pvar->leftTop.y, pvar->width, pvar->height), Scalar(255 * pvar->score), -1);
		}
		else {
			pvar->exist = false;
		}
	}
	return result;
}

/// TEST:根据FFT获得角度信息并旋转(可以粗略实现矫正,但是其精度较低,并且要求图片中条码所占比例较大,所以效果并不明显)
void FFT2Rotate(Mat &img) {
	Mat after_guussian;
	GaussianBlur(img, after_guussian, Size(5, 5), 0);

	Mat imageSobelX, imageSobelY, imageSobelOut;
	//水平和垂直方向灰度图像的梯度和,使用Sobel算子    
	Mat imageX16S, imageY16S;
	Sobel(after_guussian, imageX16S, CV_16S, 1, 0, 3, 1, 0, 4);
	Sobel(after_guussian, imageY16S, CV_16S, 0, 1, 3, 1, 0, 4);
	convertScaleAbs(imageX16S, imageSobelX, 1, 0);
	convertScaleAbs(imageY16S, imageSobelY, 1, 0);
	imageSobelOut = imageSobelX + imageSobelY;
	//imshow("XY方向梯度和", imageSobelOut);

	Mat srcImg = imageSobelOut;
	//宽高扩充，非必须，特定的宽高可以提高傅里叶运算效率  
	Mat padded;
	int opWidth = getOptimalDFTSize(srcImg.rows);
	int opHeight = getOptimalDFTSize(srcImg.cols);
	copyMakeBorder(srcImg, padded, 0, opWidth - srcImg.rows, 0, opHeight - srcImg.cols, BORDER_CONSTANT, Scalar::all(0));
	Mat planes[] = { Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F) };
	Mat comImg;

	//通道融合，融合成一个2通道的图像  
	merge(planes, 2, comImg);
	dft(comImg, comImg);
	split(comImg, planes);
	//magnitude(planes[0], planes[1], planes[0]);
	Mat no_use;
	cartToPolar(planes[0], planes[1], planes[0], no_use);
	Mat magMat = planes[0];
	magMat += Scalar::all(1);
	log(magMat, magMat);     //对数变换，方便显示 
	magMat = magMat(Rect(0, 0, magMat.cols & -2, magMat.rows & -2));

	int cx = magMat.cols / 2;
	int cy = magMat.rows / 2;
	Mat q0(magMat, Rect(0, 0, cx, cy));
	Mat q1(magMat, Rect(0, cy, cx, cy));
	Mat q2(magMat, Rect(cx, cy, cx, cy));
	Mat q3(magMat, Rect(cx, 0, cx, cy));
	Mat tmp;
	q0.copyTo(tmp);
	q2.copyTo(q0);
	tmp.copyTo(q2);
	q1.copyTo(tmp);
	q3.copyTo(q1);
	tmp.copyTo(q3);
	normalize(magMat, magMat, 0, 1, CV_MINMAX);
	Mat magImg(magMat.size(), CV_8UC1);
	magMat.convertTo(magImg, CV_8UC1, 255, 0);
	//namedWindow("傅立叶频谱", 0);
	//cvResizeWindow("傅立叶频谱", 800, 600);
	//imshow("傅立叶频谱", magImg);

	//HoughLines查找傅立叶频谱的直线，该直线跟原图的一维码方向相互垂直
	threshold(magImg, magImg, 180, 255, CV_THRESH_BINARY);
	//namedWindow("二值化", 0);
	//cvResizeWindow("二值化", 800, 600);
	//imshow("二值化", magImg);

	//Mat element0 = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	//erode(magImg, magImg, element0, Point(-1, -1), 1);
	//imshow("Erode", magImg);

	vector<Vec2f> lines;
	float pi180 = (float)CV_PI / 360;
	Mat linImg(magImg.size(), CV_8UC3);
	float theta_global = 0;
	HoughLines(magImg, lines, 1, pi180, 180, 0, 0);
	int numLines = lines.size();

	//for (int l = 0; l < numLines; l++) {
	//  float rho = lines[l][0];
	//  float theta = lines[l][1];
	//  float aa = (theta / CV_PI) * 180;
	//  Point pt1, pt2;
	//  double a = cos(theta), b = sin(theta);
	//  double x0 = a*rho, y0 = b*rho;
	//  pt1.x = cvRound(x0 + 5000 * (-b));
	//  pt1.y = cvRound(y0 + 5000 * (a));
	//  pt2.x = cvRound(x0 - 5000 * (-b));
	//  pt2.y = cvRound(y0 - 5000 * (a));
	//  line(linImg, pt1, pt2, Scalar(255, 0, 0), 3, 8, 0);
	//}

	for (int l = 0; l < 1; l++) {
		float rho = lines[l][0];
		float theta = lines[l][1];
		float aa = (theta / CV_PI) * 180;
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		pt1.x = cvRound(x0 + 5000 * (-b));
		pt1.y = cvRound(y0 + 5000 * (a));
		pt2.x = cvRound(x0 - 5000 * (-b));
		pt2.y = cvRound(y0 - 5000 * (a));
		line(linImg, pt1, pt2, Scalar(255, 0, 0), 3, 8, 0);
	}

	theta_global = lines[0][1];


	//vector<Vec4i> lines2;
	//HoughLinesP(magImg, lines2, 1, CV_PI / 180, 10, 3, 15);
	//for (size_t i = 0; i < lines2.size(); i++) {
	//  line(linImg, Point(lines2[i][0], lines2[i][1]),
	//      Point(lines2[i][2], lines2[i][3]), Scalar(0, 0, 255), 3, 8);
	//  //cout << "X1: " << lines[i][0] << " \tY1: " << lines[i][1] << " ----> " << " X2: " << lines[i][2] << " \tY2:" << lines[i][3] << endl;
	//}

	//if (lines2.size() >= 2) {
	//  theta_avg = atan((lines2[3][3] - lines2[3][1])*1.0 / (lines2[3][2] - lines2[3][0])*1.0);
	//}

	//namedWindow("Hough直线", 0);
	//cvResizeWindow("Hough直线", 800, 600);
	//imshow("Hough直线", linImg);

	//校正角度计算
	float angelD = 180 * theta_global / CV_PI - 90;
	Point center(img.cols / 2, img.rows / 2);
	Mat rotMat = getRotationMatrix2D(center, angelD, 1.0);
	Mat imageSource = Mat::ones(img.size(), CV_8UC3);
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//仿射变换校正图像  
																				  //namedWindow("角度校正", 0);
																				  //cvResizeWindow("角度校正", 800, 600);
																				  //imshow("角度校正", imageSource);
}

/// TEST:根据hough变换得到角度信息并变换(较为失败,一般通用性较差)
void Hough2Rotate(Mat &img) {
	Mat after_canny;
	Canny(img, after_canny, 200, 180, 3);
	imshow("Canny", after_canny);

	Mat img_cpy;
	after_canny.copyTo(img_cpy);

	float theta_avg = 0;
	vector<Vec2f> lines;
	HoughLines(after_canny, lines, 1, CV_PI / 360, 200);
	size_t size = lines.size();
	size = size > 5 ? 5 : size;
	int cnt = 0;
	for (size_t i = 0; i < size; i++) {
		float rho = lines[i][0];
		float theta = lines[i][1];
		if (theta < CV_PI / 2) {
			theta_avg += theta;
			++cnt;
		}
		Point pt1, pt2;
		double a = cos(theta);
		double b = sin(theta);
		double x0 = rho * a;
		double y0 = rho * b;
		pt1.x = cvRound(x0 + 5000 * (-b));
		pt1.y = cvRound(y0 + 5000 * a);
		pt2.x = cvRound(x0 - 5000 * (-b));
		pt2.y = cvRound(y0 - 5000 * a);
		line(img_cpy, pt1, pt2, Scalar(255, 255, 255), 1, CV_AA);
	}
	imshow("HoughLine", img_cpy);

	theta_avg /= cnt;

	float angelD = 0;
	if (theta_avg - 0 > 1e-3) {
		angelD = 180 * theta_avg / CV_PI - 90;
	}
	else {
		angelD = 0;
	}
	Point center(img.cols / 2, img.rows / 2);
	Mat rotMat = getRotationMatrix2D(center, angelD, 1.0);
	Mat imageSource = Mat::ones(img.size(), CV_8UC3);
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//仿射变换校正图像  
	imshow("角度校正", imageSource);
}

/// INUSE:通过计算得到的角度进行矫正
void Angle2Rotate(Mat &img, float angle) {
	Point center(img.cols / 2, img.rows / 2);
	Mat rotMat = getRotationMatrix2D(center, angle, 1.0);
	Mat imageSource = Mat::ones(img.size(), CV_8UC3);
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//仿射变换校正图像  
}

int main1(int argc, char *argv[]) {
	/// 计时
	clock_t begin, end;

	/// 区块尺寸(对结果影响较大,可以进行多次尝试)
	int block_size = 70;

	begin = clock();
	const string IMG_PATH = "../imgs/15633495854075477.jpg";

	Mat src = imread(IMG_PATH, CV_LOAD_IMAGE_COLOR);
	if (!src.data) {
		cout << "Read image error" << endl;
		return -1;
	}

	string indexstr = "05";

	///将图片格式变为灰度图像
	cvtColor(src, src, CV_RGB2GRAY);

	///make a copy
	Mat src_cpy;
	src.copyTo(src_cpy);

	///对图像进行一个9×9的均值滤波,以实现将图像细节部分的纹理信息屏蔽
	blur(src_cpy, src_cpy, Size(9, 9));

	///对整幅图像进行Sobel求两个方向上的梯度
	Mat gx(src_cpy.rows, src_cpy.cols, CV_32F);
	Mat gy(src_cpy.rows, src_cpy.cols, CV_32F);
	Sobel(src_cpy, gx, CV_32F, 1, 0, -1);
	Sobel(src_cpy, gy, CV_32F, 0, 1, -1);

	///根据两个方向上的梯度信息,获得每个像素点的弧度和每个像素点的梯度强度
	Mat magnitude(gx.rows, gx.cols, CV_32F);
	Mat anglemat(gx.rows, gx.cols, CV_32F);
	cartToPolar(gx, gy, magnitude, anglemat);

	///得到梯度强度的最大最小值
	minMaxIdx(magnitude, &gradient_minVal, &gradient_maxVal);

	///将图像分割区块
	vector<Mat> imgDiv;
	imgDiv = division(src, block_size, block_size);

	///遍历每个区块,求得所有区块的有效像素数量以及方向信息
	for (vector<Mat >::iterator p = imgDiv.begin(); p < imgDiv.end(); ++p) {
		sobelAndCalc(*p);
	}

	blockSetTravel();

	/// 计算大概的整体方向
	int dir = getDirection();
	/// 将得到的该方向的区块根据梯度强度绘制不同灰度级强度的色块
	Mat region = drawBlockSet(src.size(), dir);

	namedWindow("Region", 0);
	imshow("Region", region);

#ifdef FILE_SAVE
	savedivisionfiles(imgDiv);
#endif

	/// 寻找最大连通域
	vector<vector<Point>> con;
	findContours(region, con, RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	double max_area = 0;
	int max_index = 0;

	size_t i = 0;
	for (; i < con.size(); ++i) {
		if (contourArea(con.at(i)) > max_area) {
			max_area = contourArea(con.at(i));
			max_index = i;
		}
	}

	RotatedRect rect = minAreaRect(con.at(max_index));
	Point2f vertices[4];
	rect.points(vertices);

	//cvtColor(src, src, CV_GRAY2RGB);

	/// 将最大连通域绘制出来
	for (int i = 0; i < 4; i++)
		line(src, vertices[i], vertices[(i + 1) % 4], Scalar(0, 0, 255), 4);

	namedWindow("Lined", 0);
	imshow("Lined", src);

	/// 获得该区域的最小正的外接矩形
	int x_min = 50000, y_min = 50000;
	int x_max = 0, y_max = 0;
	for (int i = 0; i < 4; i++) {
		if (vertices[i].x < x_min) {
			x_min = vertices[i].x;
		}
		if (vertices[i].x > x_max) {
			x_max = vertices[i].x;
		}
		if (vertices[i].y < y_min) {
			y_min = vertices[i].y;
		}
		if (vertices[i].y > y_max) {
			y_max = vertices[i].y;
		}
	}

	int select_top_index = 0;
	for (int i = 0; i < 4; i++) {
		if ((int)vertices[i].y == y_min) {
			select_top_index = i;
		}
	}

	//int select_left_index = 0;
	//for (int i = 0; i < 4; i++) {
	//  if ((int)vertices[i].x == x_min) {
	//      select_left_index = i;
	//  }
	//}

	/// 计算正外接矩形的position&size
	int new_width = x_max - x_min + 1;
	int new_height = y_max - y_min + 1;
	int x_min_new = x_min < 0 ? 0 : x_min;
	int y_min_new = y_min < 0 ? 0 : y_min;

	if (x_min_new + new_width > src.cols) {
		new_width = src.cols - x_min_new;
	}

	if (y_min_new + new_height > src.rows) {
		new_height = src.rows - y_min_new;
	}

	Rect rect_clip(x_min_new, y_min_new, new_width, new_height);
	Mat region_clip = src(rect_clip);

	/// 将裁减之后的正的最小外接矩形显示
	namedWindow("Clip", 0);
	imshow("Clip", region_clip);

	/// 计算区域的带角度的外接矩形的顶部点在裁剪之后的图片中的坐标
	Point2d top_point(vertices[select_top_index]);
	top_point.x = (int)top_point.x - x_min;
	top_point.y = (int)top_point.y - y_min;

	/// 计算区域的带角度的外接矩形的长边与短边的长度
	double distance1 = sqrt((vertices[1].x - vertices[0].x)*(vertices[1].x - vertices[0].x) + (vertices[1].y - vertices[0].y)*(vertices[1].y - vertices[0].y));
	double distance2 = sqrt((vertices[2].x - vertices[1].x)*(vertices[2].x - vertices[1].x) + (vertices[2].y - vertices[1].y)*(vertices[2].y - vertices[1].y));

	/// 需要旋转的角度
	double angle = 0;
	/// 旋转之后的图像的尺寸
	double width_rotate = 0, height_rotate = 0;

	/// 判断哪一个是长边,然后计算需要旋转的角度和新图像的尺寸(因为最终结果需要以长边长度为宽度[即水平放置的条码才是需要的结果])
	if (distance1 > distance2) {
		angle = atan2(vertices[1].y - vertices[0].y, vertices[1].x - vertices[0].x);
		width_rotate = distance1;
		height_rotate = distance2;
	}
	else {
		angle = atan2(vertices[2].y - vertices[1].y, vertices[2].x - vertices[1].x);
		width_rotate = distance2;
		height_rotate = distance1;
	}

	/// 将图像转换为角度
	angle = angle * 180.0 / CV_PI;

	Mat rotateImg;
	region_clip.copyTo(rotateImg);

	/// 如果该条码原本就接近水平,则不需要旋转
	if ((int)angle != 180 && (int)angle != 0) {
		/// 以裁剪后(未旋转)的图像的中心点旋转
		Point2f center = Point2f(region_clip.cols / 2, region_clip.rows / 2);
		/// 不进行缩放
		double scale = 1.0;

		/// 计算得到旋转矩阵(该矩阵不包括平移的参数[即在使用该矩阵进行旋转的时候,不会进行平移])
		Mat rotateMat;
		rotateMat = getRotationMatrix2D(center, angle, scale);

		/// 将旋转矩阵加入平移的参数(以下步骤即将条码区域角点移动到新的图片的左上角)
		double* pr = rotateMat.ptr<double>(0);
		double M00 = pr[0];
		double M01 = pr[1];
		double M02 = pr[2];
		pr = rotateMat.ptr<double>(1);
		double M10 = pr[0];
		double M11 = pr[1];
		double M12 = pr[2];

		int torig_x = top_point.x;
		int torig_y = top_point.y;

		top_point.x = torig_x * 1.0 * M00 + torig_y * 1.0*M10 + M02;
		top_point.y = torig_x * 1.0 * M01 + torig_y * 1.0*M11 + M12;

		pr = rotateMat.ptr<double>(0);
		pr[2] += (width_rotate - top_point.x);
		pr = rotateMat.ptr<double>(1);
		pr[2] += (-region_clip.rows / 2) + (height_rotate / 2);

		warpAffine(region_clip, rotateImg, rotateMat, Size(width_rotate, height_rotate));

		Mat temp4Disp(rotateImg);
		namedWindow("Rotated0", 0);
		imshow("Rotated0", temp4Disp);
	}

	/// 以上旋转只是通过区域外界矩形的角度进行旋转
	/// 以下旋转是根据统计得到的角度进行二次旋转(提高精度)
	int result_angle = getAngle();
	result_angle = abs(result_angle - 180);
	float new_angle = -(result_angle - fabs(angle));

	if (fabs(new_angle) >= 0.5) {
		Angle2Rotate(rotateImg, new_angle);
	}

	namedWindow("Rotated", 0);
	imshow("Rotated", rotateImg);

	waitKey(0);

	return 0;
}
