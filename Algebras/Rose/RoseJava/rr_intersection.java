/* Generated by Together */

public class rr_intersection {

    public static void main (String[] args) {
	/*
	Triangle tg1 = new Triangle(new Point(469.6,188.8),
				    new Point(455.2,355.2),
				    new Point(28.0,265.6));
	Triangle tg2 = new Triangle(new Point(104.8,762.4),
				    new Point(78.4,100.0),
				    new Point(48.0,716.8));
	TriList tgl = tg1.intersection(tg2);
	tgl.print();
	Rational fact = new Rational(0.5);
	
	Point tg3 = new Point(455,355);
	tg1.zoom(fact);
	tg2.zoom(fact);
	tg3.zoom(fact);

	GFXout ghh = new GFXout();
	ghh.initWindow();
	ghh.add(tg1);
	ghh.add(tg2);
	ghh.add(tg3);
	ghh.showIt();
	*/
	
        SegList r1 = null;
        SegList r2 = null;
        TriList rResult = null;

	r1 = RPGConv.getRegions ("poly480_1.ply");
        if (!RPGConv.lastConversionSuccessful()){
            System.out.println (RPGConv.lastConversionErrorMsg());
            System.exit(0);
        }
        r2 = RPGConv.getRegions ("poly480_2.ply");
        if (!RPGConv.lastConversionSuccessful()){
            System.out.println (RPGConv.lastConversionErrorMsg());
            System.exit(0);
        }
	

	
	Rational fact = new Rational(80);
	for (int i = 0; i < r1.size(); i++) {
	    ((Segment)r1.get(i)).zoom(fact); }
	for (int i = 0; i < r2.size(); i++) {
	    ((Segment)r2.get(i)).zoom(fact); }
	

        TriList t1 = (new Polygons (r1)).triangles();
        TriList t2 = (new Polygons (r2)).triangles();
	
	/*
	GFXout g = new GFXout();
	g.initWindow();
	//g.addList(t1);
	//g.addList(r1);
	g.addList(t2);
	g.addList(r2);
	//g.add(new Point(396,541));
	//g.add(new Point(287,676));
	g.showIt();
	*/
	
	
	long summe = 0;
        for (int n = 0; n < 1; n++) {
	    
	    long millis1 = System.currentTimeMillis();
	    for (int i = 0; i<1; i++) {
                rResult = Algebra.rr_intersection (t1, t2);
		System.out.println(i+"...done");
            }
	    long millis2 = System.currentTimeMillis();
	    System.out.println ("Laufzeit: " + (millis2 - millis1));
	    summe = summe + (millis2 - millis1);
        }
	
        System.out.println ("Summe: " + summe);
        System.out.println ("Durchschnittliche Laufzeit: " + summe/10 + " ms");
	/*
	GFXout g = new GFXout();
	g.initWindow();
	g.addList(rResult);
	g.showIt();
	*/
    }


}
