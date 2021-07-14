
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
#include <deque> 
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
    std::ofstream x("./data_files/x.csv");
    std::ofstream y("./data_files/y.csv");
    std::ofstream angle("./data_files/angle.csv");

    //std::cout<<start<<std::endl;

    cv::namedWindow("MainWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("GrayWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("NormalizedWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("thWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("DWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("CannyWindow", cv::WINDOW_NORMAL);

    //std::string strCoords;
    std::vector<std::vector<cv::Point>> cnts; //[x,y]
    std::vector<cv::Point> points_center_prev;
    std::deque<std::vector<cv::Point>> centers;

    cv::CommandLineParser parser( argc, argv, "{@input | stuff.wmv | input video}" ); //Parser para obtener y leer el video desde los argumentos de ejecución del programa

    video = cv::VideoCapture(cv::samples::findFile(parser.get<std::string>( "@input" ) )); //Carga el input en el objeto video capture (se puede modificar para analizar paramecium en tiempo real con alguna camara como el dinolite)
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
        cv::cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY); //Converción de canal BGR a GRAY
        cv::normalize(frameGray, NormalizeFrame, 0, 255, cv::NORM_MINMAX); //Normalización del frame entre valores de 0 a 255 por el metodo de MINMAX
        cv::threshold(NormalizeFrame, thFrame, 60, 255, cv::THRESH_BINARY); //Binarización del frame, con un valor de thresh de 60 y un maxval de 255
        cv::dilate(thFrame, DilFrame, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(5,5))));//Dilatación de la imagen con una estructura especificada en tamaño y forma para la operacion morfologica.
        //MORPH_ELLIPSE es un elemento estructural eliptico el cual esta inscrito en un rectangulo[x,y,width,heigth].
        cv::erode(DilFrame, DilFrame, kernel, cv::Point(-1,-1), 2, cv::BORDER_DEFAULT, cv::morphologyDefaultBorderValue()); //Esta parte es la eroción de la imagen previamente dilatada. lo hace con una kernel de unos;
        //El punto (-1,-1) indica que el punto de ancla de la eroción esta en el centro de cada elemento y este copia el borde original {PENDIENTE cv::morphologyDefaultBorderValue()}
        cv::morphologyEx(DilFrame, DilFrame, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(10,10)))); //Erosiona la imagen y luego dilata la imagen erosionada
        //fin preprocessing

        //contornos
        cv::Canny(DilFrame, cannyFrame, 80, 200); //Encuentra los bordes de una imagen por medio del algoritmo canny
        cv::findContours(cannyFrame, cnts,cv::RETR_TREE, cv::CHAIN_APPROX_NONE);  //En base a la imagen procesada con canny este busca los puntos de los bordes y los almacena en un vector de vectores de puntos

        //cv::drawContours(frame, cnts, -1, cv::Scalar(0,0,255));
    
        std::vector<cv::RotatedRect> minEllipse(cnts.size()); //Con los puntos previamente calculados aproxima un rectangulo rotatorio (con el fin de calcular y aproximar la elipse con su respectivo angulo de rotación)
        for(int i = 0; i < cnts.size(); ++i){
            minEllipse[i] = cv::minAreaRect(cnts[i]); //Calcula e inscribe el area mas pequeña del rectangulo en donde se inscribira la elipse
            if(cnts[i].size() > 5){
                minEllipse[i] = cv::fitEllipse(cnts[i]); //Inscribe la elipse en el rectangulo
            }
        }
        cv::putText(frame, cv::format("Frame: %d/%d", count+1, total_frames), {frame.rows+25, 25}, 1, 2, cv::Scalar(100,0,255),3,8); //Muestra los frames totales y el numero de frame en el que va el video
        cv::putText(frame, cv::format("FPS: %d", fps), {frame.rows+25, 50}, 1, 2, cv::Scalar(100,0,255),3,8); //Muestra los frames por segundo (FPS)
        


        for(int i=0; i<minEllipse.size(); ++i){
            double Erad = degr_rad(minEllipse[i].angle); //Convierte el angulo de la elipse de grados a radianes
            cv::ellipse(frame, minEllipse[i], cv::Scalar(0,0,255),2);//Pinta la elipse en pantalla
            cv::putText(frame, cv::format("No. Paramecium: %ld", minEllipse.size()), {10,25}, 1,2, cv::Scalar(0,255,255),3, 8); //Pinta el numero de paramecios totales en pantalla
            cv::drawMarker(frame, minEllipse[i].center, cv::Scalar(0,0,255), 0,10); // Pinta una cruz en el centro del paramecio
            cv::putText(frame, cv::format("([%.2f, %.2f], %.2frad)",minEllipse[i].center.x,minEllipse[i].center.y, Erad) ,
                        minEllipse[i].center, 1 ,1.3,cv::Scalar(255,255,100),2, cv::LINE_AA); // Pinta las coordenadas (x,y) y el angulo del paramecio
            
            
            //std::cout<<"CENTROS EN EL "<<count<<" FRAME: "<<minEllipse[i].center<<std::endl;
            
            //cv::line(frame, centers[0], centers[1], cv::Scalar(100,200,255), 10, cv::LINE_8, 0);

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
        //Muestra todo el proceso y el resultado final en frame
        cv::imshow("MainWindow", frame);
        cv::imshow("NormalizedWindow", NormalizeFrame);
        cv::imshow("thWindow", thFrame);
        cv::imshow("DWindow", DilFrame);
        cv::imshow("GrayWindow", frameGray);
        cv::imshow("CannyWindow", cannyFrame);        
        //cv::imshow("thWindow", thFrame);


        if(cv::waitKey(1) == 27){ //condicion de paro del video, si se aprieta la tecla esc para el video
            break;
        }
        x<<"\n";
        y<<"\n";
        angle<<"\n"; 
        ++count; //contador de frames
    }
    video.release(); //cierra el video
    cv::destroyAllWindows(); //destruye todas las ventanas creadas

    return 0;
}