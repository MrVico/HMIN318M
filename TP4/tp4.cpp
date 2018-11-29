#include "CImg.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
using namespace cimg_library;
using namespace std;

typedef struct voxel
{
    float x;
    float y;
    float z;
} Voxel;

typedef struct coordinateSum
{
    int x = 0;
    int y = 0;
    int z = 0;
    int size = 0;
} CoordinateSum;


int computeCentroids(int index, char* baseName){
    stringstream filenameSS;
    filenameSS << "DATA/" << baseName << "-" << index << ".hdr";
    char* filename = strdup(filenameSS.str().c_str());
    cout << "Processing " << filename << "..." << endl;

    float voxelSize[3];
    CImg<> img;
    img.load_analyze(filename, voxelSize);

    // application du filtre médian
    img = img.get_blur_median(2);

    // application du seuil
    img.threshold(25);

    // elimination des pixels isolés
    img.erode(3, 3, 3);
    img.dilate(3, 3, 3);

    // identification des cellules par composantes connexes
    img = img.get_label();

    int numberOfLabels = 200;
    CoordinateSum coordinateSum[numberOfLabels];
    // calcul des sommes des coordonnées pour chaque cellule/label
    for(int x=0; x<img.width(); x++){
        for(int y=0; y<img.height(); y++){
            for(int z=0; z<img.depth(); z++){
                int label = (int) img(x, y, z);
                coordinateSum[label].x += x;
                coordinateSum[label].y += y;
                coordinateSum[label].z += z;
                coordinateSum[label].size += 1;
            }
        }
    }

    // création du fichier stockant les barycentres des cellules
    stringstream resFilenameSS;
    resFilenameSS << baseName << "-" << index << " centroids.dat";
    filename = strdup(resFilenameSS.str().c_str());
    FILE* file = fopen(filename, "w+");
    for(int i=0; i<numberOfLabels; i++){
        int size = coordinateSum[i].size;
        if(size > 0){
            // calcul du barycentre de la cellule actuelle
            fprintf(file, "%f %f %f\n", (float)coordinateSum[i].x/size, 
                (float)coordinateSum[i].y/size, (float)coordinateSum[i].z/size);
        }
    }
    fclose(file);
}

float getDistance(Voxel voxel1, Voxel voxel2){
    return (float)sqrt(pow(voxel1.x-voxel2.x, 2) + pow(voxel1.y-voxel2.y, 2) + pow(voxel1.z-voxel2.z, 2));
}

int computeDistances(char* baseName, int n1, int n2){
    // Lecture des deux fichiers comprenant les barycentres des cellules
    stringstream firstFile, secondFile;
    firstFile << baseName << "-" << n1 << " centroids.dat";
    secondFile << baseName << "-" << n2 << " centroids.dat";
    char* filename1 = strdup(firstFile.str().c_str());
    char* filename2 = strdup(secondFile.str().c_str());
    
    stringstream resFile;
    resFile << "dist" << " " << n1 << " " << n2 << ".dat";
    char* filename = strdup(resFile.str().c_str());
    FILE* res = fopen(filename, "w+");

    // Dans mon cas, avec le seuil que j'ai appliqué,
    // la première image n'a que 104 cellules
    // on ne va donc suivre que ces 104 cellules
    int maxIndex = 104;

    ifstream file1(filename1);
    int counter1 = 0;
    string line1, line2;
    while(getline(file1, line1)){
        counter1++;
        // Pour ne pas prendre des cellules qui n'existent pas dans toutes les images
        if(counter1 <= maxIndex){
            istringstream iss1(line1);
            Voxel voxel1;
            // error
            if(!(iss1 >> voxel1.x >> voxel1.y >> voxel1.z))
                break;

            // On cherche la distance minimale
            Voxel voxel;
            float minDistance = 10000000;
            ifstream file2(filename2);
            int counter2 = 0;
            int matchIndex = 0;
            while(getline(file2, line2)){
                counter2++;
                // Pour ne pas prendre des cellules qui n'existent pas dans toutes les images
                if(counter2 <= maxIndex){
                    istringstream iss2(line2);
                    Voxel voxel2;
                    // error
                    if(!(iss2 >> voxel2.x >> voxel2.y >> voxel2.z))
                        break;

                    float distance = getDistance(voxel1, voxel2);
                    if(distance < minDistance){
                        minDistance = distance;
                        voxel = voxel2;
                        matchIndex = counter2;
                    }
                }
            }
            // On écrit l'index correspondant à la cellule courante dans l'image suivante
            fprintf(res, "%d\n", matchIndex);
        }
    }
    fclose(res);
}

// Construit le fichier contenant les voxels de la trajectoire de la cellule
int getPathFile(char* baseName, int cellIndex, int path[]){
    stringstream resFile;
    resFile << "Path " << cellIndex << ".obj";
    char* filename = strdup(resFile.str().c_str());
    FILE* res = fopen(filename, "w+");

    // On parcourt les fichier contenants les barycentres
    for(int i=0; i<=21; i++){
        stringstream fileStream;
        fileStream << baseName << "-" << i << " centroids.dat";
        filename = strdup(fileStream.str().c_str());
        ifstream file(filename);
        string line;
        int counter = 0;
        while(getline(file, line)){
            counter++;
            // Si le numéro de la ligne correspond au chemin on ajoute la coordonnée au fichier
            if(counter == path[i]){
                istringstream iss(line);
                Voxel voxel;
                // error
                if(!(iss >> voxel.x >> voxel.y >> voxel.z))
                    break;
                fprintf(res, "v %f %f %f\n", voxel.x, voxel.y, voxel.z);
            }
        }
    }

    // On ajoute la dernière ligne dans le fichier pour que ParaView puisse le lire
    fprintf(res, "l ");
    for(int i=1; i<=21; i++){
        fprintf(res, "%d ", i);
    }
    fprintf(res, "\n");
    fclose(res);
}

// Construit le chemin contenant les indexes
int getPath(char* baseName){
    int cellPaths[104][20];
    for(int i=0; i<20; i++){
        stringstream firstFile;
        firstFile << "dist " << i << " " << (i+1) << ".dat";
        char* filename = strdup(firstFile.str().c_str());
        ifstream file(filename);
        string line;
        int counter = 0;
        int tmp[104];
        while(getline(file, line)){
            istringstream iss(line);
            int index;
            // error
            if(!(iss >> index))
                break;

            // La première fois, vu qu'il n'y a rien à comparer,
            // on ajoute juste la valeur dans le tableau
            if(i == 0){
                cellPaths[counter][i] = index;
            }
            // Sinon on le stocke dans un tableau temporaire pour plus tard
            else{
                tmp[counter] = index; 
            }
            counter++;
        }
        // Si on ne se trouve pas dans la première image
        if(i != 0){
            for(int j=0; j<counter; j++){
                // On ajoute le bon index en regardant à l'index du tableau temporaire
                // qui correspond à l'index précédent du chemin
                cellPaths[j][i] = tmp[cellPaths[j][i-1]-1];
            }
        }
    }    

    for(int i=0; i<104; i++){
        int path[21];
        // On n'oublie pas de stocker le première indice du chemin de la cellule
        // qui n'est au final que le numéro de la ligne
        path[0] = (i+1);
        for(int j=1; j<21; j++){
            path[j] = cellPaths[i][j-1];
        }
        // On crée un fichier .obj pour chaque cellule
        getPathFile(baseName, (i+1), path);
    }
}

int main(int argc, char** argv){

	if(argc != 2){
		printf("Usage: baseFilePath \n");
		exit(1);
	}

	char* baseFilename = argv[1];


    for(int i=0; i<=20; i++){
        computeCentroids(i, baseFilename);
    }

    for(int i=0; i<20; i++){
        computeDistances(baseFilename, i, i+1);
    }

    getPath(baseFilename);
	return 0;
}

//g++ -o tp4.exe tp4.cpp -O2 -L/usr/X11R6/lib -lm -lpthread -lX11