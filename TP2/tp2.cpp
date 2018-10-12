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

CImg<> extract_region(CImg<> img, voxel voxel, float opacity, float sigma){
    printf("draw_fill(%d,%d,%d) opacity:%f tolerance:%f\n", voxel.x, voxel.y, voxel.z, opacity, sigma);
    unsigned char color[] = { 255, 255, 255};
    CImg<> region = img;

    img.draw_fill(voxel.x, voxel.y, voxel.z, color, opacity, region, sigma, false);
    int counter = 0;
    for(int i = 0; i < img.width()*img.height()*img.depth(); i++) {
        if(region[i] > 0) 
        	counter++;
    }
    printf("%d, end drawfill\n", counter);
    return region;
}

void draw_graph(CImg<> image){
	int y = 50;
	CImg<> visu(500,400,1,3,0);
	CImgDisplay draw_disp(visu,"Intensity profile");
	const unsigned char red[] = { 255,0,0 }, green[] = { 0,255,0 }, blue[] = { 0,0,255 };

	while(!draw_disp.is_closed()){		
		visu.fill(0).draw_graph(image.get_crop(0,y,0,0,image.width()-1,y,0,0),red,1,1,0,255,0);
		visu.draw_graph(image.get_crop(0,y,0,1,image.width()-1,y,0,1),green,1,1,0,255,0);
		visu.draw_graph(image.get_crop(0,y,0,2,image.width()-1,y,0,2),blue,1,1,0,255,0).display(draw_disp);
	}
}

int main(int argc, char** argv){

	if(argc < 5){
		printf("Usage: filename x y z\n");
		exit(1);
	}

	char* filename = argv[1];
	int vx = atoi(argv[2]);
	int vy = atoi(argv[3]);
	int vz = atoi(argv[4]);
	float voxelSize[3];
	CImg<> img;
	vector<voxel> neighbors;
	vector<voxel> saved;
	int intensity;

	img.load_analyze(filename, voxelSize);

	CImgDisplay main_display(img, "Liver segmentation");
    int slice_index = 25;
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

            img = extract_region(img, voxel, vx/100, vy);

			main_display.display(img.get_slice(slice_index));
        }
        else if(main_display.is_key0()){
        	draw_graph(img);
        }
	}

	return 0;
}

//g++ -o tp2.exe tp2.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11