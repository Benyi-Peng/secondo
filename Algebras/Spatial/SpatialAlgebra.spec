operator touches alias TOUCHES pattern _ infixop _
operator adjacent alias ADJACENT pattern _ infixop _
operator overlaps alias OVERLAPS pattern _ infixop _
operator onborder alias ONBORDER pattern _ infixop _
operator ininterior alias ININTERIOR pattern _ infixop _
operator crossings alias CROSSINGS pattern op ( _, _ )
operator single alias SINGLE pattern  op ( _ )
operator distance alias DISTANCE pattern  op ( _, _ )
operator direction alias DIRECTION pattern  op ( _, _ )
operator nocomponents alias NOCOMPONENTS pattern  op ( _ )
operator nohalfseg alias NOHALFSEG pattern  op ( _ )
operator size alias SIZE pattern  op ( _ )
operator touchpoints alias TOUCHPOINTS pattern  op ( _, _ )
operator commonborder alias COMMONBORDER pattern  op ( _, _ )
operator bbox alias BBOX pattern  op ( _ )
operator insidepathlength alias INSIDEPATHLENGTH pattern  _ infixop _
operator insidescanned alias INSIDESCANNED pattern  _ infixop _
operator insideold alias INSIDEOLD pattern  _ infixop _
operator translate alias TRANSLATE pattern  _ op [list]
operator clip alias CLIP pattern  op (_, _)
operator windowclippingin alias WINDOWCLIPPINGIN pattern op ( _ , _)
operator windowclippingout alias WINDOWCLIPPINGOUT pattern op ( _ , _)
operator vertices alias VERTICES pattern op ( _ )
operator atpoint alias ATPOINT pattern _ op[ _ ]
operator atposition alias ATPOSITION pattern _ op[ _ ]
operator subline alias SUBLINE pattern _ op[ _, _ ]

