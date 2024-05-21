#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "utils.hpp"
#include "omp.h"

using namespace cv;

std::vector<std::vector<cv::Vec2f>> m_RTable;
cv::Vec2f m_origin;
cv::Mat m_templateImage;
cv::Mat m_grayTemplateImage;
cv::Mat m_template;

const double PI = 4.0 * std::atan(1.0);
int m_cannyThreshold1 = 300;
int m_cannyThreshold2 = 300;
double m_minPositivesDistance;
double m_deltaRotationAngle = PI / 48;
double m_minRotationAngle = 0;
double m_maxRotationAngle = 1;
double m_deltaScaleRatio = 0.1;
double m_minScaleRatio = 0.7;
double m_maxScaleRatio = 1.5;
int m_nScales = (m_maxScaleRatio - m_minScaleRatio) / m_deltaScaleRatio + 1;
int m_nRotations = (m_maxRotationAngle - m_minRotationAngle) / m_deltaRotationAngle + 1;
int m_nSlices = (2.0 * PI) / m_deltaRotationAngle;

struct GHTPoint
{
    double phi;
    double s;
    cv::Point y;
    double hits;
};

void findOrigin()
{
    m_origin = Vec2f(m_templateImage.cols / 2, m_templateImage.rows / 2); // By default, the origin is at the center
    // std::cout << m_origin;
}

Mat gradientDirection(const Mat &src)
{
    Mat dst(src.size(), CV_64F);
    Mat gradX(src.size(), CV_64F);
    Sobel(src, gradX, CV_64F, 1, 0, 5);
    Mat gradY(src.size(), CV_64F);
    Sobel(src, gradY, CV_64F, 0, 1, 5);
    double t;
    for (int y = 0; y < gradX.rows; y++)
    {
        for (int x = 0; x < gradX.cols; x++)
        {
            t = atan2(gradY.at<double>(y, x), gradX.at<double>(y, x));
            dst.at<double>(y, x) = (t == 180) ? 0 : t;
        }
    }
    return dst;
}

int rad2SliceIndex(double angle, int nSlices)
{
    double a = (angle > 0) ? (fmodf(angle, 2 * PI)) : (fmodf(angle + 2 * PI, 2 * PI));
    return floor(a / (2 * PI / nSlices + 0.00000001));
}

void createRTable()
{
    int iSlice;
    double phi;

    Mat direction = gradientDirection(m_template);
    // std::cout << direction;
    imshow("debug - template", m_template);
    imshow("debug - positive directions", direction);

    m_RTable.clear();
    m_RTable.resize(m_nSlices);
    for (auto y = 0; y < m_template.rows; ++y)
    {
        uchar *templateRow = m_template.ptr<uchar>(y);
        double *directionRow = direction.ptr<double>(y);
        for (auto x = 0; x < m_template.cols; ++x)
        {
            if (templateRow[x] == 255)
            {
                phi = directionRow[x]; // gradient direction in radians in [-PI;PI]
                // std::cout << phi << '\n';
                iSlice = rad2SliceIndex(phi, m_nSlices);
                // std::cout << iSlice << '\n';
                m_RTable[iSlice].push_back(Vec2f(m_origin[0] - x, m_origin[1] - y));
            }
        }
        // std::cout << std::endl;
    }
}

vector<vector<Vec2f>> scaleRTable(const vector<vector<Vec2f>> &RTable, double ratio)
{
    vector<vector<Vec2f>> RTableScaled(RTable.size());
    for (size_t i = 0; i < RTable.size(); i++)
    {
        for (auto r : RTable[i])
        {
            RTableScaled[i].push_back(Vec2f(ratio * r[0], ratio * r[1]));
        }
    }
    return RTableScaled;
}

vector<vector<Vec2f>> rotateRTable(const vector<vector<Vec2f>> &RTable, double angle, int m_nSlices)
{
    vector<vector<Vec2f>> RTableRotated(RTable.size());

    double c = cos(angle);
    double s = sin(angle);

    for (size_t iSlice = 0; iSlice < RTable.size(); ++iSlice)
    {
        for (size_t i = 0; i < RTable[iSlice].size(); ++i)
        {
            // Вычисляем новый индекс среза для текущего элемента
            int iSliceRotated = static_cast<int>((iSlice * (2 * CV_PI / m_nSlices) + angle) * m_nSlices / (2 * CV_PI)) % m_nSlices;

            // Поворачиваем текущий элемент и добавляем его в новую R-таблицу
            Vec2f rotatedElement(c * RTable[iSlice][i][0] - s * RTable[iSlice][i][1],
                                 s * RTable[iSlice][i][0] + c * RTable[iSlice][i][1]);
            RTableRotated[iSliceRotated].push_back(rotatedElement);
        }
    }

    return RTableRotated;
}

void setTemplate(const Mat &templateImage)
{
    templateImage.copyTo(m_templateImage);
    findOrigin();
    cvtColor(m_templateImage, m_grayTemplateImage, COLOR_BGR2GRAY);
    m_grayTemplateImage.convertTo(m_grayTemplateImage, CV_8UC1);
    m_template = Mat(m_grayTemplateImage.size(), CV_8UC1);
    blur(m_grayTemplateImage, m_template, Size(3, 3));
    Canny(m_template, m_template, m_cannyThreshold1, m_cannyThreshold2);
    createRTable();
}
void drawTemplate(Mat &image, GHTPoint params)
{
    cout << params.y << " scale: " << params.s << ", rotation: " << params.phi / PI * 180 << "�, hits: " << params.hits << endl;
    double c = cos(params.phi);
    double s = sin(params.phi);
    int x(0), y(0), relx(0), rely(0);

    cv::Rect rect{params.y.x - static_cast<int>((m_templateImage.rows / 2) * m_maxScaleRatio), params.y.y - static_cast<int>((m_templateImage.cols / 2) * m_maxScaleRatio), static_cast<int>(m_templateImage.rows * m_maxScaleRatio), static_cast<int>(m_templateImage.cols * m_maxScaleRatio)};
    cv::rectangle(image, rect, cv::Scalar{0, 0, 255, 255}, 3);
}

struct RefPoint
{
    size_t x;
    size_t y;
    size_t score;
    size_t rotate_index;
    size_t scale_index;
    size_t scale;
    size_t rotate;
};

void drawTemplate(const Mat &image, RefPoint params)
{
    int x(0), y(0), relx(0), rely(0);

    cv::Rect rect{params.x - (m_templateImage.rows / 2), params.y - (m_templateImage.cols / 2), 54, 57};
    cv::rectangle(image, rect, cv::Scalar{0, 0, 255, 255}, 3);
    imshow("RESULT", image);
}

void accumulate(const Mat &image)
{
    Mat grayImage(image.size(), CV_8UC1), edges(image.size(), CV_8UC1);
    cvtColor(image, edges, COLOR_BGR2GRAY);
    blur(edges, edges, Size(3, 3));
    Canny(edges, edges, m_cannyThreshold1, m_cannyThreshold2);
    Mat direction = gradientDirection(edges);

    imshow("debug - src edges", edges);
    imshow("debug - src edges gradient direction", direction);

    int X = image.cols;
    int Y = image.rows;

    Mat out(image.size(), image.type());
    image.copyTo(out);
    vector<vector<Vec2f>> TransformedRTable(m_RTable.size()), RTableScaled(m_RTable.size());

    int S = ceil((m_maxScaleRatio - m_minScaleRatio) / m_deltaScaleRatio) + 1; // Scale Slices Number
    int R = ceil((m_maxRotationAngle - m_minRotationAngle) / m_deltaRotationAngle) + 1;

    vector<vector<Mat>> accum(R, vector<Mat>(S, Mat::zeros(Size(X, Y), CV_64F)));

    vector<RefPoint> points = {};
    size_t global_max_score = 0;

    for (size_t step = 0; step < R; step++)
    {
        double angle = m_minRotationAngle + step * m_deltaRotationAngle + 0.0001;
        size_t iRotationSlice = round((angle - m_minRotationAngle) / m_deltaRotationAngle);
        TransformedRTable = rotateRTable(m_RTable, angle, m_nSlices);

        size_t max = 0;
        size_t max2 = 0;
        size_t max_yc = 0;
        size_t max_xc = 0;
        size_t max_r = 0;

        for (double ratio = m_minScaleRatio; ratio <= m_maxScaleRatio + 0.0001; ratio += m_deltaScaleRatio)
        {
            size_t iScaleSlice = round((ratio - m_minScaleRatio) / m_deltaScaleRatio);
            TransformedRTable = scaleRTable(TransformedRTable, ratio);
            accum[iRotationSlice][iScaleSlice] = Mat::zeros(Size(X, Y), CV_64F);

            for (size_t y = 0; y < Y; y++)
            {
                for (size_t x = 0; x < X; x++)
                {
                    double phi = direction.at<double>(y, x);

                    if (phi != 0.0)
                    {
                        size_t index = rad2SliceIndex(phi, m_nSlices);

                        size_t r_count = 0;
                        for (auto r : TransformedRTable[index])
                        {
                            r_count++;
                            int xc = x + r[0];
                            int yc = y + r[1];

                            if (xc >= 0 && xc < image.cols && yc >= 0 && yc < image.rows)
                            {
                                if (++accum[iRotationSlice][iScaleSlice].at<double>(yc, xc) >= global_max_score)
                                {
                                    max2 = accum[iRotationSlice][iScaleSlice].at<double>(yc, xc);
                                    global_max_score = max2;
                                    max_yc = yc;
                                    max_xc = xc;
                                    RefPoint point;
                                    point.x = xc;
                                    point.y = yc;
                                    point.score = max2;
                                    point.scale = ratio * 100;
                                    point.rotate = angle / PI * 180;
                                    point.rotate_index = iRotationSlice;
                                    point.scale_index = iScaleSlice;
                                    points.push_back(point);
                                }
                            }
                        }
                        max_r += r_count;
                    }
                }
            }
        }
    }

    struct pred
    {
        bool operator()(const RefPoint &dot1, const RefPoint &dot2)
        {
            return dot1.score < dot2.score;
        }
    };

    std::sort(points.begin(), points.end(), pred());

    for (auto point : points)
    {
        std::cout << "X: " << point.x << "\tY: " << point.y << "\tRotate: " << point.rotate_index << "\tScale: " << point.scale_index << "\tScore: " << point.score << '\n';
    }

    drawTemplate(image, points.back());
}

int main(int argc, char **argv)
{
    Mat tpl = imread("template_Q.png");
    imshow("template", tpl);

    setTemplate(tpl);

    Mat source = imread("letters.png");
    imshow("source", source);
    accumulate(source);
    cv::waitKey(0);
    return 0;
}