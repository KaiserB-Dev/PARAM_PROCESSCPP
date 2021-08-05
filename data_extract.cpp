#include "data_extract.hpp"


std::vector<double> dataExtract::extraer_data(std::string file){
    std::vector<double> data;
    double dat = 0;
    std::ifstream fe(file);
   

    while(fe >> dat){
        data.push_back(dat);
    }
        fe.close();

        for(int i=0;i<data.size();++i){
            std::cout<<data.at(i);
        }
        

        return data;
}


