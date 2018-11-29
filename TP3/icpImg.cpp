#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <string>
#include <math.h>
using namespace std;

#include "CImg.h"

#include "mesh.h"
#include "img.h"
#include "mathematics.h"

using namespace cimg_library;

static const unsigned char red[]={255,0,0},green[]={0,255,0},blue[]={0,0,255},lightblue[]={150,150,255},white[]={255,255,255},black[]={0,0,0},grey[]={128,128,128};

enum transformationType {affine, rigid, similitude};
enum Metric {SSD, NCC};


void computeCorrespondences(MESH& mesh, const CImg<float> &dist,const float l)
{
    unsigned int nbpoints=dist.height();
    unsigned int S=dist.width()/2;

    for(unsigned int i=0;i<nbpoints;i++)
    {
        float p[3];
        float tmpP[3];
        float n[3];
        mesh.getPoint(p, i);
        mesh.getNormal(n, i);
        int x;
        float min = 1000;
        // On cherche la distance minimale
        for(int j=0; j<dist.width(); j++){
            if(dist(j, i) < min){
                min = dist(j, i);
                x = j;
            }
        }
        // On applique la transformation au point
        for(int a=0; a<3; a++){
            tmpP[a] = p[a] + x * l * n[a];
        }
        mesh.setCorrespondence(tmpP, i);
    }
    
}


CImg<float> computeDistance(const CImg<unsigned char> &sourceProf,const CImg<unsigned char> &targetProf,const Metric metric)
{
    unsigned int nbpoints=sourceProf.height();
    // 15
    unsigned int N=sourceProf.width();
    // 10               35                  15
    int S=(targetProf.width()-sourceProf.width())/2;

    CImg<float> dist(2*S, nbpoints);
    dist.fill(0);

    if(metric == SSD)
    {
        float sum;
        for(int y=0; y<nbpoints; y++){
            for(int i=-S; i<S; i++){
                sum = 0;
                // Calcul de la somme des differences au carre
                for(int x=0; x<N; x++){
                    // +S dans target car l'image deborde de S a gauche et a droite
                    sum += pow(targetProf(x+i+S, y) - sourceProf(x, y), 2);
                }
                // On veut avoir la moyenne de la somme
                dist(i+S, y) = sum / (2*S);
            }
        }
    }
    else // NCC
    {
        float aveS = 0;
        float aveT = 0;
        float upperCC = 0;
        float lowerLeftCC = 0;
        float lowerRightCC = 0;
        for(int y=0; y<nbpoints; y++){
            // Calcul de la moyenne pour la source
            aveS = 0;
            for(int z=0; z<sourceProf.width(); z++){
                aveS += sourceProf(z, y);
            }
            aveS = aveS / sourceProf.width();
            // Calcul de la moyenne pour la target
            aveT = 0;
            for(int z=0; z<targetProf.width(); z++){
                aveT +=  targetProf(z, y);
            }
            aveT = aveT / targetProf.width();
            for(int i=-S; i<S; i++){
                upperCC = 0;
                for(int x=0; x<N; x++){
                    upperCC += (targetProf(x+i+S, y) - aveT) * (sourceProf(x, y) - aveS);
                }
                lowerLeftCC = 0;
                lowerRightCC = 0;
                for(int x=0; x<N; x++){
                    lowerLeftCC += pow(targetProf(x+i+S, y) - aveT, 2);
                    lowerRightCC += pow(sourceProf(x, y) - aveS, 2);
                }

                dist(i+S, y) = (upperCC) / sqrt(lowerLeftCC * lowerRightCC);
            }
        }
    }

    //dist.display();

    return dist;
}

CImg<unsigned char> computeProfiles(const MESH& mesh, const IMG<unsigned char,float>& img, const unsigned int Ni, 
    const unsigned int No, const float l,const unsigned int interpolationType=1)
{
    CImg<unsigned char> prof(Ni+No,mesh.getNbPoints());

    float p[3];
    float n[3];
    float tmpP[3];

    for(int i=0; i<mesh.getNbPoints(); i++){
        mesh.getPoint(p, i);
        mesh.getNormal(n, i);
        // On descend la normale
        for(int j=0; j<Ni; j++){
            // Pour chaque coordonnée on descend la normale avec le pas L
            for(int a=0; a<3; a++){
                tmpP[a] = p[a] - n[a] * l * j;
            }
            // On assigne la valeur au bon index, le plus en bas sera le plus à gauche
            prof(Ni-j-1, i) = img.getValue(tmpP, interpolationType);
        }
        // On monte la normale
        for(int j=0; j<No; j++){
            // Pour chaque coordonnée on monte la normale avec le pas L
            for(int a=0; a<3; a++){
                tmpP[a] = p[a] - n[a] * l * j;
            }
            // On assigne la valeur au bon index, le plus en haut sera le plus à droite
            prof(Ni+j, i) = img.getValue(tmpP, interpolationType);
        }
    }

    //prof.display();

    return prof;
}


void Registration(float A[3][3],  float t[3],const MESH& mesh, const transformationType &transformType)
{
    // get centroids
    float c0[]={0,0,0}, c[]={0,0,0}, N=0;
    for(unsigned int i=0;i<mesh.getNbPoints();i++)
    {
        float p[3];
        mesh.getPoint0(p,i); for(unsigned int j=0;j<3;j++) c0[j]+=mesh.getWeight(i)*p[j];
        mesh.getCorrespondence(p,i); for(unsigned int j=0;j<3;j++) c[j]+=mesh.getWeight(i)*p[j];
        N+=mesh.getWeight(i);
    }
    for(unsigned int j=0;j<3;j++) {c0[j]/=N; c[j]/=N;}

    // fill matrices
    float Q[][3]={{0,0,0},{0,0,0},{0,0,0}}, K[][3]={{0,0,0},{0,0,0},{0,0,0}},sx=0;
    for(unsigned int i=0;i<mesh.getNbPoints();i++)
    {
        float p0[3]; mesh.getPoint0(p0,i); for(unsigned int j=0;j<3;j++) p0[j]-=c0[j];
        float p[3]; mesh.getCorrespondence(p,i); for(unsigned int j=0;j<3;j++) p[j]-=c[j];
        for(unsigned int j=0;j<3;j++) {sx+=mesh.getWeight(i)*p0[j]*p0[j]; for(unsigned int k=0;k<3;k++) {Q[j][k]+=mesh.getWeight(i)*p0[j]*p0[k];  K[j][k]+=mesh.getWeight(i)*p[j]*p0[k];} }
    }

    // compute solution for affine part
    if(transformType==affine) { float Qinv[3][3]; Invert(Qinv,Q); Mult(A,K,Qinv); }
    else if(transformType==rigid)            ClosestRigid(K,A);
    else if(transformType==similitude)
    {
        ClosestRigid(K,A);
        float s=0; for(unsigned int j=0;j<3;j++) s+=A[j][0]*K[j][0]+A[j][1]*K[j][1]+A[j][2]*K[j][2];
        s/=sx;
        for(unsigned int j=0;j<3;j++) for(unsigned int k=0;k<3;k++) A[j][k]*=s;
    }

    // compute solution for translation
    Mult(t,A,c0); for(unsigned int j=0;j<3;j++) t[j]=c[j]-t[j];
}

int main(int argc,char **argv)
{
    // load data
    string meshFilename("data/femur_m.obj");    if(argc>=2)  meshFilename=string(argv[1]);
    string sourceFilename("data/thigh_m.mhd");  if(argc>=3)  sourceFilename=string(argv[2]);
    string targetFilename("data/thigh_f.mhd");    if(argc>=4)  targetFilename=string(argv[3]);

    MESH mesh;    mesh.LoadObj(meshFilename.c_str());
    IMG<unsigned char,float> source;    source.load_metaimage(sourceFilename.c_str());
    IMG<unsigned char,float> target;    target.load_metaimage(targetFilename.c_str());
    IMG<unsigned char,float> *current=&target;

    // compute source profiles
    const unsigned int Ni=10,No=5,S=10;
    float l=0.02;
    CImg<unsigned char> sourceProf = computeProfiles(mesh, source, Ni, No, l);

    // image display
    unsigned int coord[]={0,0,0}, // 3D coordinates corresponding to mouse position
            plane=0; // plane corresponding to mouse position
    int slice[]={source().width()/2,source().height()/2,source().depth()/2};   // current slices of the mpr visualisation

    bool redraw=true,display_trace=true;
    CImgDisplay disp(1024,768,"");       // display window

    // 3d view
    CImg<float> pose;
    float   Xoff = 0, Yoff = 0, Zoff = 0, sprite_scale = 1;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int renderMode = 4;
    const float focale = 500;
    bool showCorrespondences=true;
    bool showNormals=false;
    const float scaleNormal = 0.05;

    CImgDisplay disp3d(1024,768,"");       // display window
    CImg<unsigned char> visu0(disp3d.width(),disp3d.height(),1,3,255), visu;
    CImg<float> zbuffer(visu0.width(),visu0.height(),1,1,0);
    bool init_pose = true, clicked = false, redraw3d = true;
    disp3d.show().flush();

    transformationType transformType = rigid;
    Metric metric = SSD;

    while (!disp.is_closed() && !disp.is_keyQ() && !disp.is_keyESC()) // main loop
    {
        // image display
        disp.wait(1);
        if (disp.is_resized()) disp.resize();

        plane = 0;
        if(disp.mouse_x()>=0 && disp.mouse_y()>=0) // convert mouse position to 3d position
        {
            unsigned int dim[]={(*current)().width(),(*current)().height(),(*current)().depth()};
            const unsigned int mX = disp.mouse_x()*(dim[0]+dim[2])/disp.width(),mY = disp.mouse_y()*(dim[1]+dim[2])/disp.height();
            if (mX>=dim[0] && mY<dim[1]) { plane = 1; coord[1] = mY; coord[2] = mX - dim[0];   coord[0] = slice[0]; }
            else if (mX<dim[0] && mY>=dim[1]) { plane = 2; coord[0] = mX; coord[2] = mY - dim[1];   coord[1] = slice[1]; }
            else if (mX<dim[0] && mY<dim[1])       { plane = 3; coord[0] = mX; coord[1] = mY;     coord[2] = slice[2]; }
            else {plane = 0; coord[0] = coord[1] = coord[2] = 0;}
            redraw = true;
        }

        if (disp.wheel() && plane) // handle wheel interaction
        {
            unsigned int dim[]={(*current)().width(),(*current)().height(),(*current)().depth()};
            slice[plane-1]+=disp.wheel();
            if (slice[plane-1]<0) slice[plane-1] = 0;
            else if (slice[plane-1]>=(int)dim[plane-1]) slice[plane-1] = (int)dim[plane-1]-1;
            disp.set_wheel();
            redraw = true;
            redraw3d = true;
        }

        if (disp.button()&2  && plane)  // handle right click interaction
        {
            for(unsigned int i=0;i<3;i++) slice[i]=coord[i];
            redraw = true;
            redraw3d = true;
        }

        if(disp.key() == cimg::keyT)  {display_trace=!display_trace; disp.set_key(); redraw = true; } // disable/enable trace visualization
        if(disp.key() == cimg::keyPADADD || disp3d.key() == cimg::keyPADADD)        { if(transformType==affine) { transformType=rigid; std::cout<<"rigid"<<std::endl; } else if(transformType==rigid) { transformType=similitude; std::cout<<"similitude"<<std::endl; } else if(transformType==similitude) { transformType=affine; std::cout<<"affine"<<std::endl; } disp3d.set_key();  disp.set_key(); redraw = true;  }
        if(disp.key() == cimg::keyM || disp3d.key() == cimg::keyM )        { if(metric==SSD) { metric=NCC; std::cout<<"NCC"<<std::endl; } else { metric=SSD; std::cout<<"SSD"<<std::endl; } disp3d.set_key();  disp.set_key(); redraw = true; }
        if(disp.key() == cimg::keyS || disp3d.key() == cimg::keyS )        { if(current==&source) current=&target; else current=&source;   disp3d.set_key();  disp.set_key(); redraw = true; redraw3d = true;}

        if(disp.key() == cimg::keySPACE || disp3d.key() == cimg::keySPACE) // one registration step
        {
            float A[3][3]={{1,0,0},{0,1,0},{0,0,1}};  float t[3]={0,0,0};

            // get current profiles in target image
            mesh.updateNormals();
            CImg<unsigned char> targetProf = computeProfiles(mesh, target, Ni+S, No+S, l);

            // compute similarity image
            CImg<float> dist = computeDistance(sourceProf,targetProf ,metric);

            // compute corresponences
            computeCorrespondences(mesh,dist,l);

            // update global transformation
            Registration(A,t,mesh,transformType);

            // update points
            for(unsigned int i=0;i<mesh.getNbPoints();i++)
            {
                float p0[3]; mesh.getPoint0(p0,i);
                float p[3]; Mult(p,A,p0); for(unsigned int j=0;j<3;j++) p[j]+=t[j];
                mesh.setPoint(p,i);
            }

            disp.set_key(); redraw = true;
            disp3d.set_key();  redraw3d = true;
        }

        if(redraw)
        {
            CImg<> mpr_img=(*current)().get_projections2d(slice[0],slice[1],slice[2]);  // create image from planar projections
            mpr_img.resize(mpr_img.width(),mpr_img.height(),1,3); // convert to color image
            unsigned int dim[]={(*current)().width(),(*current)().height(),(*current)().depth()};

            if (display_trace) // draw mesh trace
            {
                float slicePos[3]; current->fromImage(slicePos,slice);

                for(unsigned int dir=0;dir<3;dir++)
                {
                    CImg<float> intersectionPoints;
                    CImgList<unsigned int> intersectionEdges;
                    mesh.getTrace(dir,slicePos[dir],intersectionPoints,intersectionEdges);

                    for(unsigned int i=0;i<intersectionEdges.size();i++)
                    {
                        float p1[3]={intersectionPoints(intersectionEdges(i)(0),0),intersectionPoints(intersectionEdges(i)(0),1),intersectionPoints(intersectionEdges(i)(0),2)};
                        int P1[3]; current->toImage(P1,p1);
                        float p2[3]={intersectionPoints(intersectionEdges(i)(1),0),intersectionPoints(intersectionEdges(i)(1),1),intersectionPoints(intersectionEdges(i)(1),2)};
                        int P2[3]; current->toImage(P2,p2);
                        if(dir==2) { if(P1[0]<(int)dim[0] && P2[0]<(int)dim[0] && P1[1]<(int)dim[1] && P2[1]<(int)dim[1]) mpr_img.draw_line(P1[0],P1[1],P2[0],P2[1],lightblue,1.0f); }
                        else if(dir==0) { if(P1[2]>=0 && P2[2]>=0 && P1[1]<(int)dim[1] && P2[1]<(int)dim[1]) mpr_img.draw_line((int)dim[0]+P1[2],P1[1],(int)dim[0]+P2[2],P2[1],lightblue,1.0f); }
                        else if(dir==1) { if(P1[0]<(int)dim[0] && P2[0]<(int)dim[0] && P1[2]>=0 && P2[2]>=0) mpr_img.draw_line(P1[0],(int)dim[1]+P1[2],P2[0],(int)dim[1]+P1[2],lightblue,1.0f); }
                    }
                }
            }

            mpr_img.draw_line(0,coord[1],dim[0]+dim[2]-1,coord[1],red,0.4f,0x55555555).draw_line(coord[0],0,coord[0],dim[1]+dim[2]-1,green,0.4f,0x55555555).draw_line(coord[2]+dim[0],0,coord[2]+dim[0],dim[1]-1,blue,0.4f,0x55555555).draw_line(0,coord[2]+dim[1],dim[0]-1,coord[2]+dim[1],blue,0.4f,0x55555555);  // draw lines around cursor
            mpr_img.draw_line(dim[0],0,dim[0],dim[1]+dim[2]-1,grey).draw_line(0,dim[1],dim[0]+dim[2]-1,dim[1],grey).draw_rectangle(0,0,dim[0]+dim[2]-1,dim[1]+dim[2]-1,grey,1,-1); // draw lines around images
            mpr_img.resize(disp.width(),disp.height()); // resize for displaying high resolution text
            float p[3]; int c[]={coord[0],coord[1],coord[2]}; current->fromImage(p,c);
            char text[100];  sprintf(text,"[%d %d %d] (%4.2f %4.2f %4.2f) : %d",coord[0],coord[1],coord[2],p[0],p[1],p[2],(*current)()(coord[0],coord[1],coord[2]));	 mpr_img.draw_text(2,2,text,white,black,0.7f,20); // write coordinates
            disp.display(mpr_img);
            redraw=false;
        }


        // 3d display
        if (init_pose)
        {
            float xm = 0, xM = mesh.vertices.get_shared_row(0).max_min(xm), ym = 0, yM = mesh.vertices.get_shared_row(1).max_min(ym), zm = 0, zM = mesh.vertices.get_shared_row(2).max_min(zm);
            const float delta = cimg::max(xM-xm,yM-ym,zM-zm);
            const float ratio = delta>0?(2.0f*cimg::min(disp3d.width(),disp3d.height())/(3.0f*delta)):1, dx = (xM + xm)/2, dy = (yM + ym)/2, dz = (zM + zm)/2;
            CImg<float>(4,3,1,1, ratio,0.,0.,-ratio*dx, 0.,ratio,0.,-ratio*dy, 0.,0.,ratio,-ratio*dz).move_to(pose);
            Xoff = Yoff = Zoff = 0; sprite_scale = 1;
            init_pose = false;
            redraw3d = true;
        }

        // Rotate and draw 3d object
        if (redraw3d)
        {
            visu = visu0;
            zbuffer.fill(0);
            mesh.drawMesh(visu,zbuffer,lightblue,1,focale,pose,renderMode,Xoff,Yoff,Zoff,sprite_scale);
            current->drawImage3d(visu, zbuffer,slice,1,focale,pose,renderMode,Xoff,Yoff,Zoff,sprite_scale);
            if(showCorrespondences) mesh.drawCorrespondences(visu,zbuffer,blue,1,focale,pose,renderMode,Xoff,Yoff,Zoff,sprite_scale);
            if(showNormals) {mesh.updateNormals(); mesh.drawNormals(scaleNormal,visu,zbuffer,red,1,focale,pose,renderMode,Xoff,Yoff,Zoff,sprite_scale);}

            visu.display(disp3d);
            redraw3d = false;
        }

        // Handle user interaction
        disp3d.wait(1);
        if ((disp3d.button() || disp3d.wheel()) && disp3d.mouse_x()>=0 && disp3d.mouse_y()>=0)
        {
            redraw3d = true;
            if (!clicked) { x0 = x1 = disp3d.mouse_x(); y0 = y1 = disp3d.mouse_y(); if (!disp3d.wheel()) clicked = true; }
            else { x1 = disp3d.mouse_x(); y1 = disp3d.mouse_y(); }
            if (disp3d.button()&1)
            {
                const float R = 0.45f*cimg::min(disp3d.width(),disp3d.height()),R2 = R*R,u0 = (float)(x0-disp3d.width()/2),v0 = (float)(y0-disp3d.height()/2),u1 = (float)(x1-disp3d.width()/2),v1 = (float)(y1-disp3d.height()/2),n0 = (float)std::sqrt(u0*u0+v0*v0),n1 = (float)std::sqrt(u1*u1+v1*v1),nu0 = n0>R?(u0*R/n0):u0,nv0 = n0>R?(v0*R/n0):v0,nw0 = (float)std::sqrt(cimg::max(0,R2-nu0*nu0-nv0*nv0)),nu1 = n1>R?(u1*R/n1):u1,nv1 = n1>R?(v1*R/n1):v1,nw1 = (float)std::sqrt(cimg::max(0,R2-nu1*nu1-nv1*nv1)),u = nv0*nw1-nw0*nv1,v = nw0*nu1-nu0*nw1,w = nv0*nu1-nu0*nv1,n = (float)std::sqrt(u*u+v*v+w*w),alpha = (float)std::asin(n/R2);
                (CImg<float>::rotation_matrix(u,v,w,alpha)*pose).move_to(pose);
                x0 = x1; y0 = y1;
            }
            if (disp3d.button()&2) { if (focale>0) Zoff-=(y0-y1)*focale/400; else { const float s = std::exp((y0-y1)/400.0f); pose*=s; sprite_scale*=s; } x0 = x1; y0 = y1; }
            if (disp3d.wheel()) { if (focale>0) Zoff-=disp3d.wheel()*focale/20; else { const float s = std::exp(disp3d.wheel()/20.0f); pose*=s; sprite_scale*=s; } disp3d.set_wheel(); }
            if (disp3d.button()&4) { Xoff+=(x1-x0); Yoff+=(y1-y0); x0 = x1; y0 = y1; }
        }
        else if (clicked) { x0 = x1; y0 = y1; clicked = false; redraw3d = true; }

        switch ( disp3d.key())
        {
        case cimg::keyF1 :  renderMode = 0; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyF2 :  renderMode = 1; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyF3 :  renderMode = 2; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyF4 :  renderMode = 3; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyF5 :  renderMode = 4; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyF6 :  renderMode = 5; disp3d.set_key(); redraw3d = true; break;
        case cimg::keyR :  init_pose = true; x0 = x1; y0 = y1; pose = CImg<float>(4,3,1,1, 1,0,0,0, 0,1,0,0, 0,0,1,0); disp3d.set_key();  redraw3d = true; break;
        case cimg::keyC :  showCorrespondences=!showCorrespondences; disp.set_key(); redraw3d = true; break;
        case cimg::keyN :  showNormals=!showNormals; disp.set_key(); redraw3d = true; break;
        }
        if (disp3d.is_resized()) { disp3d.resize(false); visu0 = visu0.get_resize(disp3d,1); zbuffer.assign(disp3d.width(),disp3d.height()); redraw3d = true; }
    }


    return 0;
}




