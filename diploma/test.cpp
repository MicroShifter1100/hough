#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace cv;

int main( int argc, char** argv )
{
    Mat src, src_gray, detected_edges;
    int lowThreshold = 8;
    const int ratio = 3;
    const int kernel_size = 3;
    const char* window_name = "Edge Map";

    CommandLineParser parser( argc, argv, "{@input | Untitled.png | input image}" );
    src = imread( samples::findFile( parser.get<String>( "@input" ) ), IMREAD_GRAYSCALE ); // Load an image
    if( src.empty() )
    {
    std::cout << "Could not open or find the image!\n" << std::endl;
    std::cout << "Usage: " << argv[0] << " <Input image>" << std::endl;
    return -1;
    }

    // src = imread("123.jpg", IMREAD_GRAYSCALE); // Загрузка изображения в оттенках серого

    // Проверка наличия изображения
    if( src.empty() )
    {
        std::cout << "Could not open or find the image!\n" << std::endl;
        return -1;
    }

    // Применение размытия перед обнаружением краев
    blur( src, detected_edges, Size(3,3) );

    Mat binary_image;

    // Обнаружение краев методом Canny
    Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );

    // Преобразование изображения Canny в бинарное
    // // Отображение бинарного изображения
    namedWindow( window_name, WINDOW_AUTOSIZE );
    imshow( window_name, detected_edges );
    imwrite("output_image.jpg", detected_edges);
    for (int i = 0; i < detected_edges.rows; ++i) {
        for (int j = 0; j < detected_edges.cols; ++j) {
            int pixel_value = detected_edges.at<uchar>(i, j);
            std::cout << std::setw(5) << pixel_value;
            // std::cout << std::setw(5) << pixel_value;
            // Здесь вы можете использовать значение пикселя pixel_value
        }
        std::cout << std::endl;
    }    
    // for (int i = 0; i < detected_edges.rows; i++) {
    //     for (int j = 0; j < detected_edges.cols; j++) {
    //         std::cout << detected_edges.data[i+j];
    //     }
    //     std::cout << std::endl;
    // }
    std::ofstream out("data.txt");

    out << detected_edges;
    waitKey(0);
    return 0;
}