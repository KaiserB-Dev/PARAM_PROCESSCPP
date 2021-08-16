
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

double m(double x1, double x2, double y1, double y2);
double angle_rect(double m1, double m2);
double rad_deg(double rad);



double m(double x1, double x2, double y1, double y2){
    double m = (x2-x1)/(y2-y1);

    return m;
}

double angle_rect(double m1, double m2){

    double theta = atan((m2-m1)/(1+(m2*m1)));
    return theta;
}

double rad_deg(double rad){
    double deg;
    deg = rad*(180/M_PI);
    return deg;
}







int main(int argc, char** argv){

    const std::string keys_args = "{help h ?   |           | print the help for this program}"
                                  "{@input     | stuff.wmv | input video in wmv format}"
                                  "{fN file_name |default| string for data files }"
                                  "{delay d    | 1 | set the fps or delay in video}";

    cv::CommandLineParser parser( argc, argv, keys_args ); //Parser para obtener y leer el video desde los argumentos de ejecución del programa
    std::string x_data_name = "./data_files/x_";
    std::string y_data_name = "./data_files/y_";
    std::string angle_data_name = "./data_files/angle_";

    if(parser.has("help") || parser.has("?") || parser.has("h")){
        parser.printMessage();
        return 0;
    }
    if(!parser.check()){
        parser.printErrors();
        parser.printMessage();
        return 0;
    }

    x_data_name += parser.get<std::string>("file_name");
    x_data_name += ".csv";
    y_data_name += parser.get<std::string>("file_name");
    y_data_name += ".csv";
    angle_data_name += parser.get<std::string>("file_name");
    angle_data_name += ".csv";

    double m1, m2;


    std::clock_t start = std::clock();
    cv::VideoCapture video;
    cv::Mat frame, frameGray, NormalizeFrame, thFrame, DilFrame, cannyFrame; //Definiendo matrices vacias
    cv::Mat kernel;
    kernel.ones(cv::Size(10,10), 8);
    std::ofstream x(x_data_name);
    std::ofstream y(y_data_name);
    std::ofstream angle(angle_data_name);
    //std::cout<<start<<std::endl;

    cv::namedWindow("MainWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("GrayWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("NormalizedWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("thWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("DWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("EWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("OWindow", cv::WINDOW_NORMAL);
    cv::namedWindow("CannyWindow", cv::WINDOW_NORMAL);

    //std::string strCoords;
    std::vector<std::vector<cv::Point>> cnts; //[x,y]
    video = cv::VideoCapture(cv::samples::findFile(parser.get<std::string>( "@input" ) )); //Carga el input en el objeto video capture (se puede modificar para analizar paramecium en tiempo real con alguna camara como el dinolite)
    int total_frames = video.get(cv::CAP_PROP_FRAME_COUNT);
    int fps = video.get(cv::CAP_PROP_FPS);

    std::vector<cv::Mat> saved_frames(total_frames);



    if(!video.isOpened()){
        std::cout<<"No existe el video solicitado"<<std::endl;
        return 0;
    }
    

    int count = 0;
    while(true){
        video.read(frame);
        if(frame.empty()) break;   

        saved_frames.push_back(frame);

        //preprocessing part
        cv::cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY); //Converción de canal BGR a GRAY
        cv::normalize(frameGray, NormalizeFrame, 0, 255, cv::NORM_MINMAX); //Normalización del frame entre valores de 0 a 255 por el metodo de MINMAX
        cv::threshold(NormalizeFrame, thFrame, 50, 255, cv::THRESH_BINARY); //Binarización del frame, con un valor de thresh de 60 y un maxval de 255
        cv::dilate(thFrame, DilFrame, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(6,6))));//Dilatación de la imagen con una estructura especificada en tamaño y forma para la operacion morfologica.
        cv::imshow("DWindow", DilFrame);
        //MORPH_ELLIPSE es un elemento estructural eliptico el cual esta inscrito en un rectangulo[x,y,width,heigth].
        cv::erode(DilFrame, DilFrame, kernel, cv::Point(-1,-1), 2, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue()); //Esta parte es la eroción de la imagen previamente dilatada. lo hace con una kernel de unos;
        cv::imshow("EWindow", DilFrame);
        //El punto (-1,-1) indica que el punto de ancla de la eroción esta en el centro de cada elemento y este copia el borde original {PENDIENTE cv::morphologyDefaultBorderValue()}
        cv::morphologyEx(DilFrame, DilFrame, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE,(cv::Size(10,10)))); //Dilata la imagen y luego erosiona la imagen dilatada
        cv::imshow("OWindow", DilFrame);

        //fin preprocessing
        
        //contornos
        cv::Canny(DilFrame, cannyFrame, 50, 200); //Encuentra los bordes de una imagen por medio del algoritmo canny
        cv::findContours(cannyFrame, cnts,cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);  //En base a la imagen procesada con canny este busca los puntos de los bordes y los almacena en un vector de vectores de puntos

        std::vector<std::vector<cv::Point>> PolyAprox(cnts.size());

        //CLUSTERING DE LOS CONTORNOS CALCULADOS CON LOS PUNTOS CONCAVOS

        //cv::drawContours(frame, cnts, -1, cv::Scalar(0,0,255));
    
        std::vector<cv::RotatedRect> minEllipse(cnts.size()); //Con los puntos previamente calculados aproxima un rectangulo rotatorio (con el fin de calcular y aproximar la elipse con su respectivo angulo de rotación)
        for(int i = 0; i < cnts.size(); ++i){
            
            cv::approxPolyDP(cv::Mat(cnts[i]), PolyAprox[i], 4, true);

            minEllipse[i] = cv::minAreaRect(cnts[i]); //Calcula e inscribe el area mas pequeña del rectangulo en donde se inscribira la elipse
            if(cnts[i].size() > 5){
                minEllipse[i] = cv::fitEllipse(cnts[i]); //Inscribe la elipse en el rectangulo
            }
        }
        cv::putText(frame, cv::format("Frame: %d/%d", count+1, total_frames), {frame.rows+25, 25}, 1, 2, cv::Scalar(100,0,255),3,8); //Muestra los frames totales y el numero de frame en el que va el video
        cv::putText(frame, cv::format("FPS: %d", fps), {frame.rows+25, 50}, 1, 2, cv::Scalar(100,0,255),3,8); //Muestra los frames por segundo (FPS)
        for(int i=0; i<minEllipse.size(); ++i){
            if(minEllipse[i].size.area() > 100 ){ //3500 A
                cv::ellipse(frame, minEllipse[i], cv::Scalar(0,0,255),2);//Pinta la elipse en pantalla
                cv::putText(frame, cv::format("No. Paramecium: %ld", minEllipse.size()), {10,25}, 1,2, cv::Scalar(0,255,255),3, 8); //Pinta el numero de paramecios totales en pantalla
                cv::drawMarker(frame, minEllipse[i].center, cv::Scalar(0,0,255), 0,10); // Pinta una cruz en el centro del paramecio
                double realAngle = minEllipse[i].angle;  
               
                cv::putText(frame, cv::format("([%.3f, %.3f], %.2f deg NO. %i)",minEllipse[i].center.x,minEllipse[i].center.y, realAngle, i) ,
                        minEllipse[i].center, 1 ,1.3,cv::Scalar(255,255,100),2, cv::LINE_AA); // Pinta las coordenadas (x,y) y el angulo del paramecio     
            
                cv::drawContours(frame, PolyAprox, i, cv::Scalar(255,255,0), 2);
            

                for(int v=0; v<PolyAprox[i].size(); ++v ){
                    m1 = m(PolyAprox[i][v].x, PolyAprox[i][v+1].x, PolyAprox[i][v].y, PolyAprox[i][v+1].y);
                    m2 = m(PolyAprox[i][v+1].x, PolyAprox[i][v+2].x, PolyAprox[i][v+1].y, PolyAprox[i][v+2].y);
                    
                    std::cout<<i<<"]M1: "<<v<<": "<<m1<<std::endl;
                    std::cout<<i<<"]M2: "<<v<<": "<<m2<<std::endl;

                    std::cout<<i<<"]theta: "<<rad_deg(angle_rect(m1, m2))<<std::endl;

                    if(rad_deg(angle_rect(m1, m2))< 180 && rad_deg(angle_rect(m1, m2)) > 0) {
                        std::cout<<"CONCAVE"<<std::endl;
                    }else{
                        std::cout<<"CONVEX"<<std::endl;
                    }

                }
                
                /*
                for(int v=0; v<PolyAprox[0].size(); ++v ){
                    std::cout<<v<<": "<<m(PolyAprox[33][v].x, PolyAprox[33][v+1].x, PolyAprox[33][v].y, PolyAprox[33][v+1].y)<<std::endl;
                }*/

                //std::cout<<"PARAMECIO 33: "<<PolyAprox[33]<<std::endl;


                //std::cout<<"CENTROS EN EL "<<count<<" FRAME: "<<minEllipse[i].center<<std::endl;
                
                //cv::line(frame, centers[0], centers[1], cv::Scalar(100,200,255), 10, cv::LINE_8, 0);

               /*for(int j=0; j<PolyAprox[i].size(); ++j){
                    std::cout<<i<<": "<<PolyAprox[i][j]<<std::endl;
                }*/
             
                /*
            //std::cout<<minEllipse[i].center;
                std::cout<<"PUNTOS DE CONTORNO:"<<std::endl;
                std::cout<<i+1<<": "<<cnts[i]<<std::endl;
                std::cout<<"ELLIPSE SIZE (a y b), center & angle"<<std::endl;
                std::cout<<minEllipse[i].size<<std::endl;
                std::cout<<minEllipse[i].center<<std::endl;
                std::cout<<minEllipse[i].angle<<std::endl;*/
            

                //std::cout<<minEllipse[i].size.area()<<std::endl;

                x<<minEllipse[i].center.x<<",";
                y<<minEllipse[i].center.y<<",";
                angle<<realAngle<<",";
            

            }
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
        //cv::imshow("DWindow", DilFrame);
        cv::imshow("GrayWindow", frameGray);
        cv::imshow("CannyWindow", cannyFrame);  

        if(cv::waitKey(parser.get<int>("delay")) == 27){ //condicion de paro del video, si se aprieta la tecla esc para el video
            break;
        }
        x<<"\n";
        y<<"\n";
        angle<<"\n";
        
        ++count; //contador de frames
    }
    x.close();
    y.close();
    angle.close();
    video.release(); //cierra el video
    cv::destroyAllWindows(); //destruye todas las ventanas creadas

    return 0;
}