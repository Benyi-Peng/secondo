%
% November 2004, M. Spiekermann
%
% Some TPC-H queries in Secondo SQL syntax

tpc10 :-
  sql(
select
      [
	c_custkey,
	c_name,
	sum(l_extendedprice * (1 - l_discount)) as revenue,
	c_acctbal,
	n_name,
	c_address,
	c_phone,
	c_comment
      ]
from
      [
	customer,
	orders,
	lineitem,
	nation
      ]
where
      [
	c_custkey = o_custkey,
	l_orderkey = o_orderkey,
	not(o_orderdate < cmpdate10_1),
	o_orderdate < cmpdate10_2,
	l_returnflag = "R",
	c_nationkey = n_nationkey
      ]
groupby
      [
	c_custkey,
	c_name,
	c_acctbal,
	c_phone,
	n_name,
	c_address,
	c_comment
       ]
orderby [ revenue desc ], 'head[20] consume').

tpc5 :-
   sql
select
       [
	n_name,
	sum(l_extendedprice * (1 - l_discount)) as revenue
       ]
from
       [
	customer,
	orders,
	lineitem,
	supplier,
	nation,
	region
       ]
where
       [
	c_custkey = o_custkey,
	l_orderkey = o_orderkey,
	l_suppkey = s_suppkey,
	c_nationkey = s_nationkey,
	s_nationkey = n_nationkey,
	n_regionkey = r_regionkey,
        r_name = "ASIA", 
	not(o_orderdate < cmpdate5_1), 
	o_orderdate < cmpdate5_2 
       ]
% groupby [ n_name asc ], The optimizer prints out an error, but computes a plan
% which causes an  in the groupby typemapping. Maybe the list structure is not
% analyzed carefully.
groupby [ n_name ]
orderby [ revenue desc ].


tpc3 :- 
   sql 
select
	[ 
          l_orderkey,
          sum(l_extendedprice * (1 - l_discount)) as revenue,
	  o_orderdate,
	  o_shippriority 
        ]
from
	[ 
          customer,
	  orders,
	  lineitem 
        ]
where
	[
          c_mktsegment = "BUILDING", 
          c_custkey = o_custkey,
	  l_orderkey = o_orderkey 
        ]
groupby
	[ 
          l_orderkey,
	  o_orderdate,
	  o_shippriority 
        ]
orderby
	[ 
          revenue desc,
	  o_orderdate asc 
        ]
first 10.

%select count(*) from lineitem.

tpc1 :- 
   sql 
select
	[ 
          count(*) as count_order,
          l_returnflag,
          l_linestatus,
          sum(l_quantity) as sum_qty,
          sum(l_extendedprice) as sum_base_price,
          sum(l_extendedprice * (1 - l_discount)) as sum_disc_price,
          sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge,
          avg(l_quantity) as avg_qty,
	  avg(l_extendedprice) as avg_price,
	  avg(l_discount) as avg_disc
        ]
from
	  lineitem 
where
	  l_shipdate < cmpdate1 
groupby [
	  l_returnflag,
	  l_linestatus
        ] 
orderby
	[ 
          l_returnflag asc,
	  l_linestatus asc 
        ].


