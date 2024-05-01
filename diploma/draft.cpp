#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "utils.hpp"

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
double m_maxRotationAngle = 0;
double m_deltaScaleRatio = 0.01;
double m_minScaleRatio = 1;
double m_maxScaleRatio = 1;
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
    std::cout << m_origin;
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
                std::cout << iSlice << '\n';
                m_RTable[iSlice].push_back(Vec2f(m_origin[0] - x, m_origin[1] - y));
            }
        }
        std::cout << std::endl;
    }
}

vector<vector<Vec2f>> scaleRTable(const vector<vector<Vec2f>> &RTable, double ratio)
{
    vector<vector<Vec2f>> RTableScaled(RTable.size());
    for (auto iSlice = 0u; iSlice < RTable.size(); ++iSlice)
    {
        for (auto r : RTable[iSlice])
        {
            RTableScaled[iSlice].push_back(Vec2f(ratio * r[0], r[1]));
        }
    }
    return RTableScaled;
}

vector<vector<Vec2f>> rotateRTable(const vector<vector<Vec2f>> &RTable, double angle)
{
    vector<vector<Vec2f>> RTableRotated(RTable.size());
    double c = cos(angle);
    double s = sin(angle);
    int iSliceRotated;
    for (auto iSlice = 0u; iSlice < RTable.size(); ++iSlice)
    {
        iSliceRotated = rad2SliceIndex(iSlice * m_deltaRotationAngle + angle, m_nSlices);
        for (auto r : RTable[iSlice])
        {
            RTableRotated[iSliceRotated].push_back(Vec2f(c * r[0] - s * r[1], s * r[0] + c * r[1]));
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

void accumulate(const Mat &image)
{
    // Canny edge for image
    Mat grayImage(image.size(), CV_8UC1), edges(image.size(), CV_8UC1);
    cvtColor(image, edges, COLOR_BGR2GRAY);
    blur(edges, edges, Size(3, 3));
    Canny(edges, edges, m_cannyThreshold1, m_cannyThreshold2);
    Mat direction = gradientDirection(edges);

    imshow("debug - src edges", edges);
    imshow("debug - src edges gradient direction", direction);

    int X = image.cols;
    int Y = image.rows;
    int S = ceil((m_maxScaleRatio - m_minScaleRatio) / m_deltaScaleRatio) + 1;          // Scale Slices Number
    int R = ceil((m_maxRotationAngle - m_minRotationAngle) / m_deltaRotationAngle) + 1; // Rotation Slices Number

    Mat out(image.size(), image.type());
    image.copyTo(out);

    vector<vector<Mat>> accum(R, vector<Mat>(S, Mat::zeros(Size(X, Y), CV_64F)));
    vector<vector<Vec2f>> RTableRotated(m_RTable.size()), RTableScaled(m_RTable.size());
    Mat showAccum(Size(X, Y), CV_8UC1);
    vector<GHTPoint> points;

    for (double angle = m_minRotationAngle; angle <= m_maxRotationAngle + 0.0001; angle += m_deltaRotationAngle)
    {
        double max = 0;
        GHTPoint point;
        auto iRotationSlice = round((angle - m_minRotationAngle) / m_deltaRotationAngle);
        cout << "Rotation Angle\t: " << angle / PI * 180 << "�" << endl;
        RTableRotated = rotateRTable(m_RTable, angle);
        for (double ratio = m_minScaleRatio; ratio <= m_maxScaleRatio + 0.0001; ratio += m_deltaScaleRatio)
        {
            auto iScaleSlice = round((ratio - m_minScaleRatio) / m_deltaScaleRatio);
            cout << "|- Scale Ratio\t: " << ratio * 100 << "%" << endl;
            RTableScaled = scaleRTable(RTableRotated, ratio);
            accum[iRotationSlice][iScaleSlice] = Mat::zeros(Size(X, Y), CV_64F);
            for (auto y = 0; y < image.rows; ++y)
            {
                for (auto x = 0; x < image.cols; ++x)
                {
                    auto phi = direction.at<double>(y, x);
                    if (phi != 0.0)
                    {
                        auto iSlice = rad2SliceIndex(phi, m_nSlices);

                        // For each r related to this angle-slice
                        for (auto r : RTableScaled[iSlice])
                        {
                            // We compute x+r, the supposed template origin position
                            auto ix = x + round(r[0]);
                            auto iy = y + round(r[1]);

                            // If it's between the image boundaries
                            if (ix >= 0 && ix < image.cols && iy >= 0 && iy < image.rows)
                            {
                                // Icrement the accum
                                if (++accum[iRotationSlice][iScaleSlice].at<double>(iy, ix) >= max)
                                {
                                    // If far away enough from detected points
                                    bool ok = true;
                                    // cout << "[" << ix << ", " << iy << "]" << ", hits: " << point.hits;
                                    for (auto oldPoint : points)
                                    {
                                        auto oldX = oldPoint.y.x;
                                        auto oldY = oldPoint.y.y;
                                        auto distance = sqrt(pow(oldX - ix, 2) + pow(oldY - iy, 2));
                                        if (distance < m_templateImage.cols / 4)
                                        {
                                            ok = false;
                                            // cout << " NOT far enough away from " << oldPoint.y << " REJECT" << endl;
                                            break;
                                        }
                                    }
                                    if (ok)
                                    {
                                        // cout << " far enough away OK" <<endl;
                                        max = accum[iRotationSlice][iScaleSlice].at<double>(iy, ix);
                                        point.phi = angle;
                                        point.s = ratio;
                                        point.y.y = iy;
                                        point.y.x = ix;
                                        point.hits = accum[iRotationSlice][iScaleSlice].at<double>(iy, ix);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            /* Pushing back the best point for each transformation (uncomment line 159 : "max = 0") */
            points.push_back(point);
            // Draw on out image
            drawTemplate(out, point);
            imshow("debug - output", out);
            /* Transformation accumulation visualisation */
            normalize(accum[iRotationSlice][iScaleSlice], showAccum, 0, 255, NORM_MINMAX, CV_8UC1); // To see each transformation accumulation (uncomment line 159 : "max = 0")
                                                                                                    // normalize(totalAccum, showAccum, 0, 255, NORM_MINMAX, CV_8UC1); // To see the cumulated accumulation (comment line 159 : "max = 0")
            imshow("debug - accum", showAccum);
            waitKey(0);
        }
    }
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