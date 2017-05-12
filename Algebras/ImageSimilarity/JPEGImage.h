/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

//paragraph [1] title: [{\Large \bf ]   [}]

January 2017 Michael Loris


[1] Declarations for the JPEGImage class

*/
 

#ifndef IMAGE_H_
#define IMAGE_H_

#include<vector>
#include<string>

//using namespace std;

/********************************************************************
1.1 Struct Lab

This struct models a lab-color value, which will be computed in the constructor from a rgb-color value.

********************************************************************/
struct Lab
{
    double L, a, b;
    Lab (unsigned char r_, unsigned char g_, unsigned char b_);

}; // struct Lab

/********************************************************************
1.1 Struct HSV

This struct models a hsv-color value, which will be computed in the constructor from a rgb-color value.

********************************************************************/
struct HSV
{
    int h, s, v;
    HSV (unsigned char r, unsigned char g, unsigned char b);

}; // struct HSV



struct feature //todo: Uppercase for type feature
{
    double x;
    double y;
    double r;
    double g;
    double b;
    double coarseness;
    double contrast;
};



struct ImageSignatureTuple
{
  ImageSignatureTuple() {}

  ImageSignatureTuple(double _weight, int _centroidXpos, int _centroidYpos):
    weight(_weight), centroidXpos(_centroidXpos), centroidYpos(_centroidYpos)
    {}

  ImageSignatureTuple(const ImageSignatureTuple& ist) : 
    weight(ist.weight), centroidXpos(ist.centroidXpos), 
    centroidYpos(ist.centroidYpos) {}

  ImageSignatureTuple& operator=(const ImageSignatureTuple& ist){
    weight = ist.weight;
    centroidXpos = ist.centroidXpos;
    centroidYpos = ist.centroidYpos;
    return *this;
  }

  ~ImageSignatureTuple(){}

  double weight;
  int centroidXpos;
  int centroidYpos;
};




// this class has more than one purpose, 
// but also JPEG handling, Tamura Features & clustering 
// All has been put here for the sake of simplicity

class JPEGImage
{
public:    
    void importJPEGFile(const std::string _fileName, 
                        const int colorSpace, 
                        const int picRange,
                        const int percentSamples,
                        const int noClusters);
    
    void computeCoarsenessValues(bool parallel, const int range);
    void computeContrastValues(bool parallel, const int range);
    void getRandomRepresentants(const unsigned int r);
    void clusterFeatures(const unsigned int k, 
            unsigned int dimensions, 
            unsigned int noDataPoints);
    //void clusterFeatures2(int _k, int dimensions, int noDataPoints);
    
    void writeColorImage(const char* fileName);
    void writeGrayscaleImage(const char* fileName);
    void writeCoarsenessImage(const char* fileName, double normalization);
    void writeContrastImage(const char* fileName, double normalization);
    void writeClusterImage(const char* fileName, double normalization);
    int width;
    int height;

    int* centersX;  // output of k-kmeans
    int* centersY;  // output of k-means
    double* weights; // of clusters
    void createSignature();
    std::vector<ImageSignatureTuple> signature;
    
private:
    bool isGrayscale;
    unsigned char* pixels; 
    unsigned char*** pixMat4;  // write clustered circle image
    double*** pixMat5; // for use with HSV values

    double ak( int x, int y, unsigned int k);
    double ekh(int x, int y, unsigned int k);
    double ekv(int x, int y, unsigned int k);
    double localCoarseness(int x, int y, const int range);
    double my(int x, int y, const int range);
    double sigma(int x, int y, const int range);
    double eta(int x, int y, const int range);
    double localContrast(int x, int y, const int range);

    unsigned int* randomRepresentantsX;
    unsigned int* randomRepresentantsY;
    unsigned short** assignments; // to which cluster is each centroid assigned
    std::vector<std::vector<feature>>* clusters; // output of k-means
    void drawCircle(int x, int y, int r);
    int* samplesX;
    int* samplesY;
    int noSamples;
    double* coarsenesses;
    double* contrasts;
    int colorSpace;
    int patchSize; // size of sub images to be extracted
};






#endif /* IMAGE_H_ */
