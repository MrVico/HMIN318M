#include "CImg.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
using namespace cimg_library;
using namespace std;


typedef struct coordinateSum
{
    int x = 0;
    int y = 0;
    int z = 0;
    int size = 0;
} CoordinateSum;

typedef struct voxel
{
    float x;
    float y;
    float z;
} Voxel;

int computeCentroids(int index, char* baseName){
    stringstream filenameSS;
    filenameSS << "DATA/" << baseName << "-" << index << ".hdr";
    char* filename = strdup(filenameSS.str().c_str());
    cout << "Processing " << filename << "..." << endl;

    float voxelSize[3];
    CImg<> img;
    img.load_analyze(filename, voxelSize);

    img = img.get_blur_median(2);

    img.threshold(25);

    img.erode(3, 3, 3);
    img.dilate(3, 3, 3);

    img = img.get_label();

    int numberOfLabels = 200;
    CoordinateSum coordinateSum[numberOfLabels];

    for(int x=0; x<img.width(); x++){
        for(int y=0; y<img.height(); y++){
            for(int z=0; z<img.depth(); z++){
                int label = (int) img(x, y, z);
                if(label > numberOfLabels)
                    cout << "Shit the label is " << label << endl;
                coordinateSum[label].x += x;
                coordinateSum[label].y += y;
                coordinateSum[label].z += z;
                coordinateSum[label].size += 1;
            }
        }
    }

    stringstream resFilenameSS;
    resFilenameSS /*<< "Centroids/" */<< baseName << "-" << index << " centroids.dat";
    filename = strdup(resFilenameSS.str().c_str());
    FILE* file = fopen(filename, "w+");
    for(int i=0; i<numberOfLabels; i++){
        int size = coordinateSum[i].size;
        if(size > 0){
            fprintf(file, "%f %f %f\n", (float)coordinateSum[i].x/size, 
                (float)coordinateSum[i].y/size, (float)coordinateSum[i].z/size);
        }
    }
    fclose(file);
}

float getDistance(Voxel voxel1, Voxel voxel2){
    //cout << "One: " << voxel1.x << " " << voxel1.y << " " << voxel1.z << endl;
    //cout << "Two: " << voxel2.x << " " << voxel2.y << " " << voxel2.z << endl;
    return (float)sqrt(pow(voxel1.x-voxel2.x, 2) + pow(voxel1.y-voxel2.y, 2) + pow(voxel1.z-voxel2.z, 2));
}

int computeDistances(char* baseName, int n1, int n2){
    stringstream firstFile, secondFile;
    firstFile << baseName << "-" << n1 << " centroids.dat";
    secondFile << baseName << "-" << n2 << " centroids.dat";
    char* filename1 = strdup(firstFile.str().c_str());
    char* filename2 = strdup(secondFile.str().c_str());
    
    stringstream resFile;
    resFile << "dist" << " " << n1 << " " << n2 << ".dat";
    char* filename = strdup(resFile.str().c_str());
    FILE* res = fopen(filename, "w+");

    ifstream file1(filename1);
    int counter1 = 0;
    string line1, line2;
    while(getline(file1, line1)){
        istringstream iss1(line1);
        Voxel voxel1;
        //error
        if(!(iss1 >> voxel1.x >> voxel1.y >> voxel1.z))
            break;

        counter1++;
        //cout << "Line " << counter1 << endl;

        // get the shortest distance
        Voxel voxel;
        float minDistance = 10000000;
        ifstream file2(filename2);
        int counter2 = 0;
        int matchIndex = 0;
        while(getline(file2, line2)){
            istringstream iss2(line2);
            Voxel voxel2;
            //error
            if(!(iss2 >> voxel2.x >> voxel2.y >> voxel2.z))
                break;
            counter2++;
            float distance = getDistance(voxel1, voxel2);
            if(distance < minDistance){
                minDistance = distance;
                voxel = voxel2;
                matchIndex = counter2;
                //cout << "Voxel: " << voxel.x << " " << voxel.y << " " << voxel.z << endl;
            }
        }
        //cout << "Voxel: " << voxel.x << " " << voxel.y << " " << voxel.z << endl;
        //fprintf(res, "%f %f %f\n", voxel.x-voxel1.x, voxel.y-voxel1.y, voxel.z-voxel1.z);
        fprintf(res, "%d\n", matchIndex);
    }
    fclose(res);
}

int main(int argc, char** argv){

	if(argc != 2){
		printf("Usage: baseFilePath \n");
		exit(1);
	}

	char* baseFilename = argv[1];
	float voxelSize[3];
	CImg<> baseImg/*, bluredImage*/;

    /*
	baseImg.load_analyze(baseFilename, voxelSize);

	CImgDisplay main_display(baseImg, "TP4");
    */
/*
    for(int i=0; i<=20; i++){
        computeCentroids(i, baseFilename);
    }
*/
    cout << "Done" << endl;

    for(int i=0; i<20; i++){
        computeDistances(baseFilename, i, i+1);
    }

    //CImgDisplay second_display(baseImg, "Without blur");

    //second_display.display(slice);  

    /*
    cout << "Processing..." << endl;

    baseImg = baseImg.get_blur_median(2);

    baseImg.threshold(25);

    baseImg.erode(3, 3, 3);
    baseImg.dilate(3, 3, 3);

    baseImg = baseImg.get_label();

    int numberOfLabels = 150;
    CoordinateSum coordinateSum[numberOfLabels];

    for(int x=0; x<baseImg.width(); x++){
        for(int y=0; y<baseImg.height(); y++){
            for(int z=0; z<baseImg.depth(); z++){
                int label = (int) baseImg(x, y, z);
                coordinateSum[label].x += x;
                coordinateSum[label].y += y;
                coordinateSum[label].z += z;
                coordinateSum[label].size += 1;
            }
        }
    }

    FILE* file = fopen("centroids.dat", "w+");
    for(int i=0; i<numberOfLabels; i++){
        int size = coordinateSum[i].size;
        if(size > 0){
            fprintf(file, "%d [%f, %f, %f]\n", i, (float)coordinateSum[i].x/size, 
                (float)coordinateSum[i].y/size, (float)coordinateSum[i].z/size);
        }
    }
    fclose(file);
    */

    /*
    int slice_index = 21;
    CImg<> slice = baseImg.get_slice(slice_index); 
    main_display.display(slice);

    cout << "Processing finished" << endl;

	while (!main_display.is_closed()) {
		main_display.wait();
        if(main_display.wheel()){
        	// On change la coupe en fonction de l'action de la molette
            int counter = main_display.wheel();
            slice_index += counter;
            if(slice_index < 0) 
            	slice_index = 0;
            if(slice_index >= baseImg.depth()) 
            	slice_index = baseImg.depth()-1;

            cout << "Affichage de la coupe " << slice_index+1 << endl;

            // On affiche la nouvelle coupe
            slice = baseImg.get_slice(slice_index);
            main_display.display(slice);

            // On remet la molette a 0 pour faciliter sa lecture
            main_display.set_wheel();
        }
	}
    */

	return 0;
}

//g++ -o tp4.exe tp4.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11