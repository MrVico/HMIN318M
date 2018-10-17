#include "CImg.h"
#include <iostream>
#include <vector>
#include <sstream>
using namespace cimg_library;
using namespace std;

typedef struct voxel
{
	int x;
	int y;
	int z;
} Voxel;

CImg<> delete_below_intensity(CImg<> img, int intensity){
	for(int i=0; i<img.width()*img.height()*img.depth(); i++){
		if(img[i] < intensity){
			img[i] = 0;
		}
	}
	cout << "Process done" << endl;
	return img;
}

CImg<> extract_region(CImg<> img, Voxel voxel, float opacity, float tolerance){
    printf("draw_fill(%d,%d,%d) opacity:%d tolerance:%d\n", voxel.x, voxel.y, voxel.z, (int)opacity, (int)tolerance);
    unsigned char color[] = { 255, 255, 255 };
    CImg<> region;
    img.draw_fill(voxel.x, voxel.y, voxel.z, color, opacity, region, tolerance);
    int counter = 0;
    for(int i=0; i < img.width()*img.height()*img.depth(); i++) {
        if(region[i] != 0) 
            counter++;
    }
    printf("Counter: %d\n", counter);
    return region;
}

int get_voxel_amount(CImg<> region){
    int counter = 0;
    for(int i=0; i < region.width()*region.height()*region.depth(); i++) {
        if(region[i] != 0) 
            counter++;
    }
    return counter;
}

CImg<> compute_graph(CImg<> img, Voxel voxel, float opacity, int nbOfSamples){
    CImg<> graph_img(1,nbOfSamples,1,1,0);
    CImg<> tmp;
    for(int i=0; i<nbOfSamples; i++){
        tmp = extract_region(img, voxel, opacity, i);
        int amount = get_voxel_amount(tmp);
        printf("Amount: %d\n", amount);
        graph_img(0,i) = amount;
    }
    return graph_img;
}


int main(int argc, char** argv){

	if(argc != 4){
		printf("Usage: filename opacity tolerance\n");
		exit(1);
	}

	char* filename = argv[1];
	int opacity = atoi(argv[2]);
	int tolerance = atoi(argv[3]);
	float voxelSize[3];
	CImg<> img, selectedImg;
	vector<voxel> neighbors;
	vector<voxel> saved;
	int intensity;

	img.load_analyze(filename, voxelSize);

	CImgDisplay main_display(img, "Liver segmentation");
    CImg<> visu(500, 400, 1, 3, 0);
    const unsigned char red[] = {255, 0, 0};
    CImgDisplay graph_display(visu, "Graph");
    int slice_index = 21;
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

            /*
			stringstream ss;
			ss << "Liver segmentation (" << (slice_index+1) << "/" << img.depth() << ")";
			const char* title = ss.str().c_str();
            main_display.set_title(title);
			*/

            cout << "Affichage de la coupe " << slice_index+1 << endl;

            // On affiche la nouvelle coupe
            slice = img.get_slice(slice_index);
            main_display.display(slice);

            // On remet la molette à 0 pour faciliter sa lecture
            main_display.set_wheel();
        }
        // Si on clique avec le bouton gauche de la souris
        else if(main_display.button()){
        	Voxel voxel;
        	voxel.x = main_display.mouse_x() * img.width() / main_display.width();
        	voxel.y = main_display.mouse_y() * img.height() / main_display.height();
        	voxel.z = slice_index;
        	intensity = img(voxel.x, voxel.y, voxel.z);
        	cout << "Mouse position (" << voxel.x << "," << voxel.y <<  "," << voxel.z << ")" << " val : " << intensity << endl;

        	img = delete_below_intensity(img, intensity);

        	slice = img.get_slice(slice_index);
        	main_display.display(slice);

            selectedImg = extract_region(img, voxel, opacity, tolerance);

			main_display.display(selectedImg.get_slice(slice_index)); 

            int nbOfSamples = 30;
            CImg<> graph_img = compute_graph(img, voxel, opacity, nbOfSamples);

            graph_img.display_graph(graph_display, 2, 1, "Tolerance", 0, nbOfSamples, "NbVox", 0, 200000, true);
        
            while(!graph_display.is_closed()){
                if(graph_display.is_keyS()){
                    tolerance = graph_display.mouse_x() * ((float)nbOfSamples/graph_display.width());
                    graph_display.close();
                }
            }

            selectedImg = extract_region(img, voxel, opacity, tolerance);
            main_display.display(selectedImg.get_slice(slice_index)); 
            main_display.display(selectedImg.get_slice(slice_index)); 
        }
	}

	return 0;
}

//g++ -o tp2.exe tp2.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11