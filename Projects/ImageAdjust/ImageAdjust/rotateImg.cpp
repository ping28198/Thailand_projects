#include <iostream>
#include <vector>
#include <fstream>

#include <opencv2\opencv.hpp>

#include <ctime>

//#define FILE_SAVE

using namespace cv;
using namespace std;

struct BlockInfo {
	//�������
	int index;
	//����λ��
	Point leftTop;
	//������
	int width;
	//����ߵ�
	int height;
	//����Ƕ���Ϣֱ��ͼ
	long angleHist[8];
	//�÷�(��Ч����ռ���������ı���)
	float score;
	//��Ч������
	long count;
	//���鷽��
	int direction;
	//���е�Ƕ�
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


///����ָ�������ͼ��
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

///����ֱ��ͼ
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

///�������������Sobel,���õ�ÿ�����صķ�����Ϣ,����ͳ�ƴ洢
void sobelAndCalc(Mat &img) {
	static int idx = 0;

	///������ͳ�Ƶ��ݶ�ǿ����ֵ����
	const float GRADIANT_THRESHOLD = 0.4f;

#ifdef FILE_SAVE
	char index[10];
	itoa(i, index, 10);
	string indexstr(index);
#endif

	///����ͳ�Ƶ�����ֵ������
	long pixelCnt = 0;

	Mat gx(img.rows, img.cols, CV_32F);
	Mat gy(img.rows, img.cols, CV_32F);
	Sobel(img, gx, CV_32F, 1, 0, -1);
	Sobel(img, gy, CV_32F, 0, 1, -1);

	///��ʼ���Ƕ�ֱ��ͼ
	memset(hist, 0, sizeof(long) * 8);

	Mat magnitude(gx.rows, gx.cols, CV_32F);
	Mat angleMat(gx.rows, gx.cols, CV_32F);
	cartToPolar(gx, gy, magnitude, angleMat);

	///ֻͳ���ݶ�ǿ�ȴ�������ݶ�ǿ��40%�����ص�,��������ֵ��ֵ��Ϊ0
	threshold(magnitude, magnitude, gradient_maxVal*GRADIANT_THRESHOLD, gradient_maxVal, THRESH_TOZERO);

#ifdef FILE_SAVE
	string filename0 = "..//results//cvtest//Magnitude - " + indexstr + ".bmp";
	imwrite(filename0, magnitude);
#endif

	vector<float > angleSet;

	///������ǰ�����������ص�
	for (size_t y = 0; y < gx.rows; ++y) {
		float* pa = angleMat.ptr<float>(y);
		float* pm = magnitude.ptr<float>(y);
		for (size_t x = 0; x < gx.cols*gx.channels(); ++x) {


			float angle = pa[x];
			///������������0~PI�ķ�Χ��
			angle = angle > CV_PI ? angle - CV_PI : angle;

			///��ǰ���ص���ݶ�ǿ��
			float curGradient = pm[x];

			///���֮ǰδ����Ϊ0
			if (curGradient > 0) {
				++pixelCnt;
				///������ֵת��Ϊ�Ƕ�
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

	///�洢��������Ϣ
	g_blockSet[idx].count = pixelCnt;
	g_blockSet[idx].angleSet = angleSet;
	for (int j = 0; j < 8; ++j) {
		g_blockSet[idx].angleHist[j] = hist[j];
	}
	idx++;

}

///���ֱ��ͼ��ֵ��ߵ���,����������������ĺ�
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

///������������ķ���
void blockSetTravel() {
	///������������
	for (vector<BlockInfo >::iterator pb = g_blockSet.begin(); pb < g_blockSet.end(); ++pb) {
		int maxIndex = 0;
		///�����ߵ���ĺ�
		long max = getMaxWithAdj(pb->angleHist, maxIndex);
		pb->direction = maxIndex;
		if (pb->count > pb->height*pb->width / 20.0) {
			if (pb->count > 0)
				///������ĵ÷�
				pb->score = max * 1.0 / pb->count*1.0;
			else
				pb->score = 0;
		}
		else {
			pb->score = 0;
		}
	}
}

///����������������巽��,������������Ĵ�ŷ���Χ��PI����һ��1/8�ȷ���
int getDirection() {
	int direction[8];
	memset(direction, 0, sizeof(int) * 8);
	///��Ϊ�Ǹ����������ֵ
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

/// �õ���ͼƬ�����п��ܵ�����Ƕ�(������Ϊ��getDirection�������󾫰汾)
int getAngle() {
	map<int, long> set;
	for(auto block : g_blockSet) {
		if (block.exist) {
			for(auto var : block.angleSet) {
				/// ��ͼƬ�ֱ��ʽϴ�ʱ,1�㽫��������������������ص�ƫ��,�ʴ˴���0.5��Ϊ����ͳ��
				if (var - (int)var > 0.5)
					var = (int)var*1.0 + 0.5;
				else
					var = (int)var;
				set[var]++;
			}
		}
	}

	/// ���Ƕ���Ϣ���ճ��ֵĴ�����������
	vector<pair<int, long > > vec(set.begin(), set.end());
	sort(vec.begin(), vec.end(), CmpByValue());

	/// ���س��ִ��������Ǹ��Ƕ�
	return vec.begin()->first;
}

///��ָ���������������ݶ�ǿ��ǿ�������ǳ����
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

/// TEST:����FFT��ýǶ���Ϣ����ת(���Դ���ʵ�ֽ���,�����侫�Ƚϵ�,����Ҫ��ͼƬ��������ռ�����ϴ�,����Ч����������)
void FFT2Rotate(Mat &img) {
	Mat after_guussian;
	GaussianBlur(img, after_guussian, Size(5, 5), 0);

	Mat imageSobelX, imageSobelY, imageSobelOut;
	//ˮƽ�ʹ�ֱ����Ҷ�ͼ����ݶȺ�,ʹ��Sobel����    
	Mat imageX16S, imageY16S;
	Sobel(after_guussian, imageX16S, CV_16S, 1, 0, 3, 1, 0, 4);
	Sobel(after_guussian, imageY16S, CV_16S, 0, 1, 3, 1, 0, 4);
	convertScaleAbs(imageX16S, imageSobelX, 1, 0);
	convertScaleAbs(imageY16S, imageSobelY, 1, 0);
	imageSobelOut = imageSobelX + imageSobelY;
	//imshow("XY�����ݶȺ�", imageSobelOut);

	Mat srcImg = imageSobelOut;
	//������䣬�Ǳ��룬�ض��Ŀ�߿�����߸���Ҷ����Ч��  
	Mat padded;
	int opWidth = getOptimalDFTSize(srcImg.rows);
	int opHeight = getOptimalDFTSize(srcImg.cols);
	copyMakeBorder(srcImg, padded, 0, opWidth - srcImg.rows, 0, opHeight - srcImg.cols, BORDER_CONSTANT, Scalar::all(0));
	Mat planes[] = { Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F) };
	Mat comImg;

	//ͨ���ںϣ��ںϳ�һ��2ͨ����ͼ��  
	merge(planes, 2, comImg);
	dft(comImg, comImg);
	split(comImg, planes);
	//magnitude(planes[0], planes[1], planes[0]);
	Mat no_use;
	cartToPolar(planes[0], planes[1], planes[0], no_use);
	Mat magMat = planes[0];
	magMat += Scalar::all(1);
	log(magMat, magMat);     //�����任��������ʾ 
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
	//namedWindow("����ҶƵ��", 0);
	//cvResizeWindow("����ҶƵ��", 800, 600);
	//imshow("����ҶƵ��", magImg);

	//HoughLines���Ҹ���ҶƵ�׵�ֱ�ߣ���ֱ�߸�ԭͼ��һά�뷽���໥��ֱ
	threshold(magImg, magImg, 180, 255, CV_THRESH_BINARY);
	//namedWindow("��ֵ��", 0);
	//cvResizeWindow("��ֵ��", 800, 600);
	//imshow("��ֵ��", magImg);

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

	//namedWindow("Houghֱ��", 0);
	//cvResizeWindow("Houghֱ��", 800, 600);
	//imshow("Houghֱ��", linImg);

	//У���Ƕȼ���
	float angelD = 180 * theta_global / CV_PI - 90;
	Point center(img.cols / 2, img.rows / 2);
	Mat rotMat = getRotationMatrix2D(center, angelD, 1.0);
	Mat imageSource = Mat::ones(img.size(), CV_8UC3);
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//����任У��ͼ��  
																				  //namedWindow("�Ƕ�У��", 0);
																				  //cvResizeWindow("�Ƕ�У��", 800, 600);
																				  //imshow("�Ƕ�У��", imageSource);
}

/// TEST:����hough�任�õ��Ƕ���Ϣ���任(��Ϊʧ��,һ��ͨ���Խϲ�)
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
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//����任У��ͼ��  
	imshow("�Ƕ�У��", imageSource);
}

/// INUSE:ͨ������õ��ĽǶȽ��н���
void Angle2Rotate(Mat &img, float angle) {
	Point center(img.cols / 2, img.rows / 2);
	Mat rotMat = getRotationMatrix2D(center, angle, 1.0);
	Mat imageSource = Mat::ones(img.size(), CV_8UC3);
	warpAffine(img, imageSource, rotMat, img.size(), 1, 0, Scalar(255, 255, 255));//����任У��ͼ��  
}

int main1(int argc, char *argv[]) {
	/// ��ʱ
	clock_t begin, end;

	/// ����ߴ�(�Խ��Ӱ��ϴ�,���Խ��ж�γ���)
	int block_size = 70;

	begin = clock();
	const string IMG_PATH = "../imgs/15633495854075477.jpg";

	Mat src = imread(IMG_PATH, CV_LOAD_IMAGE_COLOR);
	if (!src.data) {
		cout << "Read image error" << endl;
		return -1;
	}

	string indexstr = "05";

	///��ͼƬ��ʽ��Ϊ�Ҷ�ͼ��
	cvtColor(src, src, CV_RGB2GRAY);

	///make a copy
	Mat src_cpy;
	src.copyTo(src_cpy);

	///��ͼ�����һ��9��9�ľ�ֵ�˲�,��ʵ�ֽ�ͼ��ϸ�ڲ��ֵ�������Ϣ����
	blur(src_cpy, src_cpy, Size(9, 9));

	///������ͼ�����Sobel�����������ϵ��ݶ�
	Mat gx(src_cpy.rows, src_cpy.cols, CV_32F);
	Mat gy(src_cpy.rows, src_cpy.cols, CV_32F);
	Sobel(src_cpy, gx, CV_32F, 1, 0, -1);
	Sobel(src_cpy, gy, CV_32F, 0, 1, -1);

	///�������������ϵ��ݶ���Ϣ,���ÿ�����ص�Ļ��Ⱥ�ÿ�����ص���ݶ�ǿ��
	Mat magnitude(gx.rows, gx.cols, CV_32F);
	Mat anglemat(gx.rows, gx.cols, CV_32F);
	cartToPolar(gx, gy, magnitude, anglemat);

	///�õ��ݶ�ǿ�ȵ������Сֵ
	minMaxIdx(magnitude, &gradient_minVal, &gradient_maxVal);

	///��ͼ��ָ�����
	vector<Mat> imgDiv;
	imgDiv = division(src, block_size, block_size);

	///����ÿ������,��������������Ч���������Լ�������Ϣ
	for (vector<Mat >::iterator p = imgDiv.begin(); p < imgDiv.end(); ++p) {
		sobelAndCalc(*p);
	}

	blockSetTravel();

	/// �����ŵ����巽��
	int dir = getDirection();
	/// ���õ��ĸ÷������������ݶ�ǿ�Ȼ��Ʋ�ͬ�Ҷȼ�ǿ�ȵ�ɫ��
	Mat region = drawBlockSet(src.size(), dir);

	namedWindow("Region", 0);
	imshow("Region", region);

#ifdef FILE_SAVE
	savedivisionfiles(imgDiv);
#endif

	/// Ѱ�������ͨ��
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

	/// �������ͨ����Ƴ���
	for (int i = 0; i < 4; i++)
		line(src, vertices[i], vertices[(i + 1) % 4], Scalar(0, 0, 255), 4);

	namedWindow("Lined", 0);
	imshow("Lined", src);

	/// ��ø��������С������Ӿ���
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

	/// ��������Ӿ��ε�position&size
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

	/// ���ü�֮���������С��Ӿ�����ʾ
	namedWindow("Clip", 0);
	imshow("Clip", region_clip);

	/// ��������Ĵ��Ƕȵ���Ӿ��εĶ������ڲü�֮���ͼƬ�е�����
	Point2d top_point(vertices[select_top_index]);
	top_point.x = (int)top_point.x - x_min;
	top_point.y = (int)top_point.y - y_min;

	/// ��������Ĵ��Ƕȵ���Ӿ��εĳ�����̱ߵĳ���
	double distance1 = sqrt((vertices[1].x - vertices[0].x)*(vertices[1].x - vertices[0].x) + (vertices[1].y - vertices[0].y)*(vertices[1].y - vertices[0].y));
	double distance2 = sqrt((vertices[2].x - vertices[1].x)*(vertices[2].x - vertices[1].x) + (vertices[2].y - vertices[1].y)*(vertices[2].y - vertices[1].y));

	/// ��Ҫ��ת�ĽǶ�
	double angle = 0;
	/// ��ת֮���ͼ��ĳߴ�
	double width_rotate = 0, height_rotate = 0;

	/// �ж���һ���ǳ���,Ȼ�������Ҫ��ת�ĽǶȺ���ͼ��ĳߴ�(��Ϊ���ս����Ҫ�Գ��߳���Ϊ���[��ˮƽ���õ����������Ҫ�Ľ��])
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

	/// ��ͼ��ת��Ϊ�Ƕ�
	angle = angle * 180.0 / CV_PI;

	Mat rotateImg;
	region_clip.copyTo(rotateImg);

	/// ���������ԭ���ͽӽ�ˮƽ,����Ҫ��ת
	if ((int)angle != 180 && (int)angle != 0) {
		/// �Բü���(δ��ת)��ͼ������ĵ���ת
		Point2f center = Point2f(region_clip.cols / 2, region_clip.rows / 2);
		/// ����������
		double scale = 1.0;

		/// ����õ���ת����(�þ��󲻰���ƽ�ƵĲ���[����ʹ�øþ��������ת��ʱ��,�������ƽ��])
		Mat rotateMat;
		rotateMat = getRotationMatrix2D(center, angle, scale);

		/// ����ת�������ƽ�ƵĲ���(���²��輴����������ǵ��ƶ����µ�ͼƬ�����Ͻ�)
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

	/// ������תֻ��ͨ�����������εĽǶȽ�����ת
	/// ������ת�Ǹ���ͳ�Ƶõ��ĽǶȽ��ж�����ת(��߾���)
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
