#include "CImg.h"
#include <iostream>
using namespace cimg_library;
using namespace std;

// Effectue la segmentation de l'image en fonction des paramètres passés en arguments
CImg<float> segmentation(char* filename, int threshold, int p){
	if(p < 3)
		p = 3;

	CImg<float> img;
	float voxelSize[3];

	img.load_analyze(filename, voxelSize);

	// On applique le seuil
	img.threshold(threshold);

	// On applique les p erosions
	for(int i=0; i<p; i++){
		img.erode(2,2,2);
	}

	// On label les zones pour pouvoir recuperer celle du cerveau
	img.label();

	int labels[2500];
	for(int i=0; i<2500; i++){
		labels[i] = 0;
	}

	int maxLabel = -1;
	int maxOcc = -1;
	// On cherche le label le plus present
	for(int i=0; i<img.width()*img.height()*img.depth(); i++){
		if(img[i] != 0){
			labels[(int)img[i]]++;
			if(labels[(int)img[i]] > maxOcc){
				maxOcc = labels[(int)img[i]];
				maxLabel = img[i];
			}
		}
	}

	// On affiche uniquement les pixels correspondant au label le plus present
	for(int i=0; i<img.width()*img.height()*img.depth(); i++){
		if(img[i] == maxLabel)
			img[i] = 255;
		else
			img[i] = 0;
	}

	// On applique les p dilatations
	for(int i=0; i<p; i++){
		img.dilate(2,2,2);
	}

	// On sauvegarde l'image resultante
	img.save_analyze("Results/res.hdr", voxelSize);

	return img;
}

/*
	Les touches + et - du pavé numérique augmente et baisse la valeur du seuil
	Les flèches du haut et du bas augmente et baisse la valeur de p, le nombre de transformations
*/
int main(int argc, char** argv){

	if(argc < 4){
		printf("Usage: filename threshold p\n");
		exit(1);
	}

	char* filename = argv[1];
	int threshold = atoi(argv[2]);
	int p = atoi(argv[3]);

	CImg<float> img = segmentation(filename, threshold, p);

	CImgDisplay main_display(img, "Brain segmentation");
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
        else if(main_display.is_keyPADADD()){
        	threshold += 1;
        }
        else if(main_display.is_keyPADSUB()){
        	threshold -= 1;
        }
        else if(main_display.is_keyARROWUP()){
        	p += 1;
        }
        else if(main_display.is_keyARROWDOWN()){
        	p -= 1;
        }

        // Dans ces cas là il faut mettre à jour le display
        if(main_display.is_keyPADSUB() or main_display.is_keyPADADD() or main_display.is_keyARROWUP() or main_display.is_keyARROWDOWN()){
        	cout << "Calcul en cours..." << endl;
        	img = segmentation(filename, threshold, p);
        	cout << "Nouvelle segmentation avec un seuil de " << threshold << " et " << p << " transformations." << endl;
            // On affiche la nouvelle coupe
            slice = img.get_slice(slice_index);
            main_display.display(slice);
        }
	}

	return 0;
}

//g++ -o tp1.exe tp1.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11