#include "CImg.h"
using namespace cimg_library;

//g++ -o tp1.exe tp1.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11

int main(int argc, char** argv){

	if(argc < 4){
		printf("Usage: filename threshold p\n");
		exit(1);
	}

	char* filename = argv[1];
	int threshold = atoi(argv[2]);
	int p = atoi(argv[3]);

	CImg<float> img;
	float voxelSize[3];;

	// On recupere les informations pratiques de l'image
	img.load_analyze(argv[1], voxelSize);

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

	return 0;
}