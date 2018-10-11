#include "CImg.h"
#include <iostream>
#include <vector>
using namespace cimg_library;
using namespace std;

typedef struct voxel
{
	int x;
	int y;
	int z;
} voxel;

int segment(CImg<float> img, vector<voxel> neighbors, vector<voxel> saved, int intensity){
	
}

int main(int argc, char** argv){

	if(argc < 2){
		printf("Usage: filename\n");
		exit(1);
	}

	char* filename = argv[1];
	float voxelSize[3];
	CImg<float> img;
	vector<voxel> neighbors;
	vector<voxel> saved;
	int intensity;

	img.load_analyze(filename, voxelSize);

	CImgDisplay main_display(img, "Liver segmentation");
    int slice_index = img.depth()/2;
    CImg<> slice = img.get_slice(slice_index);   
    main_display.display(slice);

	while (!main_display.is_closed()) {
		main_display.wait();
        if(main_display.wheel()){
        	// On change la coupe en fonction de l'action de la molette
            int counter = main_display.wheel();
            slice_index += counter;
            if(slice_index < 0) 
            	slice_index = 0;
            if(slice_index >= img.depth()) 
            	slice_index = img.depth()-1;

            cout << "Affichage de la coupe " << slice_index+1 << endl;

            // On affiche la nouvelle coupe
            slice = img.get_slice(slice_index);
            main_display.display(slice);

            // On remet la molette à 0 pour faciliter sa lecture
            main_display.set_wheel();
        }
        // Si on clique avec le bouton gauche de la souris
        else if(main_display.button()){
        	int x = main_display.mouse_x() * img.width() / main_display.width();
        	int y = main_display.mouse_y() * img.height() / main_display.height();
        	intensity = img(x, y, slice_index);
        	cout << "Mouse position (" << x << "," << y << ")" << " val : " << intensity << endl;

        	voxel tmp;
        	tmp.x = x;
        	tmp.y = y;
        	tmp.z = slice_index;
        	neighbors.push_back(tmp);
        	segment(img, neighbors, saved, intensity);

        }
	}

	return 0;
}

//g++ -o tp2.exe tp2.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11