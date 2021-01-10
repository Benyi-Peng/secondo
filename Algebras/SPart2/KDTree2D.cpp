/*
----
This file is part of SECONDO.

Copyright (C) 2020,
University in Hagen,
Faculty of Mathematics and Computer Science,
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

*/


#include "KDTree2D.h"
#include "Algebras/Relation-C++/RelationAlgebra.h"
#include "Algebras/Collection/IntSet.h"
#include <iterator>
#include <map>

/*
  Class Cell2DTree

*/
Cell2DTree::Cell2DTree() {
  cellId = -1;
  x1 = -1;
  x2 = -1;
  y1 = -1;
  y2 = -1;
}

void
Cell2DTree::setCellId(int cell_id) {
  this->cellId = cell_id;
}

int
Cell2DTree::getCellId() {
  return cellId;
}

void
Cell2DTree::setValFromX(double x_1) {
  x1 = x_1;
}

double
Cell2DTree::getValFromX() {
  return x1;
}

void
Cell2DTree::setValToX(double x_2) {
  x2 = x_2;
}

double
Cell2DTree::getValToX() {
  return x2;
}

void
Cell2DTree::setValFromY(double y_1) {
  y1 = y_1;
}

double
Cell2DTree::getValFromY() {
  return y1;
}

void
Cell2DTree::setValToY(double y_2) {
  y2 = y_2;
}

double
Cell2DTree::getValToY() {
  return y2;
}

Cell2DTree::~Cell2DTree() { }

/* 
  Class KDNodeList

*/
KDNodeList::KDNodeList() {
  x = -1;
  y = -1;
  axis = -1;
  depth = -1;
  left = nullptr;
  right = nullptr;
  cellId = -1;
  leaf = false;
}

void
KDNodeList::setCellId(int cell_id) {
  this->cellId = cell_id;
}

int
KDNodeList::getCellId() {
  return cellId;
}

void KDNodeList::setAxis(int axis_) {
  this->axis = axis_;
}

int KDNodeList::getAxis() {
  return axis;
}

void KDNodeList::setDepth(int depth_) {
  this->depth = depth_;
}

int KDNodeList::getDepth() {
  return depth;
}

void KDNodeList::setValx(double x_) {
  this->x = x_;
}

double KDNodeList::getValx() {
  return x;
}

void KDNodeList::setValy(double y_) {
  this->y = y_;
}

double KDNodeList::getValy() {
  return y;
}

void KDNodeList::setLeft(KDNodeList* left_) {
  this->left = left_;
}

KDNodeList* KDNodeList::getLeft() {
  return left;
}

void KDNodeList::setRight(KDNodeList* right_) {
  this->right = right_;
} 

KDNodeList* KDNodeList::getRight() {
  return right;
}

void KDNodeList::setIsLeaf(bool leaf_) {
  this->leaf = leaf_;
}

bool KDNodeList::isLeaf() {
  return leaf;
}

KDNodeList::~KDNodeList() {}

/*
  Class KDMedList

*/
KDMedList::KDMedList() {
  part = -1;
  axis = -1;
  depth = -1;
  left = nullptr;
  right = nullptr;
  cellId = -1;
  isleaf = false;
}

void KDMedList::setVal(double val_) {
  this->part = val_;
}

double KDMedList::getVal() {
  return part;
}

void KDMedList::setAxis(int axis_) {
  this->axis = axis_;
}

int KDMedList::getAxis() {
  return axis;
}

void KDMedList::setDepth(int depth_) {
  this->depth = depth_;
}

int KDMedList::getDepth() {
  return depth;
}

void KDMedList::setLeft(KDMedList* left_) {
  this->left = left_;
}

KDMedList* KDMedList::getLeft() {
  return left;
}

void KDMedList::setRight(KDMedList* right_) {
  this->right = right_;
}

KDMedList* KDMedList::getRight() {
  return right;
}

void KDMedList::setCellId(int cellid_) {
  this->cellId = cellid_;
}

int KDMedList::getCellId() {
  return cellId;
}

bool KDMedList::isLeaf() {
  return isleaf;
}

KDMedList::~KDMedList() {}

/*
  Class KDTree2D

*/
KDTree2D::KDTree2D() {
    boundingBox = nullptr;
}

KDTree2D::KDTree2D(const KDTree2D& g) {
  boundingBox = g.boundingBox;
}

KDTree2D::KDTree2D(Rectangle<2> &bounding_box) {
  boundingBox = &bounding_box;
}

void KDTree2D::Set(Stream<Rectangle<2>> rStream,
    Rectangle<2> &bounding_box, int mode_) {

  boundingBox = &bounding_box;
  mode = mode_;

  create2DTree(rStream);
}

void KDTree2D::SetVector(std::vector<Rectangle<2>>* rVector,
                     Rectangle<2> &bounding_box) {
   boundingBox = &bounding_box;

   create2DTreeVector(rVector);
}

void KDTree2D::create2DTree(Stream<Rectangle<2>> rStream) {
    processInput(rStream);

  // create 2dtree
  build2DTree();

}

void KDTree2D::create2DTreeVector(std::vector<Rectangle<2>>* rVector) {
     processInputVector(rVector);

   // create kdtree
   build2DTree();
}

Rectangle<2> *
KDTree2D::getBoundingBox() {
  return boundingBox;
}

int KDTree2D::getMode() {
  return mode;
}

void
KDTree2D::setPointsVector(std::vector<KDNodeList*> points_vect) {
  this->pointsVector = points_vect;
}


std::vector<KDNodeList*>&
KDTree2D::getPointsVector() {
  return this->pointsVector;
}

void
KDTree2D::setPointsMedVector(std::vector<KDMedList*> points_vect) {
  this->kdmedListVec = points_vect;
}


std::vector<KDMedList*>&
KDTree2D::getPointsMedVector() {
  return this->kdmedListVec;
}

void
KDTree2D::setCellVector(std::vector<Cell2DTree> cell_vect) {
  this->cellVector = cell_vect;
}


std::vector<Cell2DTree>&
KDTree2D::getCellVector() {
  return this->cellVector;
}

TPoint
getCuboidCentre(Rectangle<2>* r) {
  double a = (r->getMaxY() - r->getMinY()) / (double)2;
  double b = (r->getMaxX() - r->getMinX()) / (double)2;

  TPoint r_c { (r->getMinX())+b, (r->getMinY())+a};

  return r_c;
}

/* 
  Check if a rectangle <2> is inside the irgrid2d bounding box

*/
bool
insideRectangleKD(Rectangle<2>* bbox, Rectangle<2>* r) {
  double le = bbox->getMinX();
  double ri = bbox->getMaxX();
  double bo = bbox->getMinY();
  double to = bbox->getMaxY();

  if (r->getMinX() >= le && r->getMaxX() <= ri
      && r->getMinY() >= bo && r->getMaxY() <=to) {                        

    return true;
  }

 return false;
}

bool
pointComparisonX(TPoint p1, TPoint p2) {
  return p1.x < p2.x;
}

bool
xValueComparison(Cell2DTree x1, Cell2DTree x2) {
  return x1.getValFromX() < x2.getValFromX();
}

bool
yValueComparison(Cell2DTree y1, Cell2DTree y2) {
  return y1.getValFromY() < y2.getValFromY();
}

bool
pointComparisonY(TPoint p1, TPoint p2) {
  return p1.y < p2.y;
}


std::vector<TPoint> 
slice(std::vector<TPoint> const &v, int m, int n)
{
  auto first = v.cbegin() + m;
  auto last = v.cbegin() + n + 1;
  
  std::vector<TPoint> vec(first, last);
  return vec;
}

/*
  Calculates 2D-Tree recursive with median of points

*/
KDMedList*
KDTree2D::KDTreeMedRec(std::vector<TPoint> point_list,
 int depth) 
{
  int axis = depth % 2; // dim = 2
  double median;
  std::vector<TPoint> point_list_left;
  std::vector<TPoint> point_list_right;
  int mid = point_list.size()/2;

  if(axis == 0) {
  std::sort(point_list.begin(), point_list.end(), 
          pointComparisonX);

  // calculate median
  if(point_list.size() % 2 == 0) 
  {
    median = 0.5*(point_list.at(mid-1).x + point_list.at(mid).x);
  } else {
    median = point_list.at(mid).x;
  }
    for(size_t i=0; i < point_list.size(); i++)
    {
      if(point_list.at(i).x < median)
      {
        point_list_left.push_back(point_list.at(i));
      } else {
        point_list_right.push_back(point_list.at(i));
      }
    }

    // repartition if size of point_list_left or point_list_right 
    // corresponds to size of (old) point_list from input
    // in case take middle position as median
    if(point_list_left.size() == point_list.size() 
    || point_list_right.size() == point_list.size())
    {
      if (point_list_left.size() > 0) {
        point_list_left.clear();
      }
      if (point_list_right.size() > 0) {
        point_list_right.clear();
      }
      median = point_list[mid].x;
      for(int l=0; l < mid; l++)
      {
        point_list_left.push_back(point_list[l]);
      }
      for(size_t r=mid; r < point_list.size(); r++)
      {
        point_list_right.push_back(point_list[r]);
      }
    }

  } else if (axis == 1) {
    std::sort(point_list.begin(), point_list.end(), 
        pointComparisonY);
    if(point_list.size() % 2 == 0) 
    {
      median = 0.5*(point_list.at(mid-1).y + point_list.at(mid).y);
    } else {
      median = point_list.at(mid).y;
    }

    for(size_t i=0; i < point_list.size(); i++)
    {
      if(point_list.at(i).y < median)
      {
        point_list_left.push_back(point_list.at(i));
      } else {
        point_list_right.push_back(point_list.at(i));
      }
    }

    // repartition if size of point_list_left or point_list_right 
    // corresponds to size of (old) point_list from input
    // in case take middle position as median
    if(point_list_left.size() == point_list.size() 
    || point_list_right.size() == point_list.size())
    {
      if (point_list_left.size() > 0) {
        point_list_left.clear();
      }
      if (point_list_right.size() > 0) {
        point_list_right.clear();
      }
      median = point_list[mid].y;
      for(int l=0; l < mid; l++)
      {
        point_list_left.push_back(point_list[l]);
      }
      for(size_t r=mid; r < point_list.size(); r++)
      {
        point_list_right.push_back(point_list[r]);
      }
    }
    
  } else {
    return 0;
  }

    KDMedList* tmp = new KDMedList();
    tmp->setAxis(axis);
    tmp->setDepth(depth);
    tmp->setVal(median);


    if(point_list_left.size() > 1) {
      tmp->left = KDTreeMedRec(point_list_left, depth+1); 
    }
    if(point_list_right.size() > 1) {
      tmp->right = KDTreeMedRec(point_list_right, depth+1);
    }

    // push current node in vector
    kdmedListVec.push_back(tmp);
    if (point_list_left.size() > 0) {
      point_list_left.clear();
    }
    if (point_list_right.size() > 0) {
      point_list_right.clear();
    }

    return tmp;
  

}

/*
  Calculates 2D-Tree recursive with middle of given list of points

*/
KDNodeList*
KDTree2D::KDTreeRec(std::vector<TPoint> point_list, int begin,
 int end, int depth) {
  
  if(begin == end) {
    // only one element in list is a leaf
    KDNodeList* lastNode = new KDNodeList();
    if(!point_list.empty()) {    
      lastNode->setValx(point_list[0].x);
      lastNode->setValy(point_list[0].y);
      lastNode->setAxis(depth%2);
      lastNode->setDepth(depth);

      KDNodeList* leftl = new KDNodeList();
      leftl->setCellId(-1);
      leftl->setIsLeaf(true);
      KDNodeList* rightl = new KDNodeList();
      rightl->setCellId(-1);
      rightl->setIsLeaf(true);
      lastNode->setLeft(leftl);
      lastNode->setRight(rightl);
      pointsVector.push_back(lastNode);
    }
    return lastNode;
  }
  if(end < begin) {
    return nullptr;
  } 

  int axis = depth % 2; //dim = 2
  if(axis == 0) {
  std::sort(point_list.begin(), point_list.end(), 
          pointComparisonX);
  } else if (axis == 1) {
    std::sort(point_list.begin(), point_list.end(), 
        pointComparisonY);
  }

  for(int i = 0; i <= (int)point_list.size()-1; i++) {
  }

  int medianpos = point_list.size() / 2;
  TPoint middle = point_list[medianpos];
  KDNodeList* tmp = new KDNodeList();

  if(!point_list[medianpos].used) {
    tmp->setValx(middle.x);
    tmp->setValy(middle.y);
    tmp->setAxis(axis);
    tmp->setDepth(depth);

  
    std::vector<TPoint> point_list_left;
    std::vector<TPoint> point_list_right;
    point_list[medianpos].used = true;

    // divide list of points in two parts
    for(int l = 0; l <= medianpos-1; l++)
    {
      point_list_left.push_back(point_list[l]);

    }
  
    int pos = 0;

    for(int r = medianpos+1; r <= (int)point_list.size()-1; r++)
    {
      point_list_right.push_back(point_list[r]);
      pos++;

    }

  
    // in case point_list_left/right is empty, declare leaf
    KDNodeList* leftk = new KDNodeList();
    leftk->setCellId(-1);
    leftk->setIsLeaf(true);
    KDNodeList* rightk = new KDNodeList();
    rightk->setCellId(-1);
    rightk->setIsLeaf(true);
  
  
    if(!point_list_left.empty()) {
      tmp->left = KDTreeRec(point_list_left, 0, medianpos-1, depth+1); 
    } else {
      //set leaf if list is empty
      tmp->setLeft(leftk);
    }
    if(!point_list_right.empty()) {
      tmp->right = KDTreeRec(point_list_right, medianpos+1,
       (int)point_list.size(), depth+1);
    } else {
      //set leaf if list is empty
      tmp->setRight(rightk);
    }
    
    // push current node in vector
    pointsVector.push_back(tmp);
    if (point_list_left.size() > 0) {
      point_list_left.clear();
    }
    if (point_list_right.size() > 0) {
      point_list_right.clear();
    }

  }

  return tmp;

}

/*
  returns tree in preoder
  
*/
void 
KDTree2D::preorder (KDNodeList* root)
{
  if (root == nullptr) {
    return;
  } 
  pointsPreorder.push_back(root);
  if(root->left != nullptr) {
    preorder(root->left);
  }

  if(root->right != nullptr) {
    preorder(root->right);
  }

  return;
}

/*
  return tree from median creation in preoder

*/ 
void 
KDTree2D::preorderMed(KDMedList* root)
{
  if (root == nullptr) {
    return;
  } 
  pointsPreorderMed.push_back(root);
  if(root->left != nullptr) {
    preorderMed(root->left);
  }

  if(root->right != nullptr) {

    preorderMed(root->right);
  }

  return;
}

/*
  returns grid from median creation

*/
void
KDTree2D::preorderMedGrid (CellKD* boundBox, 
 KDMedList* node)
  //std::vector<KDMedList*> pointsPreOrdered)
{

  boundBox->left = new CellKD();
  CellKD* cell_left = boundBox->left;
  boundBox->right = new CellKD();
  CellKD* cell_right = boundBox->right;

  if(node->getAxis() == 0) {
  cell_left->value.x1 = boundBox->value.x1;
  cell_left->value.x2 = node->getVal();
  cell_left->value.y1 = boundBox->value.y1;
  cell_left->value.y2 = boundBox->value.y2;
  cell_left->final = true;

  cell_right->value.x1 = node->getVal();
  cell_right->value.x2 = boundBox->value.x2;
  cell_right->value.y1 = boundBox->value.y1;
  cell_right->value.y2 = boundBox->value.y2;
  cell_right->final = true;

  boundBox->final = false;

  //cellsPreorder.push_back(boundBox);
  cellsPreorder.push_back(cell_right);
  cellsPreorder.push_back(cell_left);
  } else {
    cell_left->value.y1 = boundBox->value.y1;
    cell_left->value.y2 = node->getVal();
    cell_left->value.x1 = boundBox->value.x1;
    cell_left->value.x2 = boundBox->value.x2;
    cell_left->final = true;


    cell_right->value.y1 = node->getVal();
    cell_right->value.y2 = boundBox->value.y2;
    cell_right->value.x1 = boundBox->value.x1;
    cell_right->value.x2 = boundBox->value.x2;
    cell_right->final = true;

    boundBox->final = false;

    cellsPreorder.push_back(cell_right);
    cellsPreorder.push_back(cell_left);
  }

  if(node->getLeft() != nullptr) {
  preorderMedGrid(cell_left, node->getLeft());
  }
  if(node->getRight() != nullptr) {
  preorderMedGrid(cell_right, node->getRight());
  }
  
}

/*
  returns grid 

*/
void
KDTree2D::preorderGrid (CellKD* boundBox,
KDNodeList* node)
{
  boundBox->left = new CellKD();
  CellKD* cell_left = boundBox->left;
  boundBox->right = new CellKD();
  CellKD* cell_right = boundBox->right;

  if(node->getAxis() == 0) {
    cell_left->value.x1 = boundBox->value.x1;
    cell_left->value.x2 = node->x;
    cell_left->value.y1 = boundBox->value.y1;
    cell_left->value.y2 = boundBox->value.y2;
    cell_left->final = true;

    cell_right->value.x1 = node->x;
    cell_right->value.x2 = boundBox->value.x2;
    cell_right->value.y1 = boundBox->value.y1;
    cell_right->value.y2 = boundBox->value.y2;
    cell_right->final = true;

    boundBox->final = false;

    cellsPreorder.push_back(cell_right);
    cellsPreorder.push_back(cell_left);
  } else {
    cell_left->value.y1 = boundBox->value.y1;
    cell_left->value.y2 = node->y;
    cell_left->value.x1 = boundBox->value.x1;
    cell_left->value.x2 = boundBox->value.x2;
    cell_left->final = true;


    cell_right->value.y1 = node->y;
    cell_right->value.y2 = boundBox->value.y2;
    cell_right->value.x1 = boundBox->value.x1;
    cell_right->value.x2 = boundBox->value.x2;
    cell_right->final = true;

    boundBox->final = false;

    cellsPreorder.push_back(cell_right);
    cellsPreorder.push_back(cell_left);
  }

  if(node->getLeft() != nullptr && !node->getLeft()->isLeaf()) {
  preorderGrid(cell_left, node->getLeft());
  }
  if(node->getRight() != nullptr && !node->getRight()->isLeaf()) {
  preorderGrid(cell_right, node->getRight());
  }

}

/*
  sets cell ids

*/
void
KDTree2D::setCellId (Cell2DTree cell, KDNodeList* kdNode)
{
  while(kdNode != nullptr) {
    if(kdNode->isLeaf()) {
      break; 
    }

    if(kdNode->axis == 0) {
    // check x1 value of cell
    if(cell.getValFromX() < kdNode->x) {
      kdNode = kdNode->left;
    } else {
      kdNode = kdNode->right;
    }
    } else if(kdNode->axis == 1) {
      // check y1 value of cell
      if(cell.getValFromY() < kdNode->y) {
        kdNode = kdNode->left;
      } else {
        kdNode = kdNode->right;
      }
    } else {
      return;
    }
    
  }

  kdNode->setCellId(cell.getCellId());
  return;
}

/*
  sets cell id from median creation

*/
void
KDTree2D::setCellId (Cell2DTree cell, KDMedList* kdNode)
{
  KDMedList* node = new KDMedList();

  while(kdNode != nullptr) {

    if(kdNode->axis == 0) {
    // check x1 value of cell
    if(cell.getValFromX() < kdNode->getVal()) {
      if(kdNode->left != nullptr) {
        kdNode = kdNode->left;
      } else {
        node->isleaf = true;
        kdNode->left = node;
        kdNode->left->setCellId(cell.getCellId());
        return;
      }
    } else {
      if(kdNode->right != nullptr) {
        kdNode = kdNode->right;
      } else { 
        node->isleaf = true;
        kdNode->right = node;
        kdNode->right->setCellId(cell.getCellId());
        return; }
    }
    } else if(kdNode->axis == 1) {
      // check y1 value of cell
      if(cell.getValFromY() < kdNode->getVal()) {
        if(kdNode->left != nullptr) {
          kdNode = kdNode->left;
        } else {
          node->isleaf = true;
          kdNode->left = node;
          kdNode->left->setCellId(cell.getCellId());
          return;
        }
      } else {
        if(kdNode->right != nullptr) {
          kdNode = kdNode->right;
        } else { 
          node->isleaf = true;
          kdNode->right = node;
          kdNode->right->setCellId(cell.getCellId());
          return; }
      }
    } else {
      return;
    }
    
  }

  kdNode->setCellId(cell.getCellId());
  return;
}


void
KDTree2D::build2DTree() {

  // create grid structure
  CellKD* boundBox = new CellKD();
  boundBox->value.x1 = boundingBox->getMinX();
  boundBox->value.x2 = boundingBox->getMaxX();
  boundBox->value.y1 = boundingBox->getMinY();
  boundBox->value.y2 = boundingBox->getMaxY();

  KDNodeList* root;
  KDMedList* root2;
  // build kdtree recursive
  if(mode == 1) {
    KDTreeRec(points, 0, (int)points.size()-1,0);
    // points saved in pointsVector, 
    // taking the root, saved in the back of vector
    root = pointsVector.back();
    // Points of KDtree in preorder
    preorder(root);
    // order cells in preorder
    preorderGrid(boundBox, pointsPreorder[0]);

  } else {
    
    KDTreeMedRec(points,0); 
    //taking the root, saved in the back of vector
    root2 = kdmedListVec.back();

    // Points of KDtree in preorder
    preorderMed(root2);

    // order cells in preorder
    preorderMedGrid(boundBox, pointsPreorderMed[0]);

  }
  Cell2DTree cell = Cell2DTree();

  // filter finale cells
  for(int r = 0; r < (int)cellsPreorder.size(); r++) {
    if(cellsPreorder.at(r)->final == 1) {
      cell.setValFromX(cellsPreorder.at(r)->value.x1);
      cell.setValToX(cellsPreorder.at(r)->value.x2);
      cell.setValFromY(cellsPreorder.at(r)->value.y1);
      cell.setValToY(cellsPreorder.at(r)->value.y2);
      // save cells in cellVector      
      cellVector.push_back(cell);
    }
  }
  
  // first sort by x-value then by y-value
  sort(cellVector.begin(), cellVector.end(), xValueComparison);
  sort(cellVector.begin(), cellVector.end(), yValueComparison);

  int cellId = 0;
  
  for(int e = 0; e < (int)cellVector.size(); e++)
  {   
    cellId++;
    cellVector.at(e).setCellId(cellId);
    // set cellIds in kdTree (pointsPreorder)
    if(mode == 1) {
      setCellId(cellVector.at(e), root); // root from pointsvector
    } else {
      setCellId(cellVector.at(e), root2); // root from pointsvector
    }

  }
}



bool
KDTree2D::duplicateP(TPoint p)
{
  for(size_t i = 0; i < points.size(); i++)
  {
    if(points[i].x == p.x && points[i].y == p.y)
    {
      return true;
    }
  }
  return false;
}

void
KDTree2D::processInput(Stream<Rectangle<2>> rStream) {
  rStream.open();
  Rectangle<2>* next = rStream.request();

  while(next != 0){
    if (!insideRectangleKD(boundingBox, next)) {
      // rectangle (partially) outside the bounding box is discarded
      next = rStream.request();
      continue;
    }
    TPoint p = getCuboidCentre(next);
    // avoid same points in vector
    if(!duplicateP(p)) {
      points.push_back(p);
    }
    next = rStream.request();
  }
  rStream.close();
}

void
KDTree2D::processInputVector(std::vector<Rectangle<2>>* rVector) {
   for (Rectangle<2> bbox : *rVector) {
      if (!insideRectangleKD(boundingBox, &bbox)) {
         // rectangle (partially) outside the bounding box is discarded
         continue;
      }
      points.push_back(getCuboidCentre(&bbox));
   }
}


KDTree2D::~KDTree2D() {}

ListExpr
KDTree2D::Property2DTree()
{
    ListExpr desclst = nl->TextAtom();
    nl->AppendText(desclst,
    "A <rectangle> bounding box "
    "followed by list of cells.\n"
    "A cell consists of a five-element list\n" 
    "(<from x> <to x> <from y> <to y> <id>). ");

  ListExpr formatlst = nl->TextAtom();
    nl->AppendText(formatlst,
    "((0.0 2.0 0.0 2.0) ((0.0 0.25 0.0 0.7 1) (0.25 0.9 0.0 0.7 2) "
    " (0.9 1.0 0.0 1.5 3) "
    "(1.0 2.0 0.0 1.5 4) (0.0 0.9 0.7 2.0 5) (0.9 2.0 1.5 2.0 6)))");

  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                             nl->StringAtom("Example Type List"),
                             nl->StringAtom("List Rep"),
                             nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                             nl->StringAtom(KDTree2D::BasicType()),
               desclst,
               formatlst)));
}

// Out function
ListExpr
KDTree2D::Out2DTree( ListExpr typeInfo, Word value ) {
  KDTree2D* kdtree2d = static_cast<KDTree2D*>( value.addr );
  if (kdtree2d != nullptr) {
    Rectangle<2> * b_box = kdtree2d->getBoundingBox();
    ListExpr bboxLstExpr = nl->FourElemList(
      nl->RealAtom(b_box->getMinX()),
      nl->RealAtom(b_box->getMaxX()),
      nl->RealAtom(b_box->getMinY()),
      nl->RealAtom(b_box->getMaxY()));

    //ListExpr rowLstExpr = nl->Empty();
    ListExpr cellLstExpr = nl->Empty();

    int mode = kdtree2d->getMode();
    std::vector<KDNodeList*> nodelist;
    std::vector<KDMedList*> nodelistM;
    if(mode == 1) {
      nodelist = kdtree2d->getPointsVector();
    } else if(mode == 2) {
      nodelistM = kdtree2d->getPointsMedVector();
    }
    std::vector<Cell2DTree>* cells = &kdtree2d->getCellVector();

        if (cells->size() > 0) {
          ListExpr lastCellLstExpr;
          for(size_t cellIdx = 0; cellIdx < cells->size(); cellIdx++) {
            Cell2DTree* curr_cell = &cells->at(cellIdx);

            if (cellIdx > 0) {
              lastCellLstExpr = nl->Append(lastCellLstExpr,
                  nl->FiveElemList(nl->RealAtom(curr_cell->getValFromX()),
                  nl->RealAtom(curr_cell->getValToX()),
                  nl->RealAtom(curr_cell->getValFromY()),
                  nl->RealAtom(curr_cell->getValToY()),
                  nl->IntAtom(curr_cell->getCellId())));
            } else {
              cellLstExpr = nl->OneElemList(nl->FiveElemList
                  (nl->RealAtom(curr_cell->getValFromX()),
                  nl->RealAtom(curr_cell->getValToX()),
                  nl->RealAtom(curr_cell->getValFromY()),
                  nl->RealAtom(curr_cell->getValToY()),
                  nl->IntAtom(curr_cell->getCellId())));
              lastCellLstExpr = cellLstExpr;
            }
          }
        }

        /*if(nodelist.size() > 0) {
          ListExpr lastNodeLstExpr;
          KDNodeList* curr_node = nodelist.back(); // root
          while(!curr_node->isLeaf())
          {
            lastNodeLstExpr = nl->Append(lastNodeLstExpr,
              nl->OneElemList
            )
          }
          
        }*/
    
    ListExpr kdtree2dLstExpr = nl->TwoElemList(bboxLstExpr, cellLstExpr);
    return kdtree2dLstExpr;
  } else {
    return (nl->SymbolAtom(Symbol::UNDEFINED()));
  }
}

// In function
Word
KDTree2D::In2DTree( const ListExpr typeInfo, const ListExpr instance,
      const int errorPos, ListExpr& errorInfo, bool& correct ) {

  Word w = SetWord(Address(0));
  try {
    Rectangle<2>* bbox;
    ListExpr bboxLstExpr;
    ListExpr cellLstExpr;

    if ( nl->ListLength( instance ) == 2 ) {
      bboxLstExpr = nl->First(instance);
      cellLstExpr = nl->Second(instance);
    } else {
      throw 1;
    }

    // fetch bounding box information from input
    if (nl->ListLength( bboxLstExpr ) == 4 ) {
      ListExpr left = nl->First(bboxLstExpr);
      ListExpr right = nl->Second(bboxLstExpr);
      ListExpr bottom = nl->Third(bboxLstExpr);
      ListExpr top = nl->Fourth(bboxLstExpr);

      if ( nl->IsAtom(left) && nl->AtomType(left) == RealType
          && nl->IsAtom(right) && nl->AtomType(right) == RealType
          && nl->IsAtom(bottom) && nl->AtomType(bottom) == RealType
          && nl->IsAtom(top) && nl->AtomType(top) == RealType) {

        double min[2], max[2];
        min[0] = nl->RealValue(left);
        min[1] = nl->RealValue(bottom);
        max[0] = nl->RealValue(right);
        max[1] = nl->RealValue(top);

        bbox = new Rectangle<2>(true, min, max);
      } else {
        throw 3;
      }

    } else {
      throw 2;
    }
    // temporary support structures
    std::map<int, int> cellRef;
    std::map<int, Cell2DTree*> cellIds;

    std::vector<Cell2DTree> cell_vec {};
    int cell_cnt = 0;

    if(nl->ListLength(cellLstExpr) > 1 ) {
      while(!nl->IsEmpty(cellLstExpr)) {
          ListExpr lstElem = nl->First(cellLstExpr);

            if (nl->ListLength( lstElem ) == 5) {
              // a five-element list initiates a new cell
              ListExpr cv1Lst = nl->First(lstElem);
              ListExpr cv2Lst = nl->Second(lstElem);
              ListExpr cv3Lst = nl->Third(lstElem);
              ListExpr cv4Lst = nl->Fourth(lstElem);
              ListExpr cv5Lst = nl->Fifth(lstElem);

              cell_cnt++;

              if ( nl->IsAtom(cv1Lst) && nl->AtomType(cv1Lst) == RealType
                  && nl->IsAtom(cv2Lst) && nl->AtomType(cv2Lst) == RealType
                  && nl->IsAtom(cv3Lst) && nl->AtomType(cv3Lst) == RealType
                  && nl->IsAtom(cv4Lst) && nl->AtomType(cv4Lst) == RealType
                  && nl->IsAtom(cv5Lst) && nl->AtomType(cv5Lst) == IntType) {
                Cell2DTree c;
                c.setValFromX(nl->RealValue(cv1Lst));
                c.setValToX(nl->RealValue(cv2Lst));
                c.setValFromY(nl->RealValue(cv3Lst));
                c.setValToY(nl->RealValue(cv4Lst));
                c.setCellId(nl->IntValue(cv5Lst));

                cell_vec.push_back(c);
                cellIds.insert(std::make_pair(
                  ((int)nl->IntValue(cv5Lst)), &c));
              } else {
                throw 4;
              }
            }
                    cellLstExpr = nl->Rest(cellLstExpr);

          }
    }
    
    std::vector<KDNodeList*> points_vec {};

    correct = true;
    KDTree2D* kdtree = new KDTree2D(*bbox);
    kdtree->setPointsVector(points_vec);
    kdtree->setCellVector(cell_vec);

    w.addr = kdtree;
    return w;
  } catch (int e) {
    correct = false;
    cmsg.inFunError("Expecting a kdtree list representation. Exit code "
        + std::to_string(e));

    return w;
  }
}

// This function checks whether the type constructor is applied correctly.
bool
KDTree2D::KindCheck2DTree( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, KDTree2D::BasicType() ));
}

// Close -function
void
KDTree2D::Close2DTree( const ListExpr typeInfo, Word& w )
{
  delete (KDTree2D *)w.addr;
  w.addr = 0;
}

// Clone function
Word
KDTree2D::Clone2DTree( const ListExpr typeInfo, const Word& w )
{
  KDTree2D *g = new KDTree2D( *((KDTree2D *)w.addr) );
  return SetWord( g );
}

// Create function
Word
KDTree2D::Create2DTree( const ListExpr typeInfo )
{
  return SetWord( new KDTree2D() );
}

// Delete function
void
KDTree2D::Delete2DTree( const ListExpr typeInfo, Word& w )
{
  delete (KDTree2D *)w.addr;
  w.addr = 0;
}

// SizeOf function
int
KDTree2D::SizeOf2DTree()
{
  return sizeof(KDTree2D);
}

ListExpr
KDTree2D::KdTree2dFeedTypeMap( ListExpr args )
{
  if(nl->HasLength(args, 1)) {

    ListExpr first = nl->First(args);
    if (KDTree2D::checkType(first)) {
      ListExpr resAttrList = nl->TwoElemList(
          nl->TwoElemList(
            nl->SymbolAtom("Id"),
            nl->SymbolAtom(CcInt::BasicType())),
          nl->TwoElemList(
            nl->SymbolAtom("Cell2DTree"),
            nl->SymbolAtom(Rectangle<2>::BasicType())));

      return nl->TwoElemList(listutils::basicSymbol<Stream<Tuple>>(),
        nl->TwoElemList(
          listutils::basicSymbol<Tuple>(),
          resAttrList));

      }
  }
  const std::string errMsg = "The following argument is expected:"
      " 2dtree";

  return  listutils::typeError(errMsg);
}

std::vector<CellInfo2DTree*>
KDTree2D::getCellInfoVector(KDTree2D *in_kdtree2d) {
  std::vector<CellInfo2DTree*> cell_info_vect {};

  std::vector<Cell2DTree>* cells = &in_kdtree2d->getCellVector();
  for(size_t cellIdx = 0; cellIdx < cells->size(); cellIdx++) {
    Cell2DTree* cell = &cells->at(cellIdx);
    double cfx = cell->getValFromX();
    double ctx = cell->getValToX();
    double cfy = cell->getValFromY();
    double cty = cell->getValToY();
    int cid = cell->getCellId();

    CellInfo2DTree* ci = new CellInfo2DTree(cid, cfx, ctx, cfy, cty);
    cell_info_vect.push_back(ci);
  
  }

  return cell_info_vect;
}


// for value mapping function of ~feed~ operator
struct KDTRee2DTupleInfo
{
  std::vector<CellInfo2DTree*> cell_info_vect;
  unsigned int currentTupleIdx;
  ListExpr numTupleTypeList;

  void init(KDTree2D *kdtree2d_in) {
    currentTupleIdx = 0;
    cell_info_vect = KDTree2D::getCellInfoVector(kdtree2d_in);

    ListExpr tupleTypeLst = nl->TwoElemList(
      nl->SymbolAtom(Tuple::BasicType()),
      nl->TwoElemList(
        nl->TwoElemList(
          nl->SymbolAtom("Id"),
          nl->SymbolAtom(CcInt::BasicType())),
        nl->TwoElemList(
          nl->SymbolAtom("Cell2DTree"),
          nl->SymbolAtom(Rectangle<2>::BasicType()))));

    SecondoCatalog* sc = SecondoSystem::GetCatalog();
    numTupleTypeList = sc->NumericType(tupleTypeLst);
  }

  TupleType* getTupleType() {
    TupleType *tupleType = new TupleType(numTupleTypeList);

    return tupleType;
  }

  Tuple* getNext(TupleType *ttype) {
    if (currentTupleIdx < cell_info_vect.size()) {

      CellInfo2DTree * cell_info = cell_info_vect.at(currentTupleIdx);
      int tp_p1 = cell_info->cellId;
      Rectangle<2> tp_p2 = *cell_info->cell;

      Tuple *tuple = new Tuple(ttype);
      tuple->PutAttribute(0, new CcInt(true, tp_p1));
      tuple->PutAttribute(1, new Rectangle<2> (tp_p2));

      currentTupleIdx++;
      return tuple;
    } else {
      return nullptr;
    }
  }
};

/*
Value mapping function of operator ~feed~

*/
int
KDTree2D::KdTree2dValueMapFeed( Word* args, Word& result, int message,
                        Word& local, Supplier s ) {
  KDTree2D *input_kdtree2d_ptr =
    static_cast<KDTree2D*>( args[0].addr );
  KDTRee2DTupleInfo* tp_info = static_cast<KDTRee2DTupleInfo*>(local.addr);
  TupleType* tupleType = nullptr;
  Tuple* tuple = nullptr;

  switch (message) {
    case OPEN: {
      tp_info = new KDTRee2DTupleInfo();
      tp_info->init(input_kdtree2d_ptr);
      local.addr = tp_info;
      return 0;
    }
    case REQUEST: {
      if (local.addr) {
        tp_info = ((KDTRee2DTupleInfo*)local.addr);
        tupleType = tp_info->getTupleType();
      } else {
        return CANCEL;
      }
      // get next tuple
      tuple = tp_info->getNext(tupleType);

      if (tuple != nullptr) {
          result.addr = tuple;
          return YIELD;
      } else {
        result.addr = 0;
        return CANCEL;
      }
    }
    case CLOSE: {
      if (local.addr) {

        tp_info = ((KDTRee2DTupleInfo*)local.addr);
        delete tp_info;
        local.addr = 0;
      }
      return 0;
    }
  }

  return -1;
}

/*
Type mapping function ~KDTree2dCellnosTypeMap~

It is used for the ~cellnos\_kd~ operator.

*/
ListExpr
KDTree2D::Kdtree2dCellnosTypeMap( ListExpr args )
{
  if(nl->HasLength(args, 2)) {
    ListExpr first = nl->First(args);
    ListExpr second = nl->Second(args);

    if (KDTree2D::checkType(first) && Rectangle<2>::checkType(second)) {
      return nl->SymbolAtom(collection::IntSet::BasicType());
    }
  }

  const std::string errMsg = "The following two arguments are expected:"
      " 2dtree x rect";

  return  listutils::typeError(errMsg);
}

ListExpr
KDTree2D::Kdtree2dSCCTypeMap( ListExpr args )
{
  if(nl->HasLength(args, 4)) {
    ListExpr first = nl->First(args);
    ListExpr second = nl->Second(args);
    ListExpr third = nl->Third(args);
    ListExpr fourth =  nl->Fourth(args);

    if (KDTree2D::checkType(first) && Rectangle<2>::checkType(second)
      && Rectangle<2>::checkType(third) && CcInt::checkType(fourth)) {
      return nl->SymbolAtom(CcBool::BasicType());
    }
  }

  const std::string errMsg = "The following four arguments are expected:"
      " 2dtree x rect x rect x int";

  return  listutils::typeError(errMsg);
}

ListExpr
KDTree2D::Kdtree2dGetCellTypeMap( ListExpr args )
{
  if(nl->HasLength(args, 2)) {
    ListExpr first = nl->First(args);
    ListExpr second = nl->Second(args);

    if (KDTree2D::checkType(first) && CcInt::checkType(second)) {
      return nl->SymbolAtom(Rectangle<2>::BasicType());
    }
  }

  const std::string errMsg = "The following two arguments are expected:"
      " 2dtree x int";

  return  listutils::typeError(errMsg);
}


bool
InCell(Cell2DTree cell, double valy, double valx) {
  return (valx >= cell.getValFromX()
    && valx < cell.getValToX()
    && valy >= cell.getValFromY()
    && valy < cell.getValToY());
}

//template <class C>
int
CellBS(std::vector<Cell2DTree>* c_vec, int start, 
  int end, const double valy, const double valx) 
{
  if (start > end) {
    return -1;
  }

  const int mid = start;

  if (InCell(c_vec->at(mid), valy, valx)) {
    return mid;
  }
  
  return CellBS(c_vec, mid+1, end, valy, valx);

}

void
GetLeaf(KDMedList* node, double le, double ri, 
  double bo, double to, std::set<int> *cell_ids) 
{
  if(node->isLeaf()) {
    cell_ids->insert(node->getCellId());
    return;
    }

  if(node->getAxis() == 0) {
    // check x1 value
    if(le < node->getVal()) {
      if(node->getLeft() != nullptr) {
        GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
      }
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    // check x2 value
    if(ri < node->getVal()) {
      if(node->getLeft() != nullptr) {
        GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
      }
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    
  }
  if (node->getAxis() == 1) {
    // check y1 value
    if(bo < node->getVal()) {
      if(node->getLeft() != nullptr) {
        GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
      }
    } else {
      if(node->getRight() != nullptr) {
        GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    // check y2 value
    if(to < node->getVal()) {
      if(node->getLeft() != nullptr) {
        GetLeaf(node->getLeft(), le, ri,  bo, to, cell_ids);
      }
    } else {
      if(node->getRight() != nullptr) {
        GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
  } 
  return;
}


void
GetLeaf(KDNodeList* node, double le, double ri, 
  double bo, double to, std::set<int> *cell_ids) 
{
  // reached the end of the branch => get cellId
  if(node->isLeaf()) {
    cell_ids->insert(node->getCellId());
    return;
    }

  if(node->getAxis() == 0) {
    // check x1 value
    if(le < node->getValx() && node->getLeft() != nullptr) {
      GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    // check x2 value
    if(ri < node->getValx() && node->getLeft() != nullptr) {
      GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    
  }
  if (node->getAxis() == 1) {
    // check y1 value
    if(bo < node->getValy() && node->getLeft() != nullptr) {
      GetLeaf(node->getLeft(), le, ri, bo, to, cell_ids);
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
    // check y2 value
    if(to < node->getValy() && node->getLeft() != nullptr) {
      GetLeaf(node->getLeft(), le, ri,  bo, to, cell_ids);
    } else {
      if(node->getRight() != nullptr) {
      GetLeaf(node->getRight(), le, ri, bo, to, cell_ids);
      }
    }
  } 
  return;
}

/*
  Returns true if two rectangles (l1, r1) and (l2, r2) overlap 

*/
bool rectOverlap(double toleX1, double toleY1, double boriX1,
 double boriY1, double toleX2, double toleY2, double boriX2,
 double boriY2) 
{ 
    // If one rectangle is on left side of other 
    if (toleX1 > boriX2 || toleX2 > boriX1) 
        return false; 
  
    // If one rectangle is above other 
    if (toleY1 < boriY2 || toleY2 < boriY1) 
        return false; 
  
    return true; 
} 

void
cellnumber(double le, double ri, 
  double bo, double to, std::set<int> *cell_ids,
  std::vector<Cell2DTree>* cells)
{ 
  for(size_t ce=0; ce < cells->size(); ce++)
  {
    Cell2DTree* curr_cell = &cells->at(ce);

    if(rectOverlap(curr_cell->getValFromX(), curr_cell->getValToY(), 
    curr_cell->getValToX(), curr_cell->getValFromY(), le, to, ri, bo))
    {
      cell_ids->insert(curr_cell->getCellId());
    }
  }

  return;

}

/*
Value mapping function of operator ~cellnos\_ir~

*/
int
KDTree2D::Kdtree2dValueMapCellnos( Word* args, Word& result, int message,
    Word& local, Supplier s ) {
  KDTree2D *input_kdtree2d_ptr
    = static_cast<KDTree2D*>( args[0].addr );

  Rectangle<2> *search_window_ptr
    = static_cast<Rectangle<2>*>( args[1].addr );

  if (input_kdtree2d_ptr != nullptr && search_window_ptr != nullptr) {
    std::set<int> cell_ids;

    result = qp->ResultStorage(s);
    collection::IntSet* res = (collection::IntSet*) result.addr;

    Rectangle<2> * b_box = input_kdtree2d_ptr->getBoundingBox();
    int mode_ = input_kdtree2d_ptr->getMode();
    
    if (!search_window_ptr->Intersects(*b_box)) {
      cell_ids.insert(0);
      res->setTo(cell_ids);
      return 0;
    }

    // 'truncate' search window in case of partial cutting
    if (!b_box->Contains(*search_window_ptr)) {
      search_window_ptr = new Rectangle<2>(
        search_window_ptr->Intersection(*b_box));

      cell_ids.insert(0);
    }


    //std::vector<Cell2DTree>* cells = &input_kdtree2d_ptr->getCellVector();    
    double le = search_window_ptr->getMinX();
    double ri = search_window_ptr->getMaxX();
    double bo = search_window_ptr->getMinY();
    double to = search_window_ptr->getMaxY();

    // mode not set
    if(mode_ != 1 && mode_ != 2) {
      // use different cellnum method!
      cell_ids.clear();
      std::vector<Cell2DTree>* cells = &input_kdtree2d_ptr->getCellVector();    
      cellnumber(le, ri, bo, to, &cell_ids, cells);
      //cell_ids.insert(0);
      res->setTo(cell_ids);
      return 0;
    }

    /*
      Select GetLeaf function depending on input 
      if mode == 1: Calculation of 2dtree with middle of list of points
      else: Calculation of 2dtree with median of points
    */
    if(mode_ == 1) {
      std::vector<KDNodeList*> nodes = input_kdtree2d_ptr->getPointsVector();
      KDNodeList* root = nodes.back();
      GetLeaf(root, le, ri, bo, to, &cell_ids);
      // clear aux. vectors
      if (nodes.size() > 0) {
        nodes.clear();
      }
    } else {
      std::vector<KDMedList*> nodes = input_kdtree2d_ptr->getPointsMedVector();
      KDMedList* root2 = nodes.back();
      GetLeaf(root2, le, ri, bo, to, &cell_ids);
      // clear aux. vectors
      if (nodes.size() > 0) {
      nodes.clear();
      }
    }
        
    if(!cell_ids.empty()) {
      res->setTo(cell_ids);
    }

    if(cell_ids.size() > 0)
    {
      cell_ids.clear();
    }
    

    return 0;


  
    }
  
  return -1;
}


/*
  returns cell to a given id

*/
int
KDTree2D::Kdtree2dValueMapGetCell( Word* args, Word& result, int message,
    Word& local, Supplier s ) 
{
  KDTree2D *input_kdtree2d_ptr
    = static_cast<KDTree2D*>( args[0].addr );

  CcInt* cellno_ptr = static_cast<CcInt*>(args[1].addr);
  int cellno = cellno_ptr->GetIntval();

  if (input_kdtree2d_ptr != nullptr)
  {
    result = qp->ResultStorage( s );
    Rectangle<2> *res = (Rectangle<2>*) result.addr;

    std::vector<Cell2DTree>* cells = 
          &input_kdtree2d_ptr->getCellVector(); 

    for(size_t i = 0; i < cells->size(); i++)
    {
      Cell2DTree cell = cells->at(i);
      if(cell.getCellId() == cellno)
      {
        double min[2], max[2];
        min[0] = cell.getValFromX();
        min[1] = cell.getValFromY();
        max[0] = cell.getValToX();
        max[1] = cell.getValToY();
        res->Set(true, min, max);
        return 0;
      }
    }
  }
  return -1;
}

/*
  returns smallest common cellnum of two cells in
  a created grid

*/
int
KDTree2D::Kdtree2dValueMapSCC( Word* args, Word& result, int message,
    Word& local, Supplier s ) {
  KDTree2D *input_kdtree2d_ptr
    = static_cast<KDTree2D*>( args[0].addr );

  Rectangle<2> *search_window_ptr
    = static_cast<Rectangle<2>*>( args[1].addr );

    Rectangle<2> *search_window_ptr_2
    = static_cast<Rectangle<2>*>( args[1].addr );

  CcInt* cellno_ptr = static_cast<CcInt*>(args[3].addr);
  int cellno = cellno_ptr->GetIntval();

  int mode_ = input_kdtree2d_ptr->getMode();

  if (input_kdtree2d_ptr != nullptr && search_window_ptr != nullptr
    && search_window_ptr_2 != nullptr) {

      result = qp->ResultStorage( s );
      CcBool *res = (CcBool*) result.addr;
      bool boolval = false;

      // first rectangle
      double le = search_window_ptr->getMinX();
      double ri = search_window_ptr->getMaxX();
      double bo = search_window_ptr->getMinY();
      double to = search_window_ptr->getMaxY();

      // second rectangle
      double le_2 = search_window_ptr_2->getMinX();
      double ri_2 = search_window_ptr_2->getMaxX();
      double bo_2 = search_window_ptr_2->getMinY();
      double to_2 = search_window_ptr_2->getMaxY();

      std::set<int> cell_ids;
      std::set<int> cell_ids_2;

      if(mode_ == 1) {
        std::vector<KDNodeList*> nodes = input_kdtree2d_ptr->getPointsVector();
        KDNodeList* root = nodes.back();
        GetLeaf(root, le, ri, bo, to, &cell_ids);
        GetLeaf(root, le_2, ri_2, bo_2, to_2, &cell_ids_2);
      } else if(mode_ == 2) {
        std::vector<KDMedList*> nodes = 
            input_kdtree2d_ptr->getPointsMedVector();
        KDMedList* root = nodes.back();
        GetLeaf(root, le, ri, bo, to, &cell_ids);
        GetLeaf(root, le_2, ri_2, bo_2, to_2, &cell_ids_2);
      } else {
        cell_ids.clear();
        cell_ids_2.clear();
        std::vector<Cell2DTree>* cells = 
          &input_kdtree2d_ptr->getCellVector();    
        cellnumber(le, ri, bo, to, &cell_ids, cells);
        cellnumber(le_2, ri_2, bo_2, to_2, &cell_ids_2, cells);
      }

      std::vector<int> v(sizeof(cell_ids)+ sizeof(cell_ids_2));
      std::vector<int>::iterator it;

      it=std::set_intersection (cell_ids.begin(), cell_ids.end(),
         cell_ids_2.begin(), cell_ids_2.end(), v.begin());
      v.resize(it-v.begin());                      
  
      if(v.empty()) { 
      //no intersection between rectangles
        res->Set( true, boolval);
        return 0;

      }
      
      if(v[0] == cellno)
      {
        boolval = true;
        res->Set( true, boolval);
        return 0;
      }
      
    res->Set( true, boolval);
    return 0; 

    }


    return -1;
    }

