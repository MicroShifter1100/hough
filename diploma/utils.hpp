#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include <map>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>

using namespace cv;
using namespace std;

Mat loadImage(string path)
{
    Mat src = imread(path); // Load an image

    if (src.empty())
    {
        throw std::runtime_error("Could not open or find the image!\n");
    }

    return src;
}

void showImage(string name, Mat image_to_show)
{
    cv::imshow(name, image_to_show);
}

Mat getEdges(Mat image)
{
    cv::Mat detected_edges;
    cv::Canny(image, detected_edges, 60, 100);
    return detected_edges;
}

Mat getOrientation(Mat detected_edges)
{
    cv::Mat orientation(detected_edges.size(), CV_64F);
    Mat sobel_x(detected_edges.size(), CV_64F);
    Mat sobel_y(detected_edges.size(), CV_64F);
    cv::Sobel(detected_edges, sobel_x, CV_64F, 1, 0, 5);
    cv::Sobel(detected_edges, sobel_y, CV_64F, 0, 1, 5);
    double t;

    for (int y = 0; y < sobel_x.rows; y++)
        for (int x = 0; x < sobel_y.cols; x++)
        {
            t = atan2(sobel_y.at<double>(y, x), sobel_x.at<double>(y, x));
            orientation.at<double>(y, x) = (t == 180) ? 0 : t;
        }

    // std::cout << orientation;
    return orientation;
}

std::vector<std::array<size_t, 2>> getEdgeCoordinate(Mat detected_edges)
{
    std::vector<std::array<size_t, 2>> coordinates = {};
    int x = detected_edges.cols;
    int y = detected_edges.rows;

    for (int i = 0; i < detected_edges.rows; ++i)
    {
        for (int j = 0; j < detected_edges.cols; ++j)
        {
            int pixel_value = detected_edges.at<u_char>(i, j);
            if (detected_edges.at<char>(i, j))
            {
                coordinates.push_back({{i, j}});
            }
        }
    }

    return coordinates;
}

std::array<size_t, 2> getReferencePoint(std::vector<std::array<size_t, 2>> coordinates)
{
    std::size_t x = 0;
    std::size_t y = 0;

    for (auto coord : coordinates)
    {
        x += coord[0];
        y += coord[1];
    }

    x /= coordinates.size();
    y /= coordinates.size();

    std::array<std::size_t, 2> ref_point = {x, y};
    // std::cout << x << ' ' << y;
    return ref_point;
}
// const double PI = 4.0 * std::atan(1.0);

// int rad2SliceIndex(double angle, int nSlices)
// {
//     double a = (angle > 0) ? (fmodf(angle, 2 * PI)) : (fmodf(angle + 2 * PI, 2 * PI));
//     return floor(a / (2 * PI / nSlices + 0.00000001));
// }

// std::multimap<float, std::vector<std::array<int, 2>>>
// int getRTable(Mat detected_edges)
// {
//     // auto gradient = getOrientation(detected_edges);
//     // std::multimap<double, std::array<int, 2>> R_table = {};
//     std::vector<std::vector<std::vector<int>>> R_table = {};
//     R_table.clear();
//     int m_deltaRotationAngle = PI / 48;
//     int m_nSlices = (2.0 * PI) / m_deltaRotationAngle;
//     double phi;
//     int iSlice;

//     for (int y = 0; y < detected_edges.rows; y++)
//     {
//         uchar *templateRow = detected_edges.ptr<uchar>(y);
//         std::cout << *templateRow;
//         // double *directionRow = direction.ptr<double>(y);
//         for (int x = 0; x < detected_edges.cols; x++)
//         {
//             if (templateRow[x] == 255)
//             {
//                 std::cout << templateRow[x];
//                 // phi = directionRow[x]; // gradient direction in radians in [-PI;PI]
//                 // std::cout << phi << '\n';
//                 // iSlice = rad2SliceIndex(phi, m_nSlices);
//                 // std::cout << iSlice << '\n';

//                 // std::vector<int> coord;
//                 // coord.push_back(ref_point[0] - x);
//                 // coord.push_back(ref_point[1] - y);
//                 // R_table[iSlice].push_back(coord);
//             }
//             // else
//             // {
//             //     std::cout << '0';
//             // }
//         }
//         // std::cout << std::endl;
//     }

//     return 0;

// // auto gradient = getOrientation(detected_edges);
// std::multimap<double, std::array<int, 2>> R_table = {};
// // std::cout << gradient;
// for (auto point : coordinates)
// {
//     int rx = ref_point[0] - point[0];
//     int ry = ref_point[1] - point[1];
//     // std::cout << "RX: " << rx << " RY: " << ry << " REF_P_X: " << ref_point[0] << " REF_P_Y: " << ref_point[0] << " X: " << point[0] << " Y: " << point[1] << '\n';
//     std::array<int, 2> r = {rx, ry};
//     double phi = direction.at<double>(point[0], point[1]);
//     // std::cout << "PHI: " << phi << '\n';

//     R_table.insert(std::make_pair(phi, r));
// }

// for (auto phi : R_table)
// {
//     std::cout << phi.first << ": ";
//     for (auto coord : phi.second)
//     {
//         std::cout << coord << ' ';
//     }
//     std::cout << std::endl;
// }

// return 0;
// }

// void GeneralHoughTransform::showRTable(std::multimap<double, std::array<int, 2>> R_table)
// {
//     int N(0);
//     cout << "--------" << endl;
//     for (auto r : RTable)
//     {
//         for (auto c : r)
//         {
//             cout << c;
//             N++;
//         }
//         cout << endl;
//     }
//     cout << N << " elements" << endl;
// }
