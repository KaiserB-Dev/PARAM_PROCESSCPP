
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
//#include <opencv2/ml/ml.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <ctime>
#define M_PI 3.14159265358979323846
/*
#include "MatlabEngine.hpp" 
#include "MatlabDataArray.hpp"*/

double degr_rad(double degr);

double degr_rad(double degr){
    double rad;

    rad = degr*(M_PI/180);

    return rad;
}

int main(int argc, char** argv){

    std::clock_t start = std::clock();
    cv::VideoCapture video;
    cv::Mat frame, frameGray, NormalizeFrame, thFrame, DilFrame, cannyFrame; //Definiendo matrices vacias
    cv::Mat kernel;
    kernel.ones(cv::Size(10,10), 8);

    //std::cout<<start<<std::endl;

    cv::namedWindow("MainWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("GrayWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("NormalizedWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("thWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("DWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("CannyWindow", cv::WINDOW_NORMAL);

    //std::string strCoords;
    std::vector<std::vector<cv::Point>> cnts; //[x,y]

    cv::CommandLineParser parser( argc, argv, "{@input | stuff.wmv | input video}" ); //Parser para obtener y leer el video desde los argumentos de ejecución del programa

    video = cv::VideoCapture(cv::samples::findFile(parser.get<std::string>( "@input" ) ));
    int total_frames = video.get(cv::CAP_PROP_FRAME_COUNT);
    int fps = video.get(cv::CAP_PROP_FPS);


    if(!video.isOpened()){
        std::cout<<"No existe el video solicitado"<<std::endl;
        return 0;
    }
    

    int count = 0;
    while(true){
        video.read(frame);
        if(frame.empty()) break;     

        //preprocessing part
        cv::cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY);
        cv::normalize(frameGray, NormalizeFrame, 0, 255, cv::NORM_MINMAX);
        cv::threshold(NormalizeFrame, thFrame, 60, 255, cv::THRESH_BINARY);
        //cv::adaptiveThreshold(NormalizeFrame, thFrame, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 45, 0);
        cv::dilate(thFrame, DilFrame, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(5,5))));
        cv::erode(DilFrame, DilFrame, kernel, cv::Point(-1,-1), 2, cv::BORDER_DEFAULT, cv::morphologyDefaultBorderValue());
        cv::morphologyEx(DilFrame, DilFrame, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(10,10)))); //Erosiona la imagen y luego dilata la imagen erosionada

        
        //cv::medianBlur(DilFrame, DilFrame, 15);

        
        //contornos
        cv::Canny(DilFrame, cannyFrame, 80, 200);
        cv::findContours(cannyFrame, cnts,cv::RETR_TREE, cv::CHAIN_APPROX_NONE); 

        //cv::drawContours(frame, cnts, -1, cv::Scalar(0,0,255));
    

       //std::cout<<frameGray<<std::endl;
        std::vector<cv::RotatedRect> minEllipse(cnts.size());
        for(int i = 0; i < cnts.size(); ++i){
            minEllipse[i] = cv::minAreaRect(cnts[i]);
            if(cnts[i].size() > 5){
                minEllipse[i] = cv::fitEllipse(cnts[i]);
            }
        }
        
        std::ofstream x("./data_files/x.csv");
        std::ofstream y("./data_files/y.csv");
        std::ofstream angle("./data_files/angle.csv");
        cv::putText(frame, cv::format("Frame: %d/%d", count+1, total_frames), {frame.rows+25, 25}, 1, 2, cv::Scalar(100,0,255),3,8);
        cv::putText(frame, cv::format("FPS: %d", fps), {frame.rows+25, 50}, 1, 2, cv::Scalar(100,0,255),3,8);
        


        for(int i=0; i<minEllipse.size(); ++i){
            double Erad = degr_rad(minEllipse[i].angle);
            cv::ellipse(frame, minEllipse[i], cv::Scalar(0,0,255),2);
            cv::putText(frame, cv::format("No. Paramecium: %ld", minEllipse.size()), {10,25}, 1,2, cv::Scalar(0,255,255),3, 8);
            cv::drawMarker(frame, minEllipse[i].center, cv::Scalar(0,0,255), 0,10);
            cv::putText(frame, cv::format("([%.2f, %.2f], %.2f)",minEllipse[i].center.x,minEllipse[i].center.y, Erad) ,
                        minEllipse[i].center, 1 ,1.3,cv::Scalar(255,255,100),2, cv::LINE_AA);
            /*
           //std::cout<<minEllipse[i].center;
            std::cout<<"PUNTOS DE CONTORNO:"<<std::endl;
            std::cout<<i+1<<": "<<cnts[i]<<std::endl;
            std::cout<<"ELLIPSE SIZE (a y b), center & angle"<<std::endl;
            std::cout<<minEllipse[i].size<<std::endl;
            std::cout<<minEllipse[i].center<<std::endl;
            std::cout<<minEllipse[i].angle<<std::endl;*/

            
            x<<minEllipse[i].center.x<<",";
            y<<minEllipse[i].center.y<<",";
            angle<<Erad<<",";

        }




        

        /*for(int i = 0; i<frame.rows; ++i){
            uchar* rowsFrame = frame.ptr<uchar>(i);
            for(int j = 0; i<frame.cols; ++j){
                std::cout<<(int)rowsFrame[j]<<std::endl;
            }
        }*/
          
        

        cv::imshow("MainWindow", frame);
        cv::imshow("NormalizedWindow", NormalizeFrame);
        cv::imshow("thWindow", thFrame);
        cv::imshow("DWindow", DilFrame);
        cv::imshow("GrayWindow", frameGray);
        cv::imshow("CannyWindow", cannyFrame);
        
        //cv::imshow("thWindow", thFrame);


        if(cv::waitKey(0) == 27){
            break;
        } 
        ++count;

    }
    video.release();
    cv::destroyAllWindows();

    return 0;
}