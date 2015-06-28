/*
This class is a FixedMRegion.

*/
using namespace std;
#include "FixedMRegion.h"
//#define DEBUG_VERBOSE
//#define DEBUG_VERBOSE2
/*
This is the default constructor. Do not use.

*/
FixedMRegion::FixedMRegion(): m(0), r(0){}

/*
This is the copy constructor.

*/
FixedMRegion::FixedMRegion(const FixedMRegion & f):t(f.t),m(f.m),r(f.r),
l(f.l){}


/*
This method creates a MMove with the given values.

*/ 
MMove FixedMRegion::createMMove(double _startX, double _startY, 
double _startangle, 
Instant _startt, double _endX, double _endY, double _endangle, 
Instant _endt){
  Interval < Instant > iv (_startt, _endt, false, true);
  UMove u01param = UMove(iv, _startX, _startY, _startangle, _endX, _endY,
       _endangle);
  MMove res(0);
  res.Clear ();
  res.StartBulkLoad ();
  res.MergeAdd (u01param);
  res.EndBulkLoad ();
  return res;
}

/*
This method creates a MMove with the given values.

*/ 
MMove FixedMRegion::createMMove(double _startX, double _startY, 
double _startangle, 
double _endX, double _endY, double _endangle){
  DateTime t1 (0.0);
  DateTime t2 (1.0);
  return createMMove(_startX, _startY, _startangle, t1,
                     _endX, _endY, _endangle, t2);
}
/*
This is the constructor that gets necessary information. 
region: object that shall be moved
\_startp: start point at starttime for the movement
\_startangle: start angle at starttime for the movement
\_endp: end point at endtime for the movement
\_endangle: end angle at endtime for the movement
rot\_center: rotational center
\_startt: start time
\_endt: end time 

*/

FixedMRegion::FixedMRegion(const Region & _region, const Point _startp, 
  const double _startangle, Instant _startt, const Point _endp, 
  const double _endangle, Instant _endt, const Point & rot_center): r(_region){
  
  m = createMMove(_startp.GetX(), _startp.GetY(), _startangle, _startt, 
     _endp.GetX(),
          _endp.GetY(), _endangle, _endt);
  t = _startt.ToDouble();
  
  xm = rot_center.GetX ();
  ym = rot_center.GetY ();
  calculateInternalVars ();
}

/*
This is the constructor that gets necessary information. 
region: object that shall be moved
\_startp: start point at starttime for the movement
\_startangle: start angle at starttime for the movement
\_endp: end point at endtime for the movement
\_endangle: end angle at endtime for the movement
rot\_center: rotational center
\_startt: start time
\_endt: end time 

*/

FixedMRegion::FixedMRegion(const Region & _region, const Point _startp, 
  const double _startangle, double _startt, const Point _endp, 
  const double _endangle, double _endt, const Point & rot_center):
  r(_region){
  m = createMMove(_startp.GetX(), _startp.GetY(), _startangle, 
  _startt, _endp.GetX(),
          _endp.GetY(), _endangle, _endt);
  t = _startt;
  
  xm = rot_center.GetX ();
  ym = rot_center.GetY ();
  calculateInternalVars ();
}

/*
This is the constructor that gets necessary information. 
region: object that shall be moved
move: the move object
rot\_center: the center of the rotation. It will be moved with the object.
starttime: the start time of the movement

*/
FixedMRegion::FixedMRegion(const Region & _region, const MMove & _move,
const Point & rot_center, double _starttime): m(_move), r(_region){
  t = _starttime;
  xm = rot_center.GetX ();
  ym = rot_center.GetY ();
  calculateInternalVars ();
}
/*
deprecated
This is the constructor that should not be used externally. It gets necessary information. 
$\_t$: never used
$\_xm$: x-value of the rotation center, that will be moved, too
$\_ym$: y-value of the rotation center, that will be moved, too
$\_r$: the region to be moved
$\_x0$: the x-startvalue of the linear movement
$\_y0$: the y-startvalue of the linear movement
$\_alpha0$: the starting angle
$\_vx$: x-value of the linear movement
$\_vy$: y-value of the linear movement
$\_valpha$: the angle of movement.

*/

FixedMRegion::FixedMRegion (double _t, double _xm, double _ym,
const Region & _r, double _x0, double _y0,double _alpha0, double _vx, 
double _vy, double _valpha): m(0), r(_r){
  t = _t;
  xm = _xm;
  ym = _ym;
  m = createMMove (_x0, _y0, _alpha0, _vx, _vy, _valpha);
  calculateInternalVars ();
}

/*
This is the constructor that should be used externally. It gets necessary information.
region: object that shall be moved
start: point at which the object ist placed at the beginning $t=0$
alpha\_start: angle of object at the bginning $t=0$
speed: speed or velociy (v) of the movement
alpha\_speed: speed or velociy (v) of the angle movement
rot\_center: the rotational center of the object
starttime: the start time of the movement

*/
FixedMRegion::FixedMRegion(const Region & _region, const Point & _start,
double alpha_start, const Point & _speed,double alpha_speed, 
const Point & rot_center,double _starttime): r(_region){
  m = createMMove (_start.GetX (), _start.GetY (), alpha_start, _speed.GetX (),
            _speed.GetY (), alpha_speed);
  t = _starttime;
  xm = rot_center.GetX ();
  ym = rot_center.GetY ();
  calculateInternalVars ();
}

/*
This is the standard destructor.

*/
FixedMRegion::~FixedMRegion(){}

/*
This is a method that accepts a list of regions. The regions represent 
spots and the movement will be calculated. The constructor expects identical 
regions that can be transformed by a translation or rotation. The region itself
 cannot cahnge its shape.

*/
Region FixedMRegion::interpolate(const Region spots[]){
  Point *
    p1 = new Point (true, 0.0, 0.0);
  Point *
    p2 = new Point (true, 1.0, 0.0);
  Point *
    p3 = new Point (true, 0.0, 1.0);
  Region
    result = Region (*p1, *p2, *p3);
  return result;
  //TODO
}

/*
This method will return a region that the FMRegion will have at the given 
time ti.

*/
void FixedMRegion::atinstant (double ti, Region &result){
  sett (ti);
  result.Clear();
  Line tmp(r.Size());
  tmp.StartBulkLoad ();
  for (int i = 0; i < r.Size (); i++)
    {
      HalfSegment hs;
      r.Get (i, hs);
      const Point lp = hs.GetLeftPoint ();
     // printf("Point old lp: %f, %f\n", (lp.GetX(), lp.GetY()));
      double newx = l.getImgX (lp.GetX (), lp.GetY ());
      double newy = l.getImgY (lp.GetX (), lp.GetY ());
      Point newlp (true, newx, newy);
      //printf("Point new lp: %f, %f\n", (newlp.GetX(), newlp.GetY()));
      const Point rp = hs.GetRightPoint ();
      //printf("Point old rp: %f, %f\n", rp.GetX(), rp.GetY());
      newx = l.getImgX (rp.GetX (), rp.GetY ());
      newy = l.getImgY (rp.GetX (), rp.GetY ());
      Point newrp (true, newx, newy);
      //printf("Point new rp: %f, %f\n", (newrp.GetX(), newrp.GetY()));
      HalfSegment hsnew(hs.IsLeftDomPoint (), newlp, newrp);
      hsnew.attr.edgeno = hs.attr.edgeno;
      tmp+=hsnew;
    }
  tmp.EndBulkLoad();
  tmp.Transform(result);
}


/*
This method will return a Point3 with the moving values x,y,alpha for the given 
time t.

*/
Point3 FixedMRegion::getMovingForTimeFromMMove (const double t) const{
  DateTime t2 (t);
  return getMovingForTimeFromMMove(t2);
}
/*
This method will return a Point3 with the moving values x,y,alpha for the given 
time t.

*/
Point3 FixedMRegion::getMovingForTimeFromMMove (const DateTime t) const{
  Intime < Point3 > tp;
  m.AtInstant (t, tp);
  return tp.value;
}
/*
This method returns the start time of the valid interval as an double
as absolute time.

*/
double FixedMRegion::getMMoveStart (const double dummy) const{
  //FIXME m nicht definiert oder 0 Komponenten
  UMove unit(0);
  m.Get (0, unit);
  Instant ita = unit.getTimeInterval ().start;
  return ita.ToDouble();
}
/*
This method moves the given Point inverse to the movement of the FixedMRegion
and gives it back.

*/
void FixedMRegion::getInv (double t, const Point & p, Point & result) const{
  Point3 coord = getMovingForTimeFromMMove(t);
  LATransform lt (coord.GetX(), coord.GetY(), xm, ym, coord.GetAlpha());
  double tmpx = lt.getOrigX (p.GetX (), p.GetY ());
  double tmpy = lt.getOrigY (p.GetX (), p.GetY ());
  result.SetDefined(true);
  result.Set(tmpx, tmpy);
}
/*
This method moves the given Point according to the movement of the FixedMRegion
and gives it back.

*/
void FixedMRegion::getTransPoint(DateTime t, const Point & p, Point& result) 
   const{
  Point3 coord = getMovingForTimeFromMMove(t);
  LATransform lt (coord.GetX(), coord.GetY(), xm, ym, coord.GetAlpha());
  double tmpx = lt.getImgX (p.GetX (), p.GetY ());
  double tmpy = lt.getImgY (p.GetX (), p.GetY ());
  result.SetDefined(true);
  result.Set(tmpx, tmpy);
}
/*
This method calculates the aprroximated movement of the given MPoint
with the given precision unter the condition that the FixedMRegion does
not move. Therefore, it uses an inverse movement of the Region and lets
the MPoint move that way in addition to its own movement.

*/
MPoint FixedMRegion::approxMovement (const MPoint & p, double precision) const{
  UPoint unit;
  p.Get (p.GetNoComponents () - 1, unit);
  Instant ite = unit.getTimeInterval ().end;
  p.Get (0, unit);
  Instant ita = unit.getTimeInterval ().start;

  //te and ta are relatime
  double ta = ita.ToDouble ();
  double te = ite.ToDouble ();
  int steps=(int)ceil((te-ta)/precision);

  MPoint res (0);
  res.StartBulkLoad ();

  DateTime to (ita);
  Point po(0);
  getInv(ta, unit.p0, po);
  //FIXME: MPoint-Intervalle nutzen und ev. die teilen, wenn sie zu lang sind!!
  for (int i=1; i<=steps; i++) {
    double now=((te-ta)/steps)*i+ta;
    DateTime tn(now);
    Intime < Point > tp;
    p.AtInstant (tn, tp);
    Point pn(0);
    getInv(now, tp.value, pn);
    Interval < Instant > in (to, tn, true, false);
    UPoint up (in, po, pn);
    res.MergeAdd (up);
    to = tn;
    po = pn;
  }
  res.EndBulkLoad();
  return res;
}
/*
This method calculates the step with depending on alpha.

*/
int FixedMRegion::calcStepWith(const double _ta, const double _te) const {
  //double alpha_a;
  //double alpha_e;
  Intime<Point3> p(0);
  m.AtInstant(_ta, p);
  Point3 p3 = p.value;
  double alpha_a = p3.GetAlpha();
  m.AtInstant(_te, p);
  p3 = p.value;
  double alpha_e = p3.GetAlpha();
  double tmp = (alpha_e - alpha_a);
  ///(_te-_ta);
  double kr = M_PI/16;
  if(tmp <= kr){
    return 1;
  }
  int steps=(int)ceil((tmp)/(kr));
  return steps;
}
/*
This method calculates the aprroximated movement of the given MPoint
with the given precision unter the condition that the FixedMRegion does
not move. Therefore, it uses an inverse movement of the Region and lets
the MPoint move that way in addition to its own movement.

*/
MPoint FixedMRegion::approxMovementNew2(const MPoint & p) 
const{
  UPoint unit;
  p.Get (p.GetNoComponents () - 1, unit);
  Instant ite = unit.getTimeInterval ().end;
  p.Get (0, unit);
  Instant ita = unit.getTimeInterval ().start;

  //te and ta are relatime
  double ta = ita.ToDouble ();
  double te = ite.ToDouble ();
  //int steps=(int)ceil((te-ta)/precision);
  int steps = calcStepWith(ta, te);
  MPoint res (0);
  res.StartBulkLoad ();

  DateTime to (ita);
  Point po(0);
  getInv(ta, unit.p0, po);
  //FIXME: MPoint-Intervalle nutzen und ev. die teilen, wenn sie zu lang sind!!
  for (int i=1; i<=steps; i++) {
    double now=((te-ta)/steps)*i+ta;
    DateTime tn(now);
    Intime < Point > tp;
    p.AtInstant (tn, tp);
    Point pn(0);
    getInv(now, tp.value, pn);
    Interval < Instant > in (to, tn, true, false);
    UPoint up (in, po, pn);
    res.MergeAdd (up);
    to = tn;
    po = pn;
  }
  res.EndBulkLoad();
  return res;
}
/*
This method calculates the aprroximated movement of the given MPoint
with the given precision unter the condition that the FixedMRegion does
not move. Therefore, it uses an inverse movement of the Region and lets
the MPoint move that way in addition to its own movement.

*/
MPoint FixedMRegion::approxMovementNew (const MPoint & p) const{
  UPoint unit;
  RefinementPartition<MPoint,MMove,UPoint,UMove> rp(p, m);
  MPoint res (0);
  res.StartBulkLoad ();
  for (unsigned int i = 0; i < rp.Size(); i++) {
    Interval<Instant> iv;
    int urPos;
    int upPos;
    rp.Get(i, iv, urPos, upPos);
    //te and ta are relatime
    double ta = iv.start.ToDouble();
    double te = iv.end.ToDouble();
    printf("Intervall %d: von %f bis %f\n", i, ta, te);
    int steps = calcStepWith(ta, te);
    DateTime to (ta);
    Intime < Point > tpo;
    p.AtInstant (to, tpo);
    Point po(0);
    getInv(ta, tpo.value, po);
    for (int i=1; i<=steps; i++) {
      double now=((te-ta)/steps)*i+ta;
      DateTime tn(now);
      Intime < Point > tp;
      p.AtInstant (tn, tp);
      Point pn(0);
      getInv(now, tp.value, pn);
      Interval < Instant > in (to, tn, true, false);
      UPoint up (in, po, pn);
      res.MergeAdd (up);
      to = tn;
      po = pn;
    }
  }
  res.EndBulkLoad();
  return res;
}
/*
This method creates a non moving MRegion.

*/
MRegion *FixedMRegion::buildMovingRegion(){
  MRegion *mr;
  Region r(0);
  atinstant (getMMoveStart(0.0), r);
  MPoint rock (0);
  rock.Clear ();
  rock.StartBulkLoad ();
  DateTime t1 (instanttype);
  t1.Set (0, 1, 1, 0, 0);
  DateTime t2 (instanttype);
  t2.Set (3999, 12, 31, 23, 59);
  Interval < Instant > iv (t1, t2, false, true);
  UPoint ub (iv, 0, 0, 0, 0);
  rock.MergeAdd (ub);
  rock.EndBulkLoad ();
  //create non moving MRegion
  mr = new MRegion (rock, r);
  return mr;
}

/*
This method will join adjacent UBools, if they have got the same value.

*/
MBool FixedMRegion::joinBools(MBool a){
  MBool res(0);
  res.Clear();
  res.StartBulkLoad();

  if(a.GetNoComponents()==0){
   res.EndBulkLoad ();
  return res;
  }
  
  bool start=true;
  int i=0;   
  UBool up;
  a.Get(i, up);
  DateTime t_start = up.getTimeInterval().start;
  DateTime t_end = up.getTimeInterval().end;
  CcBool val = up.constValue;
  i++;
  while(i<a.GetNoComponents()){
   a.Get(i, up);
   if((t_end == up.getTimeInterval().start) && (val == up.constValue)){
     t_end = up.getTimeInterval().end;
   }else{
     if (t_start!=t_end) {
       Interval < Instant > iv (t_start, t_end, start, true);
       UBool ub (iv, (CcBool) val);
       res.MergeAdd (ub);
     }
     start=false;
     t_start = up.getTimeInterval().start;
     t_end = up.getTimeInterval().end;
     val = up.constValue;
   }
   i++;
  }
  if (t_start!=t_end) {
    Interval < Instant > ive (t_start, t_end, start, true);
    UBool ube (ive, (CcBool) val);
    res.MergeAdd (ube);
  }
  
  res.EndBulkLoad ();
  return res;
}

/*
This method will calculate a MPoint with which is defined, if the MPoint mp 
is inside the FMRegion at the given time and else not defined. The MPoint 
will be calculated for the time intervall ta to te.

*/
MBool FixedMRegion::inside (const MPoint & mp){
  MRegion * rfix = buildMovingRegion ();
  //MPoint tmp = approxMovement (mp, 1e-5);
  //MPoint tmp = approxMovementNew(mp);
  MPoint tmp = approxMovementNew2(mp);
  //**TestCode**
  for (int i=0; i<tmp.GetNoComponents(); i++) {
    UPoint t;
    tmp.Get(i, t);
    printf("%2d: (%f, %f)@%f - (%f, %f)@%f\n",
    i, t.p0.GetX(), t.p0.GetY(), t.getTimeInterval().start.ToDouble(),
    t.p1.GetX(), t.p1.GetY(), t.getTimeInterval().end.ToDouble());
  }
  //**End TestCode**
  MBool res (0);
  rfix->Inside (tmp, res);
  delete rfix;
  MBool result = joinBools(res);
  return result;
}


/*
This method will remove the inverse movement of the region from the given 
MPoint. This movement of the Region should have been added some steps 
before from approxMovement.

*/
//FIXME: Zeitintervalle aus p holen und Orte aus dem Orginalpunkt
MPoint FixedMRegion::reverseMovement (const MPoint & p) const{
  MPoint res (0);
  res.StartBulkLoad ();
  for (int i = 0; i < p.GetNoComponents (); i++)
    {
      UPoint unit;
      p.Get (i, unit);
      Point p0, p1;
      getTransPoint (unit.getTimeInterval().start, unit.p0, p0);
      getTransPoint (unit.getTimeInterval().end, unit.p1, p1);
      UPoint u2 (unit.getTimeInterval(), p0, p1);
      res.MergeAdd (u2);
    }
  res.EndBulkLoad ();
  return res;

}
/*
This method will calculate a MPoint which is defined, if the MPoint mp 
intersects with the FMRegion at the given time and else not defined. The 
MPoint will be calculated for the time intervall ta to te.

*/
MPoint FixedMRegion::intersection(MPoint & mp){
  //MRegion *
//    rfix = buildMovingRegion ();
  MPoint
    tmp = approxMovement (mp, 1e-2);
  MPoint
    res (0);
  //rfix->Intersection (tmp, res);
  Region r(0);
  atinstant(getMMoveStart(0.0), r);
  tmp.AtRegion(&r, res);
  return reverseMovement (res);
}

/*
This method will calculate a MPoint which is defined, if the MPoint mp 
intersects with the FMRegion at the given time and else not defined. The 
MPoint will be calculated for the time intervall ta to te.

*/
MPoint FixedMRegion::intersectionNew(MPoint & mp) {
  MBool parts = inside(mp);
  MPoint result(0);
  result.Clear();
  result.StartBulkLoad();
  for (int i=0; i<parts.GetNoComponents(); i++) {
    UBool up;
    parts.Get(i, up);
    if(up.constValue==(CcBool)true){
      DateTime t_start = up.getTimeInterval().start;
      DateTime t_end = up.getTimeInterval().end;
      Interval < Instant > iv (t_start, t_end, false, true);
      //FIXME richtige true fals übernehmen
      Intime<Point> ip_start(0);
      Intime<Point> ip_end(0);
      mp.AtInstant(t_start, ip_start);
      mp.AtInstant(t_end, ip_end);
      Point p_start = ip_start.value;
      Point p_end = ip_end.value;
      UPoint ub(iv, p_start, p_end);
      result.MergeAdd(ub);
    }
  }
  result.EndBulkLoad();
  return result;
}
/*
This method will calculate the Region which contains all points / areas, that
the FMRegion has at least on time (or more often) traversed in the given 
intervall ta to te. 
deprecated!

*/
/*void FixedMRegion::traversed2 (double ta, double te, double precision,
  Region & result){
  Region *res = NULL;
  for (double i = 0; i <= (te - ta); i = i + precision)
    {
      Region *tmp = atinstant (ta + i);
      if (res == NULL)
        {
          res = tmp;
        }
      else
        {
          Region *tmp2 = new Region (*res);
          tmp2->Clear ();
          RobustPlaneSweep::robustUnion (*res, *tmp, *tmp2);
          delete tmp;
          delete res;
          res = tmp2;
        }
    }
  return res;
}*/

/*
This method calculates the mass center of the given points.

*/
Point calculateMassCenter(const vector<Point> &p){
  return Point(false, -8.0, -9.0);
}

/*
This method calculates the 
intersection point of the given lines p1p2 and p3p4.

*/
Point FixedMRegion::getIntersectionPoint (const Point & p1,
  const Point & p2,const Point & p3, const Point & p4){
  double x1 = p1.GetX ();
  double y1 = p1.GetY ();
  double x2 = p2.GetX ();
  double y2 = p2.GetY ();
  double x3 = p3.GetX ();
  double y3 = p3.GetY ();
  double x4 = p4.GetX ();
  double y4 = p4.GetY ();

  double a, b, c, d, e, f, D, Dx;
  double t;

  a = x1 - x3;
  b = x1 - x2;
  c = x4 - x3;

  d = y1 - y3;
  e = y1 - y2;
  f = y4 - y3;

  D = b * f - c * e;

  //if (D==0) {
  //Fallunterscheidung nicht notwendig, weil nur in Spezialfällen aufgerufen
  //}

  Dx = a * f - c * d;

  t = Dx / D;

  double x = x1 + (x2 - x1) * t;
  double y = y1 + (y2 - y1) * t;
  Point p_res = Point (true, x, y);
  return p_res;
}

/*
This method will find out, which case the two lines p1p2 and p3p4 
belong to. The cases have got a short explanation in getTraversedArea.
A two digit value in a number system of base 4 is the result.

*/
int FixedMRegion::getTraversedCase (const Point & p1, const Point & p2,
 const Point & p3, const Point & p4){
  double x1 = p1.GetX ();
  double y1 = p1.GetY ();
  double x2 = p2.GetX ();
  double y2 = p2.GetY ();
  double x3 = p3.GetX ();
  double y3 = p3.GetY ();
  double x4 = p4.GetX ();
  double y4 = p4.GetY ();
#ifdef DEBUG_VERBOSE
  printf ("gettraversedcase P1: (%f, %f), P2:"
          "(%f, %f), P3: (%f, %f), P4: (%f, %f), ", x1, y1, x2, y2, x3, y3, x4,
          y4);
#endif
  double a, b, c, d, e, f, D, Dx, Dy;
  double t, s;

  a = x1 - x3;
  b = x1 - x2;
  c = x4 - x3;

  d = y1 - y3;
  e = y1 - y2;
  f = y4 - y3;

  D = b * f - c * e;

  if (AlmostEqual (D, 0.0))
    {
      //Fallunterscheidung Geraden parallel oder aufeinander:
      //Strecken parallel zur x-Achse
      if (AlmostEqual (x1, x2))
        {
          if (AlmostEqual (x3, x1))
            {
              return -2;
            }
          else
            {
              return -1;
            }
        }
      double t = (x3 - x1) / (x2 - x1);
      if (AlmostEqual (y3, y1 + (y2 - y1) * t))
        {
          //Geraden liegen aufeinander
          return -2;
        }
      else
        {
          //Geraden liegen parallel
          return -1;
        }
    }

  Dx = a * f - c * d;
  Dy = b * d - a * e;

  t = Dx / D;
  s = Dy / D;
//Strecke1: (x1, y1), (x2, y2)
//Strecke2: (x3, y3), (x4, y4)
  int cc = 0;                   // Schnitt außerhalb
  //Fallunterscheidung nach (x1, y1), (x2, y2)
  if ((t > 0) && (t < 1))
    cc = 3;                     //Schnitt zwischen (x1, y1) und (x2, y2)
  if (AlmostEqual (t, 0))
    cc = 1;                     //Schnitt auf (x1, y1)
  if (AlmostEqual (t, 1))
    cc = 2;                     //Schnitt auf (x2, y2)
  //Fallunterscheidung nach (x3, y3), (x4, y4)
  if (AlmostEqual (s, 0)) {
    cc += 4;                    //Schnitt auf (x3, y3)
  } else if (AlmostEqual (s, 1)) {
    cc += 8;                    // Schnitt auf (x4, y4)
  } else if ((s > 0) && (s < 1)) {
    cc += 12;                   //Schnitt zwischen (x3, y3) und (x4, y4)
  }

#ifdef DEBUG_VERBOSE
  printf ("Schnittpunkt: %3.2f, %3.2f\n", x1 + (x2 - x1) * t,
          y1 + (y2 - y1) * t);
#endif

  return cc;
}

/*
This method creates a vectorvector and puts the given vector into it.

*/
vector < vector < Point > >FixedMRegion::traversedGetVectorVector (vector <
  Point > v){
  vector < vector < Point > >vv;
  vv.push_back (v);
  return vv;
}

/*
This methods creates a polygon of the given Points.
This method adds four edges, the given points, to a vector. This is a 
preparation to create a quadrangle.

*/
vector < vector < Point > >FixedMRegion::traversedCalculateQuadrangle(
  const Point & p1,const Point & p2, const Point & p3, const Point & p4){
  vector < Point > v;
  v.push_back (p1);
  v.push_back (p2);
  v.push_back (p3);
  v.push_back (p4);
  v.push_back (p1); //once again the first one
  return traversedGetVectorVector (v);
}

/*
This is an Internal method that is used from traversedCalculateTriangle.
This methods creates a polygon of the given Points.
This method adds three edges, the given points, to a vector. This is a 
preparation to create a triangle.

*/
vector < Point > FixedMRegion::traversedCalcTriangle (const Point & p1,
 const Point & p2, const Point & p3){
  vector < Point > v;   
  v.push_back (p1);
  v.push_back (p2);
  v.push_back (p3);
  v.push_back (p1);  
  return v;
}

/*
This methods creates a polygon of the given Points.
This method adds three edges, the given points, to a vector. This is a 
preparation to create a triangle.

*/
vector < vector < Point > >FixedMRegion::traversedCalculateTriangle(
  const Point & p1,const Point & p2, const Point & p3){
  vector < Point > v;   
  v = traversedCalcTriangle (p1, p2, p3);
  return traversedGetVectorVector (v);
}

/*
This methods creates two polygons of the given Points, using the intersection
of the lines p1p2 and p3p4.
It will prepare two vectors with triangles and gives them back in a vectorvector.

*/
vector < vector < Point > >FixedMRegion::traversedCalculateTwoTriangles(
  Point p1, Point p2,Point p3, Point p4){
  Point pi = getIntersectionPoint (p1, p2, p3, p4);
  vector < Point > t1;  
  vector < Point > t2;
  t1 = traversedCalcTriangle (pi, p1, p3);
  t2 = traversedCalcTriangle (pi, p2, p4);
  vector < vector < Point > >vv;
  vv.push_back (t1);
  vv.push_back (t2);
  return vv;
}

/*
This methods finds out the correct case and calls other methods to calculate 
the polygon that was traversed in this step. It uses the halfsegments' start 
and end position. It has to deal with al lot of different cases.

*/
vector < vector < Point > >FixedMRegion::getTraversedArea(
  const HalfSegment & hsold,const HalfSegment & hsnew){
  vector < vector < Point > >res;
  Point p1 = hsold.GetDomPoint ();
  Point p2 = hsold.GetSecPoint ();
  Point p3 = hsnew.GetDomPoint ();
  Point p4 = hsnew.GetSecPoint ();
  vector < Point > v;
  int
    casetype = getTraversedCase (p1, p2, p3, p4);
#ifdef DEBUG_VERBOSE2
  printf ("P1: (%f, %f), P2: (%f, %f), P3: (%f, %f), P4: (%f, %f), ",
          p1.GetX (), p1.GetY (),
          p2.GetX (), p2.GetY (),
          p3.GetX (), p3.GetY (), p4.GetX (), p4.GetY ());
  printf ("Case: %d\n", casetype);
#endif
  switch (casetype)
    {
    case -1:   //Strecken sind parallel
    case 0:    //Schnittpunkt liegt außerhalb beider Strecken
    case 3:    //Schnittpunkt liegt innerhalb Strecke1 und außerhalb Strecke2
    case 12:   //Schnittpunkt liegt außerhalb Strecke1 und innerhalb Strecke2
      res = traversedCalculateQuadrangle (p1, p2, p4, p3);
      break;

    case 15:   //Schnittpunkt liegt innerhalb Strecke1 und innerhalb Strecke2
      res = traversedCalculateTwoTriangles (p1, p2, p3, p4);
      break;
    case -2:   //Strecken liegen auf der selben Geraden
      //Entartung
      break;
    case 2:    //Schnittpunkt liegt auf P2 und außerhalb Strecke2
      res = traversedCalculateTriangle (p1, p2, p3);
      break;
    case 1:    //Schnittpunkt liegt auf P1 und außerhalb Strecke2
      res = traversedCalculateTriangle (p1, p2, p4);
      break;
    case 4:    //Schnittpunkt liegt außerhalb Strecke1 und auf P3
      res = traversedCalculateTriangle (p2, p3, p4);
      break;
    case 5:    //Schnittpunkt liegt auf P1 und auf P3
      res = traversedCalculateTriangle (p1, p2, p4);
      break;
    case 8:    //Schnittpunkt liegt außerhalb Strecke1 und auf P4
      res = traversedCalculateTriangle (p1, p3, p4);
      break;
    case 6:    //Schnittpunkt liegt auf P2 und auf P3
      res = traversedCalculateTriangle (p1, p2, p4);
      break;
    case 7:    //Schnittpunkt liegt innerhalb Strecke1 und auf P3
      res = traversedCalculateTriangle (p1, p2, p4);
      break;
    case 9:    //Schnittpunkt liegt auf P1 und auf P4
      res = traversedCalculateTriangle (p1, p2, p3);
      break;
    case 10:   //Schnittpunkt liegt auf P2 und auf P4
      res = traversedCalculateTriangle (p1, p2, p3);
      break;
    case 11:   //Schnittpunkt liegt innerhalb Strecke1 und auf P4
      res = traversedCalculateTriangle (p1, p3, p4);
      break;
    case 13:   //Schnittpunkt liegt auf P1 und innerhalb Strecke2
      res = traversedCalculateTriangle (p1, p2, p4);
      break;
    case 14:   //Schnittpunkt liegt auf P2 und innerhalb Strecke2
      res = traversedCalculateTriangle (p1, p2, p3);
      break;

    default:
      assert (false);
      break;
    }
  return res;
}
/*
This method orientates the given polygons clockwise. Therefore, they will be 
faces.

*/
void FixedMRegion::traversedCreateCyclesNotHoles(
     vector < vector < Point > >&v_list){
#ifdef DEBUG_VERBOSE
  printf ("Numcycles: %d", v_list.size ());
#endif
  for (size_t i = 0; i < v_list.size (); i++)
    {
      if (!getDir (v_list[i]))
        {
#ifdef DEBUG_VERBOSE
          printf ("R: %d, ", i);
#endif
          reverseCycle (v_list[i]);
        }
    }
#ifdef DEBUG_VERBOSE
  printf ("\n");
#endif
}

/*
This method extracts a list of halfsegments from the region.

*/
vector < HalfSegment > FixedMRegion::getHSFromRegion(){
  vector < HalfSegment > result;
  for (int i = 0; i < r.Size (); i++)
    {
      HalfSegment tmp;
      r.Get (i, tmp);
      if (tmp.IsLeftDomPoint ())
        {
          result.push_back (tmp);
        }
    }
  return result;
}
/*
This method extracts a list of halfsegments from the given region.

*/
vector < HalfSegment > FixedMRegion::getHSFromRegion (Region reg){
   vector < HalfSegment > result;
  for (int i = 0; i < reg.Size (); i++)
    {
      HalfSegment tmp;
      reg.Get (i, tmp);
      if (tmp.IsLeftDomPoint ())
        {
          result.push_back (tmp);
        }
    }
  return result;
}

/*
This method will return a list of Halfsegments that the FMRegion will have at the given 
time ti.

*/
void FixedMRegion::atinstant (double ti,
 const vector < HalfSegment > &v, vector < HalfSegment > & res){
  sett (ti);
  HalfSegment hs;
  for (size_t i = 0; i < v.size (); i++)
    {
      hs = v[i];

      const
        Point
        lp = hs.GetLeftPoint ();
      double
        newx = l.getImgX (lp.GetX (), lp.GetY ());
      double
        newy = l.getImgY (lp.GetX (), lp.GetY ());
      Point newlp (true, newx, newy);

      const
        Point
        rp = hs.GetRightPoint ();
      newx = l.getImgX (rp.GetX (), rp.GetY ());
      newy = l.getImgY (rp.GetX (), rp.GetY ());
      Point newrp (true, newx, newy);
      hs.Set (hs.IsLeftDomPoint (), newlp, newrp);

      res.push_back (hs);
    }
}


/*
This methods creates a region that contains the traversed area between the 
step's region at t\_step\_start and t\_step\_end.

*/
Region * FixedMRegion::getDiffRegion (const vector < HalfSegment >
 *resultold, const vector < HalfSegment > *resultnew){
  Region *diffregion = NULL;
  Region *tmp2 = NULL;
  Region *tmp_region = NULL;
  HalfSegment hsold;
  HalfSegment hsnew;
  for (size_t i = 0; i < resultold->size (); i++)
    {
      hsold = (*resultold)[i];
      hsnew = (*resultnew)[i];
      vector < vector < Point > >tmp_polygons = getTraversedArea (hsold, hsnew);
      //printf("tmp_polygons.size (): %d\n",tmp_polygons.size ());
      if (tmp_polygons.size () > 0)
        {
          //FIXME: routine zum orientierung prüfen und korrigieren
          traversedCreateCyclesNotHoles (tmp_polygons);

          tmp_region = buildRegion2 (tmp_polygons); 
#ifdef DEBUG_VERBOSE
          vector < Region * >v;
          tmp_region->Components (v);
          printf ("No. Components: %d\n", v.size ());
          for (size_t j = 0; j < v.size (); j++)
            delete v[j];
#endif

          if (diffregion == NULL)
            {
              diffregion = tmp_region;
            }
          else
            {
              tmp2 = new Region (0);
              RobustPlaneSweep::robustUnion (*diffregion, *tmp_region, *tmp2);
              delete diffregion;
              delete tmp_region;
              diffregion = tmp2;
            }
        }
    }
  return diffregion;
}
/*
This method will calculate the Region which contains all points / areas, that
the FMRegion has at least on time (or more often) traversed in the given 
intervall ta to te. 

*/
void FixedMRegion::traversed(Region & result,double ta, double te, 
                             double precision){
  Region * res= new Region(0);
  atinstant (ta, *res);
  vector < HalfSegment > vhs = getHSFromRegion ();
  vector < HalfSegment > tiold;
  atinstant (ta, vhs, tiold);
  for (double i = 0; i <= (te - ta); i = i + precision)
    {
      vector < HalfSegment > tinew;
      atinstant (ta + i, vhs, tinew);
      Region *tmp = getDiffRegion (&tiold, &tinew);
      if (tmp != NULL)
        {
          Region *tmp2 = new Region (0);
          tmp2->Clear ();
          //printf("union: %d,  %d\n", res->Size(), tmp->Size());
          RobustPlaneSweep::robustUnion (*res, *tmp, *tmp2);
          delete tmp;
          delete res;
          res = tmp2;
        }
      //delete tiold;
      tiold = tinew;
    }
  //delete tiold;
  result = *res;
  delete res;
  return;
}
/*
This method will calculate the Region which contains all points / areas, that
the FMRegion has at least on time (or more often) traversed in the given 
intervall ta to te. 

*/
void FixedMRegion::traversedNew(Region & result,double ta, double te){
   result.Clear();
  //RefinementPartition<MMove,MMove,UMove,UMove> rp(m, m);
  //printf("Size: %d\n", m.size);
  for ( int i = 0; i < m.GetNoComponents(); i++) {
    //int urPos;
    //int upPos;
    //m.Get(i, iv, urPos, upPos);
    UMove um(0);
    m.Get(i, um);
    Interval<Instant> iv=um.getTimeInterval();
    //iv=um.value;
    //te and ta are relatime
    double _ta = iv.start.ToDouble();
    double _te = iv.end.ToDouble();
    printf("Int %d: %f, %f, %f, %f\n", i, ta, te, _ta, _te);
    if (_ta<ta)
      _ta=ta;
    if (_te>te)
      _te=te;
    if(_ta<_te){
  Region * res= new Region(0);
  atinstant (_ta, *res);
  vector < HalfSegment > vhs = getHSFromRegion ();
  vector < HalfSegment > tiold;
  atinstant (_ta, vhs, tiold);
      printf("Intervall %d: von %f bis %f\n", i, ta, te);
      int steps = calcStepWith(_ta, _te);
      for(int i=1; i<=steps; i++){
        vector < HalfSegment > tinew;
        atinstant (_ta + (_te-_ta)/steps*i, vhs, tinew);
        Region *tmp = getDiffRegion (&tiold, &tinew);
        if (tmp != NULL){
          Region *tmp2 = new Region (0);
          tmp2->Clear ();
          //printf("union: %d,  %d\n", res->Size(), tmp->Size());
          RobustPlaneSweep::robustUnion (*res, *tmp, *tmp2);
          delete tmp;
          delete res;
          res = tmp2;
        }
        //delete tiold;
        tiold = tinew;
      }
    Region tmp(result);
    result.Clear();
    RobustPlaneSweep::robustUnion (*res, tmp, result);
    delete res;
    }
  }
  //delete tiold;
  //result = *res;
  //delete res;
  return;
}
/*
This method returns the non moving region.

*/
const void FixedMRegion::getRegion(Region & result) const{
  result= r;
}
/*
This method sets the non moving region.

*/
void FixedMRegion::setRegion(const Region & _r){
  r = _r;
}
/*
This method returns the time t.
not used

*/
const double FixedMRegion::gett() const{
  return t;
}
/*
This method sets the time t.
not used

*/
void FixedMRegion::sett(double _t){
  t = _t;
  calculateInternalVars ();
}
/*
This method calculates the internal variables. It is used, when
the time has changed.

*/
void FixedMRegion::calculateInternalVars(){
  Point3 coord = getMovingForTimeFromMMove(DateTime(t));
  //printf("Point3(%f): (%f, %f, %f)\n", t, 
    //     coord.GetX(), coord.GetY(), coord.GetAlpha());
  l = LATransform (coord.GetX(), coord.GetY(), 
       xm, ym, coord.GetAlpha());
}
/*
This method returns the LATransform.

*/
const LATransform &FixedMRegion::getLATransform() const {
  return l;
}
/*
This method sets the LATransform.

*/
void FixedMRegion::setLATransform(const LATransform & _l){
  l = _l;
}
/*
This method returns the Move.

*/
const MMove &FixedMRegion::getMove(){
  return m;
}
/*
This method sets the Move.

*/
void FixedMRegion::setMove(const MMove & _m){
  m = _m;
}



/*
This method generates a list of all Points of the region.

*/
vector<Point> FixedMRegion::generateListOfRegionPoints(const Region &r){
  return vector<Point>(0);
}
/*
This calculates the euclidian distance between the two points.

*/
double FixedMRegion::getOneDistance(const Point &p1, const Point &p2){
  //https://de.serlo.org/mathe/deutschland/bayern/gymnasium/
  //klasse-12/geraden-und-ebenen-im-raum/abstand-zweier-punkte-berechnen
   //printf("p1.x= %f\n", p1.GetX());
   //printf("p2.x= %f\n", p2.GetX());
   //printf("p1.y= %f\n", p1.GetY());
   //printf("p2.y= %f\n", p2.GetY());   
  double res = sqrt(pow(p2.GetX() - p1.GetX(), 2) + pow(p2.GetY() 
  - p1.GetY(), 2) );
  return res;
}
/*
This method calculates the euclidian distance between the points. One Point 
willl always be the given one.

*/
vector<double> FixedMRegion::getDistancesForPoint(int numberOfPoint, 
  const vector<Point> &p){
  vector<double> res = vector<double>(0);
  for(size_t i=0;i<p.size();i++){
    double z= getOneDistance(p[i], p[numberOfPoint]);
    res.push_back(z);
  }
  return res;
}
/*
This method calculates the distance matrix of the points.

*/
vector<vector<double> > FixedMRegion::generateDistancesMatrix(const 
  vector<Point> &p){
  vector<vector<double> > res= vector<vector<double> >(0);
  for(size_t i=0;i<p.size();i++){
    vector<double> r = vector<double>(0);
    r=getDistancesForPoint(i, p);
    res.push_back(r);
  }
  return res;
}
/*
This method identifies the line of the matrix, which equals the given 
line of distances.
return value: starts with zero for the first element
-1 in case of error

*/
int FixedMRegion::identifyPoint(const vector<vector<double> > 
  &matrixOfDistancesOfRegion1, const vector<double> 
  &listOfDistancesOfRegion2){
  int ret =-1;
  for(size_t i=0;i<=matrixOfDistancesOfRegion1.size();i++){
    ret=i;
    if(listOfDistancesOfRegion2==matrixOfDistancesOfRegion1[i]){
      return ret;
    }
  }
  return ret;
}
/*
This method returns a list that has got the numbers of matrix lines two whom
correspond to the given [i]-line of matrix2.
FIXME: erläutern, wer mit wem
return value: starts with zero for the first element
-1 in case of error

*/
vector<int> FixedMRegion::identifyPoints(const vector<vector<double> > 
&matrixOfDistancesOfRegion1, const vector<vector<double> > 
&matrixOfDistancesOfRegion2){
  vector<int> ret= vector<int>(0);
  for(size_t i=0;i<=matrixOfDistancesOfRegion2.size();i++){
    int tmp= identifyPoint(matrixOfDistancesOfRegion1, 
      matrixOfDistancesOfRegion2[i]);
    ret.push_back(tmp);
  }
  return ret;
}

;
