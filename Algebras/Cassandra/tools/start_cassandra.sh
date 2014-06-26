#!/bin/bash
#
# This script starts cassandra
# on multiple nodes
#
# Jan Kristof Nidzwetzki
#
#######################################

# Cassandra Nodes
nodes="node1 node2 node3 node4 node5 node6"

# Cassandra binary
cassandrabin="/opt/psec/nidzwetzki/cassandra/apache-cassandra-2.0.7/bin/cassandra"


for node in $nodes; do

   ssh $node $cassandrabin &

done
