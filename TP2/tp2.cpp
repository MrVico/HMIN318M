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

// Supprime les voxels d'intensite inferieure pour la fonction draw_fill()
CImg<> delete_below_intensity(CImg<> img, int intensity){
	for(int i=0; i<img.width()*img.height()*img.depth(); i++){
		if(img[i] < intensity){
			img[i] = 0;
		}
	}
	return img;
}

// Extrait la region en fonction d'un voxel source
CImg<> extract_region(CImg<> img, Voxel voxel, int opacity, int tolerance){
    printf("Extracting given region with a tolerance of %d\n", tolerance);
    unsigned char color[] = { 255, 255, 255 };
    CImg<> region;
    img.draw_fill(voxel.x, voxel.y, voxel.z, color, opacity, region, tolerance);
    return region;
}

// Recupere le nombre de voxels d'une region
int get_voxel_amount(CImg<> region){
    int counter = 0;
    for(int i=0; i < region.width()*region.height()*region.depth(); i++) {
        if(region[i] != 0) 
            counter++;
    }
    return counter;
}

// Construit l'image utilise par le graphe
CImg<> compute_graph(CImg<> img, Voxel voxel, int opacity, int nbOfSamples){
    CImg<> graph_img(1,nbOfSamples,1,1,0);
    CImg<> tmp;
    for(int i=0; i<graph_img.height(); i++){
        tmp = extract_region(img, voxel, opacity, i);
        graph_img(0,i) = get_voxel_amount(tmp);
    }
    return graph_img;
}

int get_max(CImg<> img){
    int max = 0;
    for(int i=0; i<img.height(); i++){
        if(img[i] > max)
            max = img[i];
    }
    return max;
}


int main(int argc, char** argv){

	if(argc != 3){
		printf("Usage: filename tolerance\n");
		exit(1);
	}

	char* filename = argv[1];
    int tolerance = atoi(argv[2]);
	int opacity = 1;
	float voxelSize[3];
	CImg<> baseImg, regionImg;
	int intensity;

	baseImg.load_analyze(filename, voxelSize);
    regionImg = baseImg;

	CImgDisplay main_display(baseImg, "Liver segmentation");
    CImg<> visu(500, 400, 1, 3, 0);
    int slice_index = 21;
    CImg<> slice = baseImg.get_slice(slice_index);   
    main_display.display(slice);

	while (!main_display.is_closed()) {
		main_display.wait();
        if(main_display.wheel()){
        	// On change la coupe en fonction de l'action de la molette
            int counter = main_display.wheel();
            slice_index += counter;
            if(slice_index < 0) 
            	slice_index = 0;
            if(slice_index >= regionImg.depth()) 
            	slice_index = regionImg.depth()-1;

            cout << "Affichage de la coupe " << slice_index+1 << endl;

            // On affiche la nouvelle coupe
            slice = regionImg.get_slice(slice_index);
            main_display.display(slice);

            // On remet la molette a 0 pour faciliter sa lecture
            main_display.set_wheel();
        }
        // Si on clique avec le bouton gauche de la souris
        else if(main_display.button()){
        	Voxel voxel;
        	voxel.x = main_display.mouse_x() * regionImg.width() / main_display.width();
        	voxel.y = main_display.mouse_y() * regionImg.height() / main_display.height();
        	voxel.z = slice_index;
        	intensity = baseImg(voxel.x, voxel.y, voxel.z);

            // Vu que draw_fill prend egalement la tolerance en-dessous, ce qu'on ne veut pas
        	regionImg = delete_below_intensity(baseImg, intensity);
            regionImg = extract_region(baseImg, voxel, opacity, tolerance);
            // On affiche la region extraite choisie par l'utilisateur
			main_display.display(regionImg.get_slice(slice_index)); 

            // Le seuil varie de 0 a cette valeur pour le graphe
            int nbOfSamples = 100;
            printf("Computing best possible region...\n");
            // On calcule les differentes regions possible
            CImg<> graph_img = compute_graph(baseImg, voxel, opacity, nbOfSamples);

            CImgDisplay graph_display(visu, "Graph");
            // Construit le graphe avec la tolerance en abscisse et le nombre de voxels par region en ordonnee
            graph_img.display_graph(graph_display, 2, 1, "Tolerance", 0, nbOfSamples, "Amount", 0, get_max(graph_img), true);
        
            while(!graph_display.is_closed()){
                // En appuyant sur T on remplace la tolerance actuelle par le X du graphe de la position de la souris
                if(graph_display.is_keyT()){
                    tolerance = graph_display.mouse_x() * ((float)nbOfSamples/graph_display.width());
                    graph_display.close();
                }
            }

            // On recupere et affiche la nouvelle region
            regionImg = extract_region(baseImg, voxel, opacity, tolerance);
            main_display.display(regionImg.get_slice(slice_index)); 
        }
        // On affiche a nouveau l'image de base
        else if(main_display.is_keyR()){
            regionImg = baseImg;
            main_display.display(regionImg.get_slice(slice_index));
        }
        // On sauvegarde l'image
        else if(main_display.is_keyS()){
            regionImg.save_analyze("region.hdr", voxelSize);
        }
	}

	return 0;
}

//g++ -o tp2.exe tp2.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11