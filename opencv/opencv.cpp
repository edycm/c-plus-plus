#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"


void RotateImage(const cv::Mat& inputImg, cv::Mat& outputImg, float angle)
{
    CV_Assert(!inputImg.empty());

    float radian = (float)(angle / 180.0 * CV_PI);
    float sinVal = fabs(sin(radian));
    float cosVal = fabs(cos(radian));

    cv::Size targetSize((int)(inputImg.cols * cosVal + inputImg.rows * sinVal), 
                        (int)(inputImg.cols * sinVal + inputImg.rows * cosVal));

    int dx = (int)(inputImg.cols * cosVal + inputImg.rows * sinVal - inputImg.cols) / 2;
    int dy = (int)(inputImg.cols * sinVal + inputImg.rows * cosVal - inputImg.rows) / 2;

    copyMakeBorder(inputImg, outputImg, dy, dy, dx, dx, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    cv::Point2f center((float)(outputImg.cols / 2), (float)(outputImg.rows / 2));
    cv::Mat affine_matrix = cv::getRotationMatrix2D(center, angle, 1.0);
    warpAffine(outputImg, outputImg, affine_matrix, outputImg.size());
}

void RotateImage(cv::Mat& image) 
{
#if 0 // 逆向90°
    cv::transpose(image, image);
    cv::flip(image, image, 0);
#endif
#if 0 // 180°
    //cv::flip(image, image, -1); //沿x、y轴翻转
    cv::flip(image, image, 0); //沿x轴翻转
    cv::flip(image, image, 1); //沿y轴翻转
#endif
#if 0
    transpose(image, image); //矩阵行列变换
    cv::flip(image, image, 1);
#endif
#if 1
    cv::transpose(image, image); //矩阵行列变换
#endif 
}

int main()
{
    int scaleSize = 10;

    cv::Mat image = cv::imread("./frame1.jpeg");
    if (!image.data) {
        printf("image data is empty!\n");
        return -1;
    }

    cv::Mat largeImg(image.rows + scaleSize, image.cols + scaleSize, CV_8UC3, cv::Scalar(255, 255, 255));
    //cv::Mat tmpImg = largeImg(cv::Rect(5, 5, image.cols, image.rows));
    cv::Mat tmpImg = largeImg(cv::Rect(image.cols / 4, image.rows / 4, image.cols / 2, image.rows / 2));

    //cv::resize(image, tmpImg, cv::Size(image.cols, image.rows));
    cv::resize(image, tmpImg, cv::Size(0, 0), 0.5, 0.5);

    cv::Mat flipImg = image.clone();
    RotateImage(flipImg);

    cv::Mat adjustImg = image.clone();
    RotateImage(image, adjustImg, -5);

    cv::Mat cropImage = image(cv::Rect(0, 0, image.cols / 2, image.rows));

    cv::imwrite("C:\\Users\\lenovo\\Desktop\\1.jpeg", largeImg);
    cv::imwrite("C:\\Users\\lenovo\\Desktop\\2.jpeg", flipImg);
    cv::imwrite("C:\\Users\\lenovo\\Desktop\\3.jpeg", adjustImg);
    cv::imwrite("C:\\Users\\lenovo\\Desktop\\4.jpeg", cropImage);
    //encode decode
#if 0
    std::vector<uchar> buf;
    bool ret = cv::imencode(".jpg", image, buf);
    printf("ret: %d\n", ret);
    std::string strBuf(buf.begin(), buf.end());
    printf("strBuf.size: %d\n", strBuf.size());
    buf.clear();
    buf.resize(0);

    cv::Mat deImg = cv::imdecode(std::vector<uchar>(strBuf.begin(), strBuf.end()), 1);
    if (deImg.data) {
        printf("imdecode success!\n");
        printf("deImag.cols: %d, deImg.rows: %d\n", deImg.cols, deImg.rows);
    }
#endif
    return 0;
}