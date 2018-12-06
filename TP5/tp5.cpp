#include "CImg.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
using namespace cimg_library;
using namespace std;

typedef struct Vec3
{
    float x;
    float y;
    float z;
} Vec3;

float compute_movement(float g, float f, float gradF){
    if(gradF != 0)
        return ((g-f)*gradF)/(gradF*gradF); 
    return 0;
}

int main(int argc, char** argv){

	if(argc != 3){
		printf("Usage: <pathImgF> <pathImgG> \n");
		exit(1);
	}

	char* pathF = argv[1];
    char* pathG = argv[2];

    CImg<> ImgF, ImgG;
    float voxelSize[3];

    ImgF.load_analyze(pathF, voxelSize);
    ImgG.load_analyze(pathG, voxelSize);

    CImg<> deformations(ImgF.width(), ImgF.height(), ImgF.depth(), 3);
    CImgList<> list = ImgF.get_gradient("xyz", 3);

    CImg<> resImg;

    int nbIterations = 0;
    int maxNbIterations = 1;

    while(nbIterations < maxNbIterations){        
        // Stockage de la deformation
        for(int i=0; i<ImgF.size(); i++){
            if(ImgG[i] > ImgF[i]){
                deformations(i, 0) = list[0][i];
                deformations(i, 1) = list[1][i];
                deformations(i, 2) = list[2][i];
            }
            else{
                deformations(i, 0) = 0;
                deformations(i, 1) = 0;
                deformations(i, 2) = 0;
            }
        }

        deformations.blur(3, 3, 3, true, true);

        resImg = ImgG;

        for(int x=0; x<ImgG.width(); x++){
            for(int y=0; y<ImgG.height(); y++){
                for(int z=0; z<ImgG.depth(); z++){
                    //int index = resImg.offset(x, y, z);
                    //resImg(x + deformations(index, 0), y + deformations(index, 1), z + deformations(index, 2)) = ImgG[index];
                    //resImg(x, y, z) = ImgF(x + deformations(x, y, z, 0), y + deformations(x, y, z, 1), z + deformations(x, y, z, 2));
                    resImg(x, y, z) = ImgF(x + compute_movement(ImgG(x, y, z), ImgF(x, y, z), deformations(x, y, z, 0)), 
                        y + compute_movement(ImgG(x, y, z), ImgF(x, y, z), deformations(x, y, z, 1)), 
                        z + compute_movement(ImgG(x, y, z), ImgF(x, y, z), deformations(x, y, z, 2)));
                }
            }
        }

        ImgG = resImg;
        nbIterations++;
    }

    CImg<> F = ImgF;
    CImg<> G = ImgG;

    CImgDisplay F_display(512, 512, "Image F");
    CImgDisplay G_display(512, 512, "Image G");
    int slice_index = 21;
    CImg<> slice_F = F.get_slice(slice_index); 
    CImg<> slice_G = G.get_slice(slice_index);
    F_display.display(slice_F);  
    G_display.display(slice_G);

    while (!F_display.is_closed() && !G_display.is_closed()) {
        F_display.wait();
        if(F_display.wheel()){
            // On change la coupe en fonction de l'action de la molette
            int counter = F_display.wheel();
            slice_index += counter;
            if(slice_index < 0) 
                slice_index = 0;
            if(slice_index >= F.depth()) 
                slice_index = F.depth()-1;

            cout << "Affichage de la coupe " << slice_index+1 << endl;

            // On affiche la nouvelle coupe
            slice_F = F.get_slice(slice_index);
            slice_G = G.get_slice(slice_index);

            F_display.display(slice_F);
            G_display.display(slice_G);

            // On remet la molette a 0 pour faciliter sa lecture
            F_display.set_wheel();
        }
    }

	return 0;
}

//g++ -o tp5.exe tp5.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11